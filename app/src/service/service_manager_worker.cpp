#include <QDebug>
#include <QtConcurrent>

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
#include "service_manager_worker.h"

ServiceManagerWorker::ServiceManagerWorker(QObject *parent) : QObject(parent)
{
}

void ServiceManagerWorker::startJob()
{
    QList<SystemServiceEntry> list;
    // id hash, only keys are used here
    QHash<QString, int> hash;
    ErrorContext ec;

    Systemd1ManagerInterface mgrIf(DBUS_SYSTEMD1_SERVICE,
                                   kSystemDObjectPath.path(),
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
    for (auto u : units) {
        if (u.getName().endsWith(UnitTypeServiceSuffix)) {
            hash[u.getName()] = 0;
        }
    }

    // filter-out non service type units
    std::function<bool(const UnitInfo &)> fltUnits;
    fltUnits = [](const UnitInfo & unit) -> bool {
        return unit.getName().endsWith(UnitTypeServiceSuffix);
    };
    std::function<SystemServiceEntry(const UnitInfo &) > mapUnit = [&mgrIf](const UnitInfo & unit) {
        ErrorContext ec{};
        SystemServiceEntry entry{};
        // SName
        entry.setSName(unit.getName().left(unit.getName().lastIndexOf('.')));
        // LoadState
        entry.setLoadState(unit.getLoadState());
        // ActiveState
        entry.setActiveState(unit.getActiveState());
        // subState
        entry.setSubState(unit.getSubState());
        entry.setUnitObjectPath(unit.getUnitObjectPath());
        // Description
        entry.setDescription(unit.getDescription());

        Systemd1UnitInterface unitIf(DBUS_SYSTEMD1_SERVICE,
                                     unit.getUnitObjectPath(),
                                     QDBusConnection::systemBus());
        // Id
        auto id = unitIf.getId();
        ec = id.first;
        if (ec) {
            qDebug() << "call getId failed:" << ec.getErrorName() << ec.getErrorMessage();
        } else {
            entry.setId(id.second);
        }

        auto bStart = unitIf.canStart();
        ec = bStart.first;
        if (ec) {
            qDebug() << "call canStart failed:" << ec.getErrorName() << ec.getErrorMessage();
        } else {
            entry.setCanStart(bStart.second);
        }

        auto bStop = unitIf.canStop();
        ec = bStop.first;
        if (ec) {
            qDebug() << "call canStart failed:" << ec.getErrorName() << ec.getErrorMessage();
        } else {
            entry.setCanStop(bStop.second);
        }

        auto bReload = unitIf.canReload();
        ec = bReload.first;
        if (ec) {
            qDebug() << "call canStart failed:" << ec.getErrorName() << ec.getErrorMessage();
        } else {
            entry.setCanReload(bReload.second);
        }

        // mainPID
        Systemd1ServiceInterface svcIf(DBUS_SYSTEMD1_SERVICE,
                                       unit.getUnitObjectPath(),
                                       QDBusConnection::systemBus());
        auto pid = svcIf.getMainPID();
        ec = pid.first;
        if (ec) {
            qDebug() << "getMainPID failed" << ec.getErrorName() << ec.getErrorMessage();
        } else {
            entry.setMainPID(pid.second);
        }

        // unit state
        auto state = mgrIf.GetUnitFileState(unit.getName());
        ec = state.first;
        if (ec) {
            qDebug() << "getUnitFileState failed" << ec.getErrorName() << ec.getErrorMessage();
        } else {
            entry.setState(state.second);
        }

        // startupType
        entry.setStartupType(ServiceManager::getServiceStartupType(
                                 entry.getSName(),
                                 entry.getState()));

        return entry;
    };
    std::function<void(QList<SystemServiceEntry> &elist, const SystemServiceEntry &entry)> addServiceEntryFromUnitInfo;
    addServiceEntryFromUnitInfo = [](QList<SystemServiceEntry> &elist, const SystemServiceEntry & entry) {
        elist << entry;
    };
    auto fltufe = QtConcurrent::filter(units, fltUnits);
    fltufe.waitForFinished();
    auto ufe = QtConcurrent::mappedReduced<QList<SystemServiceEntry>>(units, mapUnit, addServiceEntryFromUnitInfo);

    std::function<void(QList<SystemServiceEntry> &elist, const SystemServiceEntry &entry)> addServiceEntryFromUnitFileInfo;
    addServiceEntryFromUnitFileInfo = [&hash](QList<SystemServiceEntry> &elist, const SystemServiceEntry & entry) {
        if (!hash.contains(entry.getId())) {
            elist << entry;
        }
    };
    // filter-out non service type unit-files
    std::function<bool(const UnitFileInfo &)> fltUnitFiles;
    fltUnitFiles = [&hash](const UnitFileInfo & unitFile) -> bool {
        auto id = unitFile.getName().mid(unitFile.getName().lastIndexOf('/') + 1);
        if (hash.contains(id))
        {
            return false;
        } else
        {
            return unitFile.getName().endsWith(UnitTypeServiceSuffix);
        }
    };
    std::function<SystemServiceEntry(const UnitFileInfo &) > mapUnitFiles;
    mapUnitFiles = [](const UnitFileInfo & unf) {
        SystemServiceEntry entry{};
        ErrorContext ec{};

        auto id = unf.getName().mid(unf.getName().lastIndexOf('/') + 1);
        auto sname = id;
        sname.chop(strlen(UnitTypeServiceSuffix));

        entry.setSName(sname);
        entry.setState(unf.getStatus());
        entry.setStartupType(ServiceManager::getServiceStartupType(
                                 entry.getSName(),
                                 entry.getState()));
        if (sname.endsWith('@')) {
            // read description from unit file
            auto desc = readUnitDescriptionFromUnitFile(unf.getName());
            entry.setDescription(desc);
        } else {
            auto o = Systemd1UnitInterface::normalizeUnitPath(id);
            Systemd1UnitInterface unitIf(DBUS_SYSTEMD1_SERVICE,
                                         o.path(),
                                         QDBusConnection::systemBus());
            Systemd1ServiceInterface svcIf(DBUS_SYSTEMD1_SERVICE,
                                           o.path(),
                                           QDBusConnection::systemBus());

            // Id
            auto id = unitIf.getId();
            ec = id.first;
            if (ec) {
                qDebug() << "call getId failed:" << ec.getErrorName() << ec.getErrorMessage();
            } else {
                entry.setId(id.second);
            }

            auto desc = unitIf.getDescription();
            ec = desc.first;
            if (ec) {
                qDebug() << "getDescription failed:" << ec.getErrorName() << ec.getErrorMessage();
            } else {
                entry.setDescription(desc.second);
            }

            auto actState = unitIf.getActiveState();
            ec = actState.first;
            if (ec) {
                qDebug() << "getActiveState failed:" << ec.getErrorName() << ec.getErrorMessage();
            } else {
                entry.setActiveState(actState.second);
            }

            auto loadState = unitIf.getLoadState();
            ec = loadState.first;
            if (ec) {
                qDebug() << "getLoadState failed:" << ec.getErrorName() << ec.getErrorMessage();
            } else {
                entry.setLoadState(loadState.second);
            }

            auto subState = unitIf.getSubState();
            ec = subState.first;
            if (ec) {
                qDebug() << "getSubState failed:" << ec.getErrorName() << ec.getErrorMessage();
            } else {
                entry.setSubState(subState.second);
            }

            auto mainPID = svcIf.getMainPID();
            ec = mainPID.first;
            if (ec) {
                qDebug() << "getMainPID failed:" << ec.getErrorName() << ec.getErrorMessage();
            } else {
                entry.setMainPID(mainPID.second);
            }
        }

        return entry;
    };
    auto fltuffe = QtConcurrent::filter(unitFiles, fltUnitFiles);
    fltuffe.waitForFinished();
    auto uffe = QtConcurrent::mappedReduced<QList<SystemServiceEntry>>(unitFiles, mapUnitFiles, addServiceEntryFromUnitFileInfo);

    ufe.waitForFinished();
    uffe.waitForFinished();

    list << ufe.result();
    list << uffe.result();
    Q_EMIT resultReady(list);
}

QString ServiceManagerWorker::readUnitDescriptionFromUnitFile(const QString &path)
{
    FILE *fp        {nullptr};
    const int BLEN  {2048};
    QByteArray buf  {BLEN, 0};
    QByteArray desc {BLEN, 0};
    bool b          {false};

    fp = fopen(path.toLocal8Bit(), "r");
    if (!fp)
        return {};

    while ((fgets(buf.data(), BLEN, fp))) {
        if (sscanf(buf.data(), "Description=%[^\n]", desc.data()) == 1) {
            b = true;
            break;
        }
    }
    fclose(fp);

    if (b) {
        return desc.data();
    } else {
        return {};
    }
}
