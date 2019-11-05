#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QtDBus>

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
    if (ec)
        return {ec, {}};
    UnitFileInfoList unitFiles = unitFilesResult.second;

    auto unitsResult = mgrIf.ListUnits();
    ec = unitsResult.first;
    if (ec)
        return {ec, {}};
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
            return {ec, {}};
        }
        QDBusObjectPath o = oResult.second;
        Systemd1UnitInterface unitIf(DBUS_SYSTEMD1_SERVICE, o.path(), QDBusConnection::systemBus());
        auto bResult = unitIf.canStart();
        ec = bResult.first;
        if (ec) {
            qDebug() << "call canStart failed:" << ec.getErrorName() << ec.getErrorMessage();
            return {ec, {}};
        }
        entry.setCanStart(bResult.second);
        bResult = unitIf.canStop();
        ec = bResult.first;
        if (ec) {
            qDebug() << "call canStart failed:" << ec.getErrorName() << ec.getErrorMessage();
            return {ec, {}};
        }
        entry.setCanStop(bResult.second);
        bResult = unitIf.canReload();
        ec = bResult.first;
        if (ec) {
            qDebug() << "call canStart failed:" << ec.getErrorName() << ec.getErrorMessage();
            return {ec, {}};
        }
        entry.setCanReload(bResult.second);
        // mainPID
        Systemd1ServiceInterface svcIf(DBUS_SYSTEMD1_SERVICE, o.path(),
                                       QDBusConnection::systemBus());
        auto pidResult = svcIf.getMainPID();
        ec = pidResult.first;
        if (ec) {
            qDebug() << "getMainPID failed" << ec.getErrorName() << ec.getErrorMessage();
            return {ec, {}};
        }
        entry.setMainPID(pidResult.second);

        // getunitfilestate
        auto stateResult = mgrIf.GetUnitFileState(unit.getName());
        ec = stateResult.first;
        if (ec) {
            qDebug() << "getUnitFileState failed" << ec.getErrorName() << ec.getErrorMessage();
            if (ec.getCode() != QDBusError::NoMemory) {
                return {ec, {}};
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
            return {ec, {}};
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
                return {ec, {}};
            }
            entry.setDescription(descResult.second);

            auto actStateResult = unitIf.getActiveState();
            ec = actStateResult.first;
            if (ec) {
                qDebug() << "getActiveState failed:" << ec.getErrorName() << ec.getErrorMessage();
                return {ec, {}};
            }
            entry.setActiveState(actStateResult.second);

            auto loadStateResult = unitIf.getLoadState();
            ec = loadStateResult.first;
            if (ec) {
                qDebug() << "getLoadState failed:" << ec.getErrorName() << ec.getErrorMessage();
                return {ec, {}};
            }
            entry.setLoadState(loadStateResult.second);

            auto subStateResult = unitIf.getSubState();
            ec = subStateResult.first;
            if (ec) {
                qDebug() << "getSubState failed:" << ec.getErrorName() << ec.getErrorMessage();
                return {ec, {}};
            }
            entry.setSubState(subStateResult.second);

            auto mainPIDResult = svcIf.getMainPID();
            ec = mainPIDResult.first;
            if (ec) {
                qDebug() << "getMainPID failed:" << ec.getErrorName() << ec.getErrorMessage();
                return {ec, {}};
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

    return {ec, list};
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

    oResult = iface.GetUnit(buf);
    ec = oResult.first;
    if (ec) {
        qDebug() << "GetUnit failed:" << buf << ec.getErrorName() << ec.getErrorMessage();
        return {ec, false};
    }
    o = oResult.second;
    ec = updateServiceEntry(entry, o);
    if (ec) {
        return {ec, false};
    }

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

    oResult = iface.GetUnit(buf);
    ec = oResult.first;
    if (ec) {
        qDebug() << "Get Unit failed:" << ec.getErrorName() << ec.getErrorMessage();
        return {ec, false};
    }
    o = oResult.second;
    ec = updateServiceEntry(entry, o);
    if (ec) {
        return {ec, false};
    }

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

    oResult = iface.GetUnit(buf);
    ec = oResult.first;
    if (ec) {
        qDebug() << "Get Unit failed:" << buf << ec.getErrorName() << ec.getErrorMessage();
        return {ec, false};
    }
    o = oResult.second;
    ec = updateServiceEntry(entry, o);
    if (ec) {
        return {ec, false};
    }

    return {ec, true};
}

ErrorContext ServiceManager::updateServiceEntry(SystemServiceEntry &entry, const QDBusObjectPath &o)
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
        return ec;
    }
    QString id = idResult.second;
    entry.setId(id.left(id.lastIndexOf(".service")));

    auto loadStateResult = unitIf.getLoadState();
    ec = loadStateResult.first;
    if (ec) {
        qDebug() << "getLoadState failed:" << ec.getErrorName() << ec.getErrorMessage();
        return ec;
    }
    entry.setLoadState(loadStateResult.second);

    auto activeStateResult = unitIf.getActiveState();
    ec = activeStateResult.first;
    if (ec) {
        qDebug() << "getActiveState failed:" << ec.getErrorName() << ec.getErrorMessage();
        return ec;
    }
    entry.setActiveState(activeStateResult.second);

    auto subStateResult = unitIf.getSubState();
    ec = subStateResult.first;
    if (ec) {
        qDebug() << "getSubState failed:" << ec.getErrorName() << ec.getErrorMessage();
        return ec;
    }
    entry.setSubState(subStateResult.second);

    auto unitFileStateResult = mgrIf.GetUnitFileState(id);
    ec = unitFileStateResult.first;
    if (ec) {
        qDebug() << "getUnitFileState failed:" << entry.getId() << ec.getErrorName()
                 << ec.getErrorMessage();
        return ec;
    }
    entry.setState(unitFileStateResult.second);

    auto descResult = unitIf.getDescription();
    ec = descResult.first;
    if (ec) {
        qDebug() << "getDescription failed:" << ec.getErrorName() << ec.getErrorMessage();
        return ec;
    }
    entry.setDescription(descResult.second);

    auto mainPIDResult = svcIf.getMainPID();
    ec = mainPIDResult.first;
    if (ec) {
        qDebug() << "getMainPID failed:" << ec.getErrorName() << ec.getErrorMessage();
        return ec;
    }
    entry.setMainPID(mainPIDResult.second);

    auto canStartResult = unitIf.canStart();
    ec = canStartResult.first;
    if (ec) {
        qDebug() << "canStart failed:" << ec.getErrorName() << ec.getErrorMessage();
        return ec;
    }
    entry.setCanStart(canStartResult.second);

    auto canStopResult = unitIf.canStop();
    ec = canStopResult.first;
    if (ec) {
        qDebug() << "canStop failed:" << ec.getErrorName() << ec.getErrorMessage();
        return ec;
    }
    entry.setCanStop(canStopResult.second);

    auto canReloadResult = unitIf.canReload();
    ec = canReloadResult.first;
    if (ec) {
        qDebug() << "canReload failed:" << ec.getErrorName() << ec.getErrorMessage();
        return ec;
    }
    entry.setCanReload(canReloadResult.second);

    return ec;
}
