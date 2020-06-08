#include <unistd.h>

#include <QCoreApplication>
#include <QDebug>
#include <QtDBus>
#include <QProcess>
#include <QByteArray>
#include <QJsonDocument>
#include <QCommandLineParser>
#include <QFile>
#include <QDir>

#define CONF_HOST_NAME          "/etc/hostname"
#define PROC_HOST_NAME          "/proc/sys/kernel/hostname"

#define PROC_KERN_LOG_LEVEL     "/proc/sys/kernel/printk"
#define PROC_SYS_CACHE          "/proc/sys/vm/drop_caches"

#define CONF_SYSCTL             "/etc/sysctl.conf"
#define PATH_SYSCTL_D           "/etc/sysctl.d"
#define CONF_PRINTK_FILE        "kernel_printk.conf"

#define CONF_LAPTOP_MODE        "/etc/laptop-mode/laptop-mode.conf"
#define PROC_LAPTOP_MODE        "/proc/sys/vm/laptop_mode"

#define CONF_CORE_DUMP          "/etc/security/limits.conf"
#define PATH_LIMITS_D           "/etc/security/limits.d"
#define FMT_LIMIT_CORE          "%1			%2	core	%3"
#define FMT_CONF_LIMIT_FILE     "limit_core_%1.conf"
#define FMT_KERN_CORE_FILE      "kern_core.conf"
#define PROC_CORE_PATTERN       "/proc/sys/kernel/core_pattern"
#define PROC_CORE_USES_PID      "/proc/sys/kernel/core_uses_pid"

#define PATH_GRUB_CFG_CONF      "/etc/default/grub.d/10_deepin.cfg"
#define PATH_GRUB_CFG           "/boot/grub/grub.cfg"

#define DBUS_S_HOST_NAME        "org.freedesktop.hostname1"
#define DBUS_O_HOST_NAME        "/org/freedesktop/hostname1"
#define DBUS_IF_HOST_NAME       "org.freedesktop.hostname1"

#define DBUS_S_SYSTEMD          "org.freedesktop.systemd1"
#define DBUS_O_SYSTEMD          "/org/freedesktop/systemd1"
#define DBUS_IF_MANAGER         "org.freedesktop.systemd1.Manager"
#define DBUS_JOB_MODE           "replace"

#define BIN_SED                 "/usr/bin/sed"

bool write_data(const QString &path, const QByteArray &buf)
{
    bool ok {true};

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QTextStream(stderr) << QString("open %1 failed").arg(path);
        return false;
    }

    QTextStream out(&file);
    if (buf.isEmpty()) {
        out << '\0';
    } else {
        out << buf;
    }
    ok = ok && (out.status() == QTextStream::Ok);
    if (ok)
        file.flush();
    file.close();

    return ok;
}

bool write_int(const QString &path, int value)
{
    bool ok {true};

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QTextStream(stderr) << QString("open %1 failed").arg(path);
        return false;
    }

    QTextStream out(&file);
    out << value;
    ok = ok && (out.status() == QTextStream::Ok);
    if (ok)
        file.flush();
    file.close();

    return ok;
}

bool changeHostname(const QString &name)
{
    bool ok {true};
    // check name validaty
    // ref: https://www.freedesktop.org/wiki/Software/systemd/hostnamed/
    QDBusInterface busIf(DBUS_S_HOST_NAME,
                         DBUS_O_HOST_NAME,
                         DBUS_IF_HOST_NAME,
                         QDBusConnection::systemBus());
    QDBusMessage reply {};
    // signature: sb
    // s: name
    // b: needs user interaction?
    reply = busIf.call("SetHostname", name, true);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        QTextStream(stderr) << "DBus call SetHostname failed";

        // modify kernel hostname
        ok = ok && write_data(PROC_HOST_NAME, name.toLocal8Bit());
    }
    reply = busIf.call("SetStaticHostname", name, true);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        QTextStream(stderr) << "DBus call SetStaticHostname failed";

        // modify /etc/hostname
        ok = ok && write_data(CONF_HOST_NAME, name.toLocal8Bit());
    }
    return ok;
}

bool changeKernelLogLevel(int level)
{
    bool ok {true};
    ok = ok && write_int(PROC_KERN_LOG_LEVEL, level);

    auto comment_conf = [](const QString & path) -> bool {
        const char *tag = "kernel\\.printk";
        auto pattern = QString("/%1.*/s/#*kernel/#kernel/").arg(tag);
        if (QFileInfo::exists(BIN_SED))
        {
            auto rc = QProcess::execute(BIN_SED, {"-i", "-e", pattern, path});
            return !rc;
        } else
        {
            // TODO: unexpected case, 'sed' utility not installed on the system
        }
        return true;
    };

    auto new_conf = [level](const QString & path) -> bool {
        bool ok {true};

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
        {
            QTextStream(stderr) << QString("open %1 failed").arg(path) << endl;
            return false;
        }

        QTextStream out(&file);
        // default_message_loglevel, minimum_console_loglevel, default_console_loglevel
        const int defmlvl {4}, minclvl {1}, defclvl {7};
        auto cfg = QString("kernel.printk= %1 %2 %3 %4").arg(level).arg(defmlvl).arg(minclvl).arg(defclvl);
        out << cfg;
        ok = ok && (out.status() == QTextStream::Ok);
        if (ok)
            out.flush();
        file.close();

        return ok;
    };

    // comment out kernel.printk in sysctl.conf
    ok = ok && comment_conf(CONF_SYSCTL);

    QDir dir(PATH_SYSCTL_D);
    if (!dir.exists()) {
        if (!QDir::root().mkpath(PATH_SYSCTL_D)) {
            QTextStream(stderr) << QString("create dir %1 failed").arg(PATH_SYSCTL_D) << endl;
            return false;
        }
        dir.refresh();
    }

    auto elist = dir.entryInfoList({"*.conf"}, QDir::Files);
    for (auto einfo : elist) {
        if (einfo.fileName() == CONF_PRINTK_FILE) {
            dir.remove(CONF_PRINTK_FILE);
            continue;
        }
        ok = ok && comment_conf(einfo.absoluteFilePath());
    }

    ok = ok && new_conf(dir.absoluteFilePath(CONF_PRINTK_FILE));

    return ok;
}

bool dropCache(int type)
{
    return write_int(PROC_SYS_CACHE, type);
}

// laptop-mode.conf file always reset back to its default state by the system on boot,
// no idea what cause this issue to happen. so tempalory we disable this feature for now.
#if 0
bool setLaptopModeEnabled(bool enabled)
{
    bool ok {true};

    int enableLaptopModeTools;
    int enableLaptopModeOnBattery;
    int enableLaptopModeOnAC;
    int enableLaptopModeWhenLidClosed;
    if (enabled) {
        enableLaptopModeTools = 1;
        enableLaptopModeOnBattery = 1;
        enableLaptopModeOnAC = 1;
        enableLaptopModeWhenLidClosed = 1;
    } else {
        enableLaptopModeTools = 1;
        enableLaptopModeOnBattery = 0;
        enableLaptopModeOnAC = 0;
        enableLaptopModeWhenLidClosed = 0;
    }

    QStringList args;
    args << "-i"
         << "-e"
         << QString("s/ENABLE_LAPTOP_MODE_TOOLS.*/ENABLE_LAPTOP_MODE_TOOLS=%1/g").arg(enableLaptopModeTools)
         << "-e"
         << QString("s/ENABLE_LAPTOP_MODE_ON_BATTERY.*/ENABLE_LAPTOP_MODE_ON_BATTERY=%1/g").arg(enableLaptopModeOnBattery)
         << "-e"
         << QString("s/ENABLE_LAPTOP_MODE_ON_AC.*/ENABLE_LAPTOP_MODE_ON_AC=%1/g").arg(enableLaptopModeOnAC)
         << "-e"
         << QString("s/ENABLE_LAPTOP_MODE_WHEN_LID_CLOSED.*/ENABLE_LAPTOP_MODE_WHEN_LID_CLOSED=%1/g").arg(enableLaptopModeWhenLidClosed)
         << CONF_LAPTOP_MODE;
    // TODO: check 'sed' utility existense
    auto rc = QProcess::execute(BIN_SED, args);
    if (!rc) {
        // reload unit with dbus call
        QDBusInterface busIf(DBUS_S_SYSTEMD,
                             DBUS_O_SYSTEMD,
                             DBUS_IF_MANAGER,
                             QDBusConnection::systemBus());
        QDBusMessage reply {};
        // signature: ss
        // s: unit
        // s: job mode
        reply = busIf.call("ReloadUnit", "laptop-mode.service", DBUS_JOB_MODE);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            QTextStream(stderr) << "DBus call ReloadUnit failed";
            ok = false;
        } else {
            // modify proc param manually (which might not work as expected)
            ok = ok && write_int(PROC_LAPTOP_MODE, enabled ? 1 : 0);
        }
    }
    return ok;
}
#endif

bool setSplashModeEnabled(bool enabled)
{
    bool ok {true};

    // check grub-mkconfig existence
    if (QFileInfo::exists("/usr/sbin/grub-mkconfig")) {
        QFile file(PATH_GRUB_CFG_CONF);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << QString("open %1 failed").arg(PATH_GRUB_CFG_CONF);
            return !ok;
        }

        QTextStream s(&file);
        QString buffer {};
        if (enabled) {
            buffer.append("GRUB_CMDLINE_LINUX_DEFAULT=\"$GRUB_CMDLINE_LINUX_DEFAULT DEEPIN_GFXMODE=\\$DEEPIN_GFXMODE splash\"");
        } else {
            buffer.append("GRUB_CMDLINE_LINUX_DEFAULT=\"nosplash\"");
        }
        s << buffer;
        s.flush();
        if (s.status() != QTextStream::Ok) {
            qDebug() << QString("write %1 failed").arg(PATH_GRUB_CFG_CONF);
            ok = !ok;
        }
        file.close();

        if (ok) {
            // update grub
            auto rc = QProcess::execute("/usr/bin/sh", {"/usr/sbin/update-grub"});
            ok = ok && !rc;
        }
    } else {
        // need manual edit grub.cfg file
        QTemporaryFile tmp("/boot/grub/grub.cfg.XXXXXX");
        if (tmp.open()) {
            QTextStream stmp(&tmp);
            QFile cfg(PATH_GRUB_CFG);
            if (cfg.open(QIODevice::ReadWrite | QIODevice::Text)) {
                QTextStream scfg(&cfg);

                QString buffer {};
                bool menuentry_block {false};
                while (!scfg.atEnd()) {
                    buffer = scfg.readLine().trimmed();

                    if (buffer.startsWith("menuentry") && buffer.contains("gnu-linux")) {
                        menuentry_block = true;
                    }

                    if (menuentry_block && buffer.contains("vmlinuz") && buffer.contains("root=UUID=")) {
                        // replace splash/nosplash

                        if (enabled) {
                            buffer = buffer.replace("nosplash", "splash");
                        } else {
                            buffer = buffer.replace("splash", "nosplash");
                        }
                    }

                    stmp << buffer;
                }
                cfg.close();

                stmp.flush();
                stmp.seek(0);

                cfg.open(QIODevice::WriteOnly | QIODevice::Text);
                scfg.setDevice(&cfg);
                scfg.seek(0);

                // write modified contents back to the original cfg file
                while (!stmp.atEnd()) {
                    buffer = stmp.readLine();
                    scfg << buffer;
                }

                scfg.flush();
                cfg.close();

            } else {
                ok = !ok;
            }
        } else {
            ok = !ok;
        }
    }

    return ok;
}

// only modify limits for the current user
int setCoreDumpEnabled(bool enabled)
{
    bool ok {true};
    const char *user = getlogin();
    auto cfgFile = QString(FMT_CONF_LIMIT_FILE).arg(user);

    auto comment_conf = [](const QString & path, const QStringList & patternList) -> bool {
        // comment out
        if (QFileInfo::exists(BIN_SED))
        {
            QStringList params;
            params << "-i";
            for (auto &pa : patternList) {
                params << "-e";
                params << pa;
            }
            params << path;
            auto rc = QProcess::execute(BIN_SED, params);
            return !rc;
        } else
        {
            // TODO: unexpected case, 'sed' utility not installed on the system
        }
        return true;
    };
    auto new_conf = [](const QString & file, const QStringList & lines) -> bool {
        bool ok {true};
        QFile conf(file);
        if (!conf.open(QIODevice::WriteOnly))
        {
            QTextStream(stdout) << QString("open %1 failed").arg(file) << endl;
            return false;
        }
        QTextStream stream(&conf);
        for (auto &line : lines)
        {
            stream << line << endl;
        }
        ok = ok && (stream.status() == QTextStream::Ok);
        if (ok)
            stream.flush();
        conf.close();
        return ok;
    };

    // edit /etc/security/limits.conf
    QStringList patternList;
    patternList << QString("/%1.*core.*/s/#*%1/#%1/").arg(user);
    ok = ok && comment_conf(CONF_CORE_DUMP, patternList);
    // scan /etc/security/limits.d/*.conf
    QDir dir(PATH_LIMITS_D);
    if (!dir.exists()) {
        auto ok = QDir::root().mkpath(PATH_LIMITS_D);
        if (!ok) {
            QTextStream(stderr) << QString("create dir %1 failed").arg(PATH_LIMITS_D);
            return EXIT_FAILURE;
        }
        dir.refresh();
    }

    auto entries = dir.entryInfoList({"*.conf"}, QDir::Files);
    for (auto einfo : entries) {
        if (einfo.fileName() == cfgFile) {
            // remove old conf file (limit_core_xxx.conf, where xxx matches current login name)
            dir.remove(cfgFile);
            continue;
        }
        // comment out all current user's settings from other conf file
        ok = ok && comment_conf(einfo.absoluteFilePath(), patternList);
    }
    auto slimTxt = QString(FMT_LIMIT_CORE).arg(user).arg("soft").arg(enabled ? -1 : 0);
    auto hlimTxt = QString(FMT_LIMIT_CORE).arg(user).arg("hard").arg(enabled ? -1 : 0);
    QStringList lines;
    lines << slimTxt;
    lines << hlimTxt;
    ok = ok && new_conf(dir.absoluteFilePath(cfgFile), lines);

    // edit /etc/sysctl.conf & /etc/sysctl.d/*.conf, comment out "kernel.core_pattern" && "kernel.core_uses_pid"
    patternList.clear();
    patternList << "/kernel\\.core_pattern/s/#*kernel\\.core_pattern/#kernel.core_pattern/";
    patternList << "/kernel\\.core_uses_pid/s/#*kernel\\.core_uses_pid/#kernel.core_uses_pid/";
    ok = ok && comment_conf(CONF_SYSCTL, patternList);

    dir = QDir(PATH_SYSCTL_D);
    if (!dir.exists()) {
        auto ok = QDir::root().mkpath(PATH_SYSCTL_D);
        if (!ok) {
            QTextStream(stderr) << QString("create dir %1 failed").arg(PATH_SYSCTL_D);
            return EXIT_FAILURE;
        }
        dir.refresh();
    }

    entries = dir.entryInfoList({"*.conf"}, QDir::Files);
    for (auto einfo : entries) {
        if (einfo.fileName() == FMT_KERN_CORE_FILE) {
            // remove /etc/sysctl.d/kern_core.conf
            dir.remove(FMT_KERN_CORE_FILE);
            continue;
        }
        ok = ok && comment_conf(einfo.absoluteFilePath(), patternList);
    }
    // new /etc/sysctl.d/kern_core.conf
    lines.clear();
    lines << QString("kernel.core_pattern=%1").arg(enabled ? "core" : "");
    lines << QString("kernel.core_uses_pid=%1").arg(enabled ? 1 : 0);
    ok = ok && new_conf(dir.absoluteFilePath(FMT_KERN_CORE_FILE), lines);
    ok = ok && write_data(PROC_CORE_PATTERN, enabled ? "core" : "");
    ok = ok && write_int(PROC_CORE_USES_PID, enabled ? 1 : 0);

    return ok;
}

// json arg format (field may not exist)
// {
//     "Hostname": "xxxx"
//     "KernelLogLevel": "1"
//     "DropCache": "1"
//     "EnableSplashMode": true
//     "EnableCoreDump": true
// }
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.process(app);
    auto args = parser.positionalArguments();
    if (args.size() < 1) {
        qDebug() << "parse command args failed";
        return 1;
    }
    auto raw = args[0].toLocal8Bit();
    auto buf = QByteArray::fromBase64(raw);
    auto doc = QJsonDocument::fromBinaryData(buf);
    if (doc.isNull() || doc.isEmpty() || !doc.isObject()) {
        QTextStream(stderr) << "parse command args failed";
        return 1;
    }
    auto jo = doc.object();
    QJsonValue jva {};
    bool ok {true};
    for (auto it = jo.constBegin(); it != jo.constEnd(); ++it) {
        jva = it.value();
        if (it.key() == "Hostname") {
            if (jva.isString()) {
                auto name = jva.toString();
                if (!name.isEmpty()) {
                    ok = ok && changeHostname(name);
                }
            }
        } else if (it.key() == "KernelLogLevel") {
            if (jva.isString()) {
                auto lvl = jva.toString();
                if (!lvl.isEmpty()) {
                    bool ok {};
                    auto level = lvl.toInt(&ok);
                    if (ok) {
                        ok = ok && changeKernelLogLevel(level);
                    }
                }
            }
        } else if (it.key() == "DropCache") {
            if (jva.isString()) {
                auto cc = jva.toString();
                if (!cc.isEmpty()) {
                    bool ok {};
                    auto type = cc.toInt(&ok);
                    if (ok) {
                        ok = ok && dropCache(type);
                    }
                }
            }
        } else if (it.key() == "EnableSplashMode") {
            if (jva.isBool()) {
                auto enabled = jva.toBool();
                ok = ok && setSplashModeEnabled(enabled);
            }
        } else if (it.key() == "EnableCoreDump") {
            if (jva.isBool()) {
                auto enabled = jva.toBool();
                ok = ok && setCoreDumpEnabled(enabled);
            }
        }
    }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
