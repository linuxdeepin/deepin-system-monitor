#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QtDBus>

#include <DApplication>
#include <DLog>

#include "dbus/dbus_common.h"
#include "dbus/dbus_properties_interface.h"
#include "dbus/environment_file.h"
#include "dbus/systemd1_manager_interface.h"
#include "dbus/systemd1_service_interface.h"
#include "dbus/systemd1_unit_interface.h"
#include "dbus/unit_file_info.h"
#include "dbus/unit_info.h"
#include "service/system_service_entry.h"
#include "service_manager.h"

#define BIN_PKEXEC_PATH "/usr/bin/pkexec"
#define BIN_SYSTEMCTL_PATH "/usr/bin/systemctl"

DCORE_USE_NAMESPACE
using namespace DBus::Common;

#define DBUS_SYSTEMD1_SERVICE "org.freedesktop.systemd1"
const QDBusObjectPath kSystemDObjectPath {"/org/freedesktop/systemd1"};

std::atomic<ServiceManager *> ServiceManager::m_instance;
std::mutex ServiceManager::m_mutex;

ServiceManager::ServiceManager(QObject *parent)
    : QObject(parent)
{
    UnitInfo::registerMetaType();
    UnitFileInfo::registerMetaType();
    EnvironmentFile::registerMetaType();
}

QPair<ErrorContext, QList<SystemServiceEntry>> ServiceManager::getServiceEntryList() const
{
    QList<SystemServiceEntry> list;
    QHash<QString, SystemServiceEntry> hash;
    ErrorContext ec;

    Systemd1ManagerInterface mgrIf(DBUS_SYSTEMD1_SERVICE, kSystemDObjectPath.path(),
                                   QDBusConnection::systemBus());

    auto unitFilesResult = mgrIf.ListUnitFiles();
    ec = unitFilesResult.first;
    if (ec) {
        qDebug() << "ListUnitFiles failed:" << ec.getErrorName() << ec.getErrorMessage();
    }
    UnitFileInfoList unitFiles = unitFilesResult.second;

    auto unitsResult = mgrIf.ListUnits();
    ec = unitsResult.first;
    if (ec) {
        qDebug() << "ListUnits failed:" << ec.getErrorName() << ec.getErrorMessage();
    }
    UnitInfoList units = unitsResult.second;

    foreach (const auto &unit, units) {
        if (!unit.getName().endsWith(UnitTypeServiceSuffix))
            continue;

        SystemServiceEntry entry {};
        // Id
        entry.setId(unit.getName().left(unit.getName().lastIndexOf('.')));
        // LoadState
        entry.setLoadState(unit.getLoadState());
        // ActiveState
        entry.setActiveState(unit.getActiveState());
        // subState
        entry.setSubState(unit.getSubState());
        entry.setUnitObjectPath(unit.getUnitObjectPath());
        // Description
        entry.setDescription(unit.getDescription());
        auto oResult = mgrIf.GetUnit(unit.getName());
        ec = oResult.first;
        if (ec) {
            qDebug() << "GetUnit failed:" << unit.getName() << ec.getErrorName()
                     << ec.getErrorMessage();
            continue;
        }
        QDBusObjectPath o = oResult.second;
        Systemd1UnitInterface unitIf(DBUS_SYSTEMD1_SERVICE, o.path(), QDBusConnection::systemBus());
        auto bResult = unitIf.canStart();
        ec = bResult.first;
        if (ec) {
            qDebug() << "call canStart failed:" << ec.getErrorName() << ec.getErrorMessage();
            continue;
        }
        entry.setCanStart(bResult.second);
        bResult = unitIf.canStop();
        ec = bResult.first;
        if (ec) {
            qDebug() << "call canStart failed:" << ec.getErrorName() << ec.getErrorMessage();
            continue;
        }
        entry.setCanStop(bResult.second);
        bResult = unitIf.canReload();
        ec = bResult.first;
        if (ec) {
            qDebug() << "call canStart failed:" << ec.getErrorName() << ec.getErrorMessage();
            continue;
        }
        entry.setCanReload(bResult.second);
        // mainPID
        Systemd1ServiceInterface svcIf(DBUS_SYSTEMD1_SERVICE, o.path(),
                                       QDBusConnection::systemBus());
        auto pidResult = svcIf.getMainPID();
        ec = pidResult.first;
        if (ec) {
            qDebug() << "getMainPID failed" << ec.getErrorName() << ec.getErrorMessage();
            continue;
        }
        entry.setMainPID(pidResult.second);

        // getunitfilestate
        auto stateResult = mgrIf.GetUnitFileState(unit.getName());
        ec = stateResult.first;
        if (ec) {
            qDebug() << "getUnitFileState failed" << ec.getErrorName() << ec.getErrorMessage();
            if (ec.getCode() != QDBusError::NoMemory) {
                continue;
            }
        }
        entry.setState(stateResult.second);

        hash[unit.getName()] = entry;
        list << entry;
    }

    foreach (const UnitFileInfo &u, unitFiles) {
        // filter out non-service type units
        if (!(u.getName().endsWith(UnitTypeServiceSuffix)))
            continue;

        QString name = u.getName().mid(u.getName().lastIndexOf('/') + 1);
        if (hash.contains(name)) {
            hash[name].setState(u.getStatus());
            continue;
        }

        SystemServiceEntry entry {};
        // SET id
        entry.setId(name.left(name.lastIndexOf('.')));
        // SET state
        entry.setState(u.getStatus());
        list << entry;

        auto oResult = mgrIf.GetUnit(name);
        ec = oResult.first;
        if (ec && !(ec.getSubCode() == QDBusError::Other)) {
            qDebug() << "GetUnit failed:" << name << ec.getErrorName() << ec.getErrorMessage();
            continue;
        } else {
            ec.reset();
        }
        QDBusObjectPath o = oResult.second;
        if (!o.path().isEmpty()) {  // TODO: check ec subCode
            Systemd1UnitInterface unitIf(DBUS_SYSTEMD1_SERVICE, o.path(),
                                         QDBusConnection::systemBus());
            Systemd1ServiceInterface svcIf(DBUS_SYSTEMD1_SERVICE, o.path(),
                                           QDBusConnection::systemBus());

            auto descResult = unitIf.getDescription();
            ec = descResult.first;
            if (ec) {
                qDebug() << "getDescription failed:" << ec.getErrorName() << ec.getErrorMessage();
                continue;
            }
            entry.setDescription(descResult.second);

            auto actStateResult = unitIf.getActiveState();
            ec = actStateResult.first;
            if (ec) {
                qDebug() << "getActiveState failed:" << ec.getErrorName() << ec.getErrorMessage();
                continue;
            }
            entry.setActiveState(actStateResult.second);

            auto loadStateResult = unitIf.getLoadState();
            ec = loadStateResult.first;
            if (ec) {
                qDebug() << "getLoadState failed:" << ec.getErrorName() << ec.getErrorMessage();
                continue;
            }
            entry.setLoadState(loadStateResult.second);

            auto subStateResult = unitIf.getSubState();
            ec = subStateResult.first;
            if (ec) {
                qDebug() << "getSubState failed:" << ec.getErrorName() << ec.getErrorMessage();
                continue;
            }
            entry.setSubState(subStateResult.second);

            auto mainPIDResult = svcIf.getMainPID();
            ec = mainPIDResult.first;
            if (ec) {
                qDebug() << "getMainPID failed:" << ec.getErrorName() << ec.getErrorMessage();
                continue;
            }
            entry.setMainPID(mainPIDResult.second);
        } else {
            QFile f(u.getName());
            if (!f.open(QFile::ReadOnly | QFile::Text)) {
                qDebug() << "Open file failed:" << u.getName();
                continue;
            }

            QTextStream ins(&f);
            QString line = ins.readLine();
            while (!ins.atEnd()) {
                QRegularExpressionMatch match;
                if (line.contains(
                            QRegularExpression("^\\s*(Description)\\s*\\=\\s*(.+)\\s*$",
                                               QRegularExpression::CaseInsensitiveOption |
                                               QRegularExpression::ExtendedPatternSyntaxOption),
                            &match)) {
                    if (match.hasMatch())
                        entry.setDescription(match.captured(2).trimmed());
                }
                line = ins.readLine();
            }
            f.close();
        }
    }

    return {{}, list};
}

QString ServiceManager::normalizeServiceId(const QString &id, const QString &param)
{
    QString buf = id;
    if (buf.endsWith(UnitTypeServiceSuffix)) {
        if (buf.lastIndexOf('@') > 0) {
            if (!param.isEmpty())
                buf = QString("%1%2%3")
                      .arg(buf.left(buf.lastIndexOf('@') + 1))
                      .arg(param)
                      .arg(UnitTypeServiceSuffix);
        }
    } else {
        if (buf.endsWith('@') && !param.isEmpty()) {
            buf = QString("%1%2%3").arg(buf).arg(param).arg(UnitTypeServiceSuffix);
        } else {
            buf = buf.append(UnitTypeServiceSuffix);
        }
    }

    return buf;
}

QPair<ErrorContext, bool> ServiceManager::startService(SystemServiceEntry &entry,
                                                       const QString &param)
{
    ErrorContext ec;
    Systemd1ManagerInterface iface(DBUS_SYSTEMD1_SERVICE, kSystemDObjectPath.path(),
                                   QDBusConnection::systemBus());

    QString buf = normalizeServiceId(entry.getId(), param);
    QString mode = UnitControlJobModeMap.value(kJobModeReplace);

    auto oResult = iface.StartUnit(buf, mode);
    ec = oResult.first;
    if (ec) {
        qDebug() << "StartUnit failed:" << buf << ec.getErrorName() << ec.getErrorMessage();
        return {ec, false};
    }
    QDBusObjectPath o = oResult.second;
    qDebug() << "object path:" << o.path();

    if (entry.getId().endsWith("@"))
        return {ec, true};

    oResult = iface.GetUnit(buf);
    ec = oResult.first;
    if (ec) {
        qDebug() << "GetUnit failed:" << buf << ec.getErrorName() << ec.getErrorMessage();
        return {ec, false};
    }
    o = oResult.second;
    updateServiceEntry(entry, o);

    return {ec, true};
}

QPair<ErrorContext, bool> ServiceManager::stopService(SystemServiceEntry &entry)
{
    ErrorContext ec;
    Systemd1ManagerInterface iface(DBUS_SYSTEMD1_SERVICE, kSystemDObjectPath.path(),
                                   QDBusConnection::systemBus());

    QString buf = normalizeServiceId(entry.getId());
    QString mode = UnitControlJobModeMap.value(kJobModeReplace);

    auto oResult = iface.StopUnit(buf, mode);
    ec = oResult.first;
    if (ec) {
        qDebug() << "Stop Unit failed:" << ec.getErrorName() << ec.getErrorMessage();
        return {ec, false};
    }
    QDBusObjectPath o = oResult.second;
    qDebug() << "object path:" << o.path();

    if (buf.contains(QRegularExpression("^[^\\.@]+@[^\\.@]*\\.service$",
                                        QRegularExpression::CaseInsensitiveOption))) {
        return {ec, true};
    }

    if (entry.getId().endsWith("@"))
        return {ec, true};

    oResult = iface.GetUnit(buf);
    ec = oResult.first;
    if (ec) {
        qDebug() << "Get Unit failed:" << ec.getErrorName() << ec.getErrorMessage();
        return {ec, false};
    }
    o = oResult.second;
    updateServiceEntry(entry, o);

    return {ec, true};
}

QPair<ErrorContext, bool> ServiceManager::restartService(SystemServiceEntry &entry,
                                                         const QString &param)
{
    ErrorContext ec;
    Systemd1ManagerInterface iface(DBUS_SYSTEMD1_SERVICE, kSystemDObjectPath.path(),
                                   QDBusConnection::systemBus());

    QString buf = normalizeServiceId(entry.getId(), param);
    QString mode = UnitControlJobModeMap.value(kJobModeReplace);

    auto oResult = iface.RestartUnit(buf, mode);
    ec = oResult.first;
    if (ec) {
        qDebug() << "Restart Unit failed:" << buf << ec.getErrorName() << ec.getErrorMessage();
        return {ec, false};
    }
    QDBusObjectPath o = oResult.second;
    qDebug() << "object path:" << o.path();

    if (entry.getId().endsWith("@"))
        return {};

    oResult = iface.GetUnit(buf);
    ec = oResult.first;
    if (ec) {
        qDebug() << "Get Unit failed:" << buf << ec.getErrorName() << ec.getErrorMessage();
        return {ec, false};
    }
    o = oResult.second;
    updateServiceEntry(entry, o);

    return {ec, true};
}

void ServiceManager::setServiceStartupMode(const QString &service, bool autoStart)
{
    ErrorContext ec {};

#ifdef USE_POLICYKIT1_AUTHORITy_API
    // check sysv script

    // enable/disable with EnableUnitFiles/DisableUnitFiles
    Systemd1ManagerInterface mgrIf(DBUS_SYSTEMD1_SERVICE, kSystemDObjectPath.path(),
                                   QDBusConnection::systemBus());
    if (autoStart) {
        auto re = mgrIf.EnableUnitFiles({entry.getId()});
        ec = re.first;
        if (ec) {
            qDebug() << "call EnableUnitFiles failed:" << ec.getErrorName() << ec.getErrorMessage();
            return ec;
        }
        auto eResult = re.second;
    } else {
        auto re = mgrIf.DisableUnitFiles({entry.getId()});
        ec = re.first;
        if (ec) {
            qDebug() << "call DisableUnitFiles failed:" << ec.getErrorName() << ec.getErrorMessage();
            return ec;
        }
        auto dResult = re.second;
    }
#else
    auto errfmt = [ = ](ErrorContext & pe, decltype(errno) err, const QString & title, const QString &message = {}) -> ErrorContext & {
        pe.setCode(ErrorContext::kErrorTypeSystem);
        pe.setSubCode(err);
        pe.setErrorName(title);
        auto errmsg = (err != 0 ? QString("Error: [%1] %2").arg(err).arg(strerror(err)) : QString("Error: "));
        if (!message.isEmpty())
        {
            errmsg = QString("%1 - %2").arg(errmsg).arg(message);
        }
        pe.setErrorMessage(errmsg);
        return pe;
    };
    const QString title = QApplication::translate("Service.Action.Set.Startup.Mode", "Failed to set service startup type");
    int rc {};
    struct stat sbuf;

    // check pkexec existence
    errno = 0;
    sbuf = {};
    rc = stat(BIN_PKEXEC_PATH, &sbuf);
    if (rc == -1) {
        qDebug() << "check pkexec existence failed";
        ec = errfmt(ec, errno, title, BIN_PKEXEC_PATH);
        Q_EMIT errorOccurred(ec);
        return;
    }

    // check systemctl existence
    errno = 0;
    sbuf = {};
    rc = stat(BIN_SYSTEMCTL_PATH, &sbuf);
    if (rc == -1) {
        qDebug() << "check systemctl existence failed";
        ec = errfmt(ec, errno, title, BIN_SYSTEMCTL_PATH);
        Q_EMIT errorOccurred(ec);
        return;
    }

    auto *proc = new QProcess();
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
    [ = ](int exitCode, QProcess::ExitStatus exitStatus) {
        ErrorContext le;
        // exitStatus:
        //      crashed
        //
        // exitCode:
        //      127 (pkexec) - not auth/cant auth/error
        //      126 (pkexec) - auth dialog dismissed
        //      0 (systemctl) - ok
        //      !0 (systemctl) - systemctl error, read stdout from child process
        if (exitStatus == QProcess::CrashExit) {
            errno = 0;
            le = errfmt(le, errno, title, QApplication::translate("Service.Action.Set.Startup.Mode",
                                                                  "Error: Failed to set service startup type due to the crashed sub process."));
            Q_EMIT errorOccurred(le);
        } else {
            if (exitCode == 127 || exitCode == 126) {
                errno = EPERM;
                le = errfmt(le, errno, title);
                Q_EMIT errorOccurred(le);
            } else if (exitCode != 0) {
                auto buf = proc->readAllStandardOutput();
                errno = 0;
                le = errfmt(le, errno, title, buf);
                Q_EMIT errorOccurred(le);
            } else {
                // success - refresh service stat -send signal

                // special case, do nothing there
                if (service.endsWith("@"))
                    return;

                SystemServiceEntry entry {};
                entry.setId(service);

                Systemd1ManagerInterface mgrIf(DBUS_SYSTEMD1_SERVICE, kSystemDObjectPath.path(),
                                               QDBusConnection::systemBus());
                auto buf = normalizeServiceId(service, {});
                auto re = mgrIf.GetUnit(buf);
                le = re.first;
                if (le) {
                    Q_EMIT errorOccurred(le);
                } else {
                    updateServiceEntry(entry, re.second);
                    Q_EMIT errorOccurred(le);
                    Q_EMIT serviceStartupModeChanged(service, entry.getState());
                }
            }
        }
        proc->deleteLater();
    });

    // {BIN_PKEXEC_PATH} {BIN_SYSTEMCTL_PATH} {enable/disable} {service}
    QString action = autoStart ? "enable" : "disable";
    proc->start(BIN_PKEXEC_PATH, {BIN_SYSTEMCTL_PATH, action, service});
#endif
}

void ServiceManager::updateServiceEntry(SystemServiceEntry &entry, const QDBusObjectPath &o)
{
    ErrorContext ec;
    Systemd1ManagerInterface mgrIf(DBUS_SYSTEMD1_SERVICE, kSystemDObjectPath.path(),
                                   QDBusConnection::systemBus());
    Systemd1UnitInterface unitIf(DBUS_SYSTEMD1_SERVICE, o.path(), QDBusConnection::systemBus());
    Systemd1ServiceInterface svcIf(DBUS_SYSTEMD1_SERVICE, o.path(), QDBusConnection::systemBus());

    auto idResult = unitIf.getId();
    ec = idResult.first;
    if (ec) {
        qDebug() << "getId failed:" << ec.getErrorName() << ec.getErrorMessage();
    }
    QString id = idResult.second;
    entry.setId(id.left(id.lastIndexOf(".service")));

    auto loadStateResult = unitIf.getLoadState();
    ec = loadStateResult.first;
    if (ec) {
        qDebug() << "getLoadState failed:" << ec.getErrorName() << ec.getErrorMessage();
    }
    entry.setLoadState(loadStateResult.second);

    auto activeStateResult = unitIf.getActiveState();
    ec = activeStateResult.first;
    if (ec) {
        qDebug() << "getActiveState failed:" << ec.getErrorName() << ec.getErrorMessage();
    }
    entry.setActiveState(activeStateResult.second);

    auto subStateResult = unitIf.getSubState();
    ec = subStateResult.first;
    if (ec) {
        qDebug() << "getSubState failed:" << ec.getErrorName() << ec.getErrorMessage();
    }
    entry.setSubState(subStateResult.second);

    auto unitFileStateResult = mgrIf.GetUnitFileState(id);
    ec = unitFileStateResult.first;
    if (ec) {
        qDebug() << "getUnitFileState failed:" << entry.getId() << ec.getErrorName()
                 << ec.getErrorMessage();
    }
    entry.setState(unitFileStateResult.second);

    auto descResult = unitIf.getDescription();
    ec = descResult.first;
    if (ec) {
        qDebug() << "getDescription failed:" << ec.getErrorName() << ec.getErrorMessage();
    }
    entry.setDescription(descResult.second);

    auto mainPIDResult = svcIf.getMainPID();
    ec = mainPIDResult.first;
    if (ec) {
        qDebug() << "getMainPID failed:" << ec.getErrorName() << ec.getErrorMessage();
    }
    entry.setMainPID(mainPIDResult.second);

    auto canStartResult = unitIf.canStart();
    ec = canStartResult.first;
    if (ec) {
        qDebug() << "canStart failed:" << ec.getErrorName() << ec.getErrorMessage();
    }
    entry.setCanStart(canStartResult.second);

    auto canStopResult = unitIf.canStop();
    ec = canStopResult.first;
    if (ec) {
        qDebug() << "canStop failed:" << ec.getErrorName() << ec.getErrorMessage();
    }
    entry.setCanStop(canStopResult.second);

    auto canReloadResult = unitIf.canReload();
    ec = canReloadResult.first;
    if (ec) {
        qDebug() << "canReload failed:" << ec.getErrorName() << ec.getErrorMessage();
    }
    entry.setCanReload(canReloadResult.second);
}
