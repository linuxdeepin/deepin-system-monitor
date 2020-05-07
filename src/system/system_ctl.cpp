#include <sys/utsname.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>

#include <QDebug>

#include <DSysInfo>

#include "system_ctl.h"
#include "utils.h"

#define PROC_KERN_LOG_LEVEL     "/proc/sys/kernel/printk"
#define PROC_SYS_CACHE          "/proc/sys/vm/drop_caches"
#define PROC_LAPTOP_MODE        "/proc/sys/vm/laptop_mode"
//#define PROC_CORE_DUMP          "/etc/security/limits.conf"

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

    model->setHostName(DSysInfo::computerName());
    model->setOSVersion(QString("%1 %2")
                        .arg(DSysInfo::deepinVersion())
                        .arg(DSysInfo::deepinTypeDisplayName()));
    model->setOSType(QString("%1").arg(os.machine));
    model->setKernelVersion(QString("%1").arg(os.release));
    model->setCPUModel(QString("%1 x %2")
                       .arg(DSysInfo::cpuModelName())
                       .arg(sysconf(_SC_NPROCESSORS_ONLN)));
    model->setMemory(QString("%1 (%2 available)") //TODO: translate
                     .arg(Utils::formatUnit(DSysInfo::memoryInstalledSize()))
                     .arg(Utils::formatUnit(DSysInfo::memoryTotalSize())));
    model->setDiskSize(Utils::formatUnit(DSysInfo::systemDiskSize()));

    // log level
    model->setKernelLogLevel(getKernelLogLevel());

    // drop cache depth
    model->setClearCacheDepth(readSysDropCache());

    // laptop mode
    model->setLaptopModeEnabled(getLaptopModeEnabledStatus());

    // core dump
    model->setCoreDumpEnabled(getCoreDumpEnabledStatus());

    Q_EMIT systemInfoModelReady(model);
}

void SystemCtl::changeHostName(const QString &name)
{
    // check name validaty
    // ref: https://www.freedesktop.org/wiki/Software/systemd/hostnamed/
    // dbus: org.freedesktop.hostname1 -> /org/freedesktop/hostname1 -> SetHostname sb (sb: nane & user_interaction)
    // hostnamectl set-hostname
    // /etc/hostname
}

void SystemCtl::changeKernelLogLevel(SystemInfoModel::KernelLogLevel level)
{
    // write /proc/sys/kernel/printk
    // /etc/sysctl.conf & /etc/sysctl.d/*.conf
}

void SystemCtl::dropCache(SystemInfoModel::ClearCacheDepth depth)
{
    // write(stdout, depth) | pkexec tee PATH_SYS_CACHE
}

void SystemCtl::setLaptopModeEnabled(bool enabled)
{
    // /etc/laptop-mode/laptop-mode.conf
    // write(stdout, enabled) | pkexec tee PATH_LAPTOP_MODE
}

void SystemCtl::setCoreDumpEnabled(bool enabled)
{
    // write(stdout, enabled) | pkexec tee PATH_CORE_DUMP
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
bool SystemCtl::getLaptopModeEnabledStatus()
{
    int fd;
    const size_t BUFLEN = 16;
    char buf[BUFLEN + 1] {};
    bool enabled {false};

    errno = 0;
    if ((fd = open(PROC_LAPTOP_MODE, O_RDONLY)) < 0) {
        print_err(errno, QString("open %1 failed").arg(PROC_LAPTOP_MODE));
        return enabled;
    }

    auto nr = read(fd, buf, BUFLEN);
    if (nr < 0) {
        print_err(errno, QString("read %1 failed").arg(PROC_LAPTOP_MODE));
    } else {
        buf[nr] = '\0';

        int va {};
        nr = sscanf(buf, "%u", &va);
        if (nr == 1) {
            enabled = (va > 0);
        }
    }
    close(fd);

    return enabled;
}

// /etc/security/limits.conf
bool SystemCtl::getCoreDumpEnabledStatus()
{
    struct rlimit limit {};
    errno = 0;
    auto rc = getrlimit(RLIMIT_CORE, &limit);
    if (rc < 0) {
        print_err(errno, QString("call getrlimit with RLIMIT_CORE failed"));
        return false;
    }
    return limit.rlim_cur > 0;
}
