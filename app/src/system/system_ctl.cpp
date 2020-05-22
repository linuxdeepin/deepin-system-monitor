#include <sys/utsname.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>

#include <QDebug>
#include <QtDBus>
#include <QFile>
#include <QTextStream>
#include <QDir>

#include <DSysInfo>

#include "system_ctl.h"
#include "utils.h"

#define PROC_KERN_LOG_LEVEL     "/proc/sys/kernel/printk"
#define PROC_SYS_CACHE          "/proc/sys/vm/drop_caches"
#define BOOT_SPLASH_MODE        "/boot/grub/grub.cfg"
#define PATH_GRUB_D             "/etc/grub.d"

#define DBUS_S_HOST_NAME        "org.freedesktop.hostname1"
#define DBUS_O_HOST_NAME        "/org/freedesktop/hostname1"
#define DBUS_IF_HOST_NAME       "org.freedesktop.hostname1"

#define DBUS_S_SYSINFO          "com.deepin.system.SystemInfo"
#define DBUS_O_SYSINFO          "/com/deepin/system/SystemInfo"
#define DBUS_IF_SYSINFO         "com.deepin.system.SystemInfo"

DCORE_USE_NAMESPACE

std::atomic<SystemCtl *> SystemCtl::m_instance;
std::mutex SystemCtl::m_mutex;

auto print_err = [](decltype(errno) eno, const QString &msg)
{
    qDebug() << QString("Error: [%1] %2, %3").arg(eno).arg(strerror(eno)).arg(msg);
};

void SystemCtl::getSystemInfo()
{
    auto model = QSharedPointer<SystemInfoModel>::create();

    // TODO: (async)
    struct utsname os {};
    uname(&os);

    // use DSysInfo to get hostname kinda buggy,it wont refresh hostname if modified elsewhere
//    model->setHostName(DSysInfo::computerName());
    // use dbus to get hostname info instead
    QDBusInterface busIf(DBUS_S_HOST_NAME,
                         DBUS_O_HOST_NAME,
                         DBUS_IF_HOST_NAME,
                         QDBusConnection::systemBus());
    auto name = busIf.property("Hostname");
    model->setHostName(name.toString());

    model->setOSVersion(QString("%1 %2")
                        .arg(DSysInfo::deepinVersion())
                        .arg(DSysInfo::deepinTypeDisplayName()));
    model->setOSType(QString("%1").arg(os.machine));
    model->setKernelVersion(QString("%1").arg(os.release));
    model->setCPUModel(QString("%1 x %2")
                       .arg(DSysInfo::cpuModelName())
                       .arg(sysconf(_SC_NPROCESSORS_ONLN)));

    QDBusInterface sysIf(DBUS_S_SYSINFO,
                         DBUS_O_SYSINFO,
                         DBUS_IF_SYSINFO,
                         QDBusConnection::systemBus());
    if (sysIf.isValid()) {
        model->setInstalledMemory(Utils::formatUnit(sysIf.property("MemorySize").toULongLong(), Utils::B, 0));
    } else {
        model->setInstalledMemory(Utils::formatUnit(DSysInfo::memoryInstalledSize(), Utils::B, 0));
    }

    model->setAvailMemory(Utils::formatUnit(DSysInfo::memoryTotalSize()));
    model->setDiskSize(Utils::formatUnit(DSysInfo::systemDiskSize()));

    // log level
    model->setKernelLogLevel(getKernelLogLevel());

    // drop cache depth
    model->setClearCacheDepth(readSysDropCache());

    // laptop mode
    model->setSplashModeEnabled(getSplashModeEnabledStatus());

    // core dump
    model->setCoreDumpEnabled(getCoreDumpEnabledStatus());

    Q_EMIT sysInfoModelUpdated(model);
}

SystemCtl::SystemCtl(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<QSharedPointer<SystemInfoModel>>("SystemInfoModel");
}

// read /proc/sys/kernel/printk
SystemInfoModel::KernelLogLevel SystemCtl::getKernelLogLevel()
{
    int fd;
    const size_t BUFLEN = 128;
    char buf[BUFLEN + 1] {};
    // sys default log level
    SystemInfoModel::KernelLogLevel logLevel {SystemInfoModel::kLogLevelWarning};

    errno = 0;
    if ((fd = open(PROC_KERN_LOG_LEVEL, O_RDONLY)) < 0) {
        print_err(errno, QString("open %1 failed").arg(PROC_KERN_LOG_LEVEL));
        return logLevel;
    }

    auto nr = read(fd, buf, BUFLEN);
    if (nr < 0) {
        print_err(errno, QString("read %1 failed").arg(PROC_KERN_LOG_LEVEL));
    } else {
        buf[nr] = '\0';

        int lvl {};
        nr = sscanf(buf, "%u", &lvl);
        if (nr == 1 && (lvl >= 0 && lvl < SystemInfoModel::KernelLogLevelMax)) {
            logLevel = static_cast<SystemInfoModel::KernelLogLevel>(lvl);
        }
    }
    close(fd);

    return logLevel;
}

// /proc/sys/vm/drop_caches
SystemInfoModel::ClearCacheDepth SystemCtl::readSysDropCache()
{
    int fd;
    const size_t BUFLEN = 16;
    char buf[BUFLEN + 1] {};
    SystemInfoModel::ClearCacheDepth cc {SystemInfoModel::kNoClear};

    errno = 0;
    if ((fd = open(PROC_SYS_CACHE, O_RDONLY)) < 0) {
        print_err(errno, QString("open %1 failed").arg(PROC_SYS_CACHE));
        return cc;
    }

    auto nr = read(fd, buf, BUFLEN);
    if (nr < 0) {
        print_err(errno, QString("read %1 failed").arg(PROC_SYS_CACHE));
    } else {
        buf[nr] = '\0';

        int cv {};
        nr = sscanf(buf, "%u", &cv);
        if (nr == 1 && (cv >= 0 && cv < SystemInfoModel::kClearCacheMax)) {
            cc = static_cast<SystemInfoModel::ClearCacheDepth>(cv);
        }
    }
    close(fd);

    return cc;
}

// /proc/sys/vm/laptop_mode
bool SystemCtl::getSplashModeEnabledStatus()
{
    bool enabled {false};

    QFile file(BOOT_SPLASH_MODE);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << QString("open %1 failed").arg(BOOT_SPLASH_MODE);
        return false;
    }
    QTextStream s(&file);

    auto splash_matched = [&s](const QString & name) -> bool {
        s.seek(0);

        bool block_entered = false;
        bool menuentry_block  = false;
        while (!s.atEnd())
        {
            auto line = s.readLine();
            //
            if (line.contains(name) && line.contains("BEGIN")) {
                block_entered = true;
                continue;
            }
            if (line.contains(name) && line.contains("END")) {
                block_entered = false;
                continue;
            }
            if (block_entered && line.startsWith("menuentry")) {
                menuentry_block = true;
                continue;
            }

            // format: linux /vmlinuz-xxx-xxx-xxx root=xxx ... splash | nosplash ...
            if (menuentry_block && line.contains("vmlinuz")) {
                auto list = line.split(QRegularExpression("\\s+"));
                if (list.contains("splash")) {
                    return true;
                } else if (list.contains("nosplash")) {
                    return false;
                }
            }
        }
        if (s.status() != QTextStream::Ok)
        {
            qDebug() << QString("read %1 failed").arg(BOOT_SPLASH_MODE);
        }
        return false;
    };

    // find default boot entries
    QDir dir(PATH_GRUB_D);
    auto dirList = dir.entryList({"10_*"}, QDir::Files);
    if (dirList.contains("10_linux")) {
        enabled = splash_matched("10_linux");
    } else {
        for (auto &name : dirList) {
            enabled = splash_matched(name);
            if (enabled)
                break;
        }
    }
    file.close();

    return enabled;
}

// /etc/security/limits.conf
bool SystemCtl::getCoreDumpEnabledStatus()
{
    struct rlimit limit;
    errno = 0;
    auto rc = getrlimit(RLIMIT_CORE, &limit);
    if (rc == -1) {
        print_err(errno, QString("call getrlimit with RLIMIT_CORE failed"));
        return false;
    }
    return limit.rlim_cur != 0;
}
