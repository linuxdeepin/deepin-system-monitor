// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "service_manager.h"
#include "ddlog.h"
#include "application.h"
#include "dbus/dbus_common.h"
#include "dbus/dbus_properties_interface.h"
#include "dbus/environment_file.h"
#include "dbus/systemd1_manager_interface.h"
#include "dbus/systemd1_service_interface.h"
#include "dbus/systemd1_unit_interface.h"
#include "dbus/unit_file_info.h"
#include "dbus/unit_info.h"
#include "service/system_service_entry.h"
#include "service/service_manager_worker.h"

#include <DApplication>
#include <DLog>

#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QtDBus>
#include <QTimer>
#include <QDBusConnection>
#include <QDBusInterface>

#include <memory>

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#define BIN_PKEXEC_PATH "/usr/bin/pkexec"
#define BIN_SYSTEMCTL_PATH "/usr/bin/systemctl"

DCORE_USE_NAMESPACE
using namespace dbus::common;
using namespace DDLog;
std::atomic<ServiceManager *> ServiceManager::m_instance;
std::mutex ServiceManager::m_mutex;

/**
   @brief 非开发者模式下，使用后端 DBus 服务设置 systemd 服务 \a serviceName 的启动模式
 */
static bool setServiceEnable(const QString &servieName, bool enable, QString &errorString)
{
    qCDebug(app) << "Setting service" << servieName << "to" << (enable ? "enabled" : "disabled");
    QDBusInterface interface("org.deepin.SystemMonitorSystemServer",
                             "/org/deepin/SystemMonitorSystemServer",
                             "org.deepin.SystemMonitorSystemServer",
                             QDBusConnection::systemBus());
    QDBusReply<QString> retMsg = interface.call("setServiceEnable", servieName, enable);
    errorString.clear();
    if (!retMsg.isValid()) {
        errorString = retMsg.error().message();
    } else if (!retMsg.value().isEmpty()) {
        errorString = retMsg.value();
    }

    if (!errorString.isEmpty()) {
        qCWarning(app) << QString("Set service %1 failed, error %1").arg(enable ? "enable" : "disable").arg(errorString);
        return false;
    } else {
        qCDebug(app) << QString("Set service %1 ret: %2").arg(enable ? "enable" : "disable").arg(retMsg.value());
        return true;
    }
}

CustomTimer::CustomTimer(ServiceManager *mgr, QObject *parent)
    : QObject(parent), m_mgr(mgr)
{
    m_timer = new QTimer(parent);
}

void CustomTimer::start(const QString &path)
{
    qCDebug(app) << "Starting custom timer for path" << path;
    connect(m_timer, &QTimer::timeout, this, [=]() {
        qCDebug(app) << "Custom timer timeout for path" << path << "count" << m_cnt;
        SystemServiceEntry e = m_mgr->updateServiceEntry(path);
        if (m_cnt >= 6 || isFinalState(e.getActiveState().toLocal8Bit())) {
            qCDebug(app) << "Stopping custom timer for path" << path;
            m_timer->stop();
            this->deleteLater();
        } else {
            ++m_cnt;
            m_timer->stop();
            m_timer->start(m_cnt * m_cnt * 200);
        }
    });
    m_timer->start(200);
}

ServiceManager::ServiceManager(QObject *parent)
    : QObject(parent)
{
    qCDebug(app) << "ServiceManager object created";
    UnitInfo::registerMetaType();
    UnitFileInfo::registerMetaType();
    EnvironmentFile::registerMetaType();

    qRegisterMetaType<QList<SystemServiceEntry>>("ServiceEntryList");

    m_worker = new ServiceManagerWorker();
    m_worker->moveToThread(&m_workerThread);
    connect(this, &ServiceManager::beginUpdateList, m_worker, &ServiceManagerWorker::startJob);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &ServiceManagerWorker::resultReady, this, &ServiceManager::serviceListUpdated);
    qCDebug(app) << "Starting worker thread";
    m_workerThread.start();
}

ServiceManager::~ServiceManager()
{
    qCDebug(app) << "ServiceManager object destroyed";
    m_workerThread.quit();
    m_workerThread.wait();
}

void ServiceManager::updateServiceList()
{
    qCInfo(app) << "Requesting service list update";
    Q_EMIT beginUpdateList();
}

QString ServiceManager::normalizeServiceId(const QString &id, const QString &param)
{
    qCDebug(app) << "Normalizing service id" << id << "with param" << param;
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
    qCDebug(app) << "Normalized service id:" << buf;

    return buf;
}

ErrorContext ServiceManager::startService(const QString &id, const QString &param)
{
    qCDebug(app) << "Starting service" << id << "with param" << param;
    ErrorContext ec {};
    Systemd1ManagerInterface iface(DBUS_SYSTEMD1_SERVICE,
                                   kSystemDObjectPath.path(),
                                   QDBusConnection::systemBus());

    auto buf = normalizeServiceId(id, param);
    auto mode = UnitControlJobModeMap.value(kJobModeReplace);

    auto oResult = iface.StartUnit(buf, mode);
    ec = oResult.first;
    if (ec) {
        qCWarning(app) << "Failed to start service:" << buf << "Error:" << ec.getErrorName() << ec.getErrorMessage();
        return ec;
    }
    qCDebug(app) << "Successfully called StartUnit for" << buf;
    QDBusObjectPath o = oResult.second;

    if (id.endsWith("@"))
        return ErrorContext();

    oResult = iface.GetUnit(buf);
    ec = oResult.first;
    SystemServiceEntry entry;
    if (ec) {
        if (ec.getCode() == 3) {
            qCDebug(app) << "GetUnit failed with code 3, normalizing path for" << buf;
            auto o1 = Systemd1UnitInterface::normalizeUnitPath(buf);
            entry = updateServiceEntry(o1.path());
        } else {
            qCWarning(app) << "Failed to get unit after start:" << buf << "Error:" << ec.getErrorName() << ec.getErrorMessage();
            return ec;
        }
    } else {
        o = oResult.second;
        entry = updateServiceEntry(o.path());
    }

    if (!isFinalState(entry.getActiveState().toLocal8Bit())) {
        qCDebug(app) << "Service not in final state, starting timer for:" << buf;
        auto *timer = new CustomTimer { this };
        timer->start(o.path());
    }

    return ErrorContext();
}

ErrorContext ServiceManager::stopService(const QString &id)
{
    qCDebug(app) << "Stopping service" << id;
    ErrorContext ec {};
    Systemd1ManagerInterface iface(DBUS_SYSTEMD1_SERVICE,
                                   kSystemDObjectPath.path(),
                                   QDBusConnection::systemBus());

    auto buf = normalizeServiceId(id);
    auto mode = UnitControlJobModeMap.value(kJobModeReplace);

    auto oResult = iface.StopUnit(buf, mode);
    ec = oResult.first;
    if (ec) {
        qCWarning(app) << "Failed to stop service:" << buf << "Error:" << ec.getErrorName() << ec.getErrorMessage();
        return ec;
    }
    qCDebug(app) << "Successfully called StopUnit for" << buf;
    QDBusObjectPath o = oResult.second;

    if (id.endsWith("@"))
        return ErrorContext();

    oResult = iface.GetUnit(buf);
    ec = oResult.first;
    SystemServiceEntry entry;
    if (ec) {
        if (ec.getCode() == 3) {
            qCDebug(app) << "GetUnit failed with code 3, normalizing path for" << buf;
            auto o1 = Systemd1UnitInterface::normalizeUnitPath(buf);
            entry = updateServiceEntry(o1.path());
        } else {
            qCWarning(app) << "Failed to get unit after stop:" << buf << "Error:" << ec.getErrorName() << ec.getErrorMessage();
            return ec;
        }
    } else {
        o = oResult.second;
        entry = updateServiceEntry(o.path());
    }

    if (!isFinalState(entry.getActiveState().toLocal8Bit())) {
        qCDebug(app) << "Service not in final state, starting timer for:" << buf;
        auto *timer = new CustomTimer { this };
        timer->start(o.path());
    }

    return ErrorContext();
}

ErrorContext ServiceManager::restartService(const QString &id, const QString &param)
{
    qCDebug(app) << "Restarting service" << id << "with param" << param;
    ErrorContext ec {};
    Systemd1ManagerInterface iface(DBUS_SYSTEMD1_SERVICE,
                                   kSystemDObjectPath.path(),
                                   QDBusConnection::systemBus());

    auto buf = normalizeServiceId(id, param);
    auto mode = UnitControlJobModeMap.value(kJobModeReplace);

    auto oResult = iface.RestartUnit(buf, mode);
    ec = oResult.first;
    if (ec) {
        qCWarning(app) << "Failed to restart service:" << buf << "Error:" << ec.getErrorName() << ec.getErrorMessage();
        return ec;
    }
    qCDebug(app) << "Successfully called RestartUnit for" << buf;
    QDBusObjectPath o = oResult.second;

    if (id.endsWith("@"))
        return ErrorContext();

    oResult = iface.GetUnit(buf);
    ec = oResult.first;
    SystemServiceEntry entry;
    if (ec) {
        if (ec.getCode() == 3) {
            qCDebug(app) << "GetUnit failed with code 3, normalizing path for" << buf;
            auto o1 = Systemd1UnitInterface::normalizeUnitPath(buf);
            entry = updateServiceEntry(o1.path());
        } else {
            qCWarning(app) << "Failed to get unit after restart:" << buf << "Error:" << ec.getErrorName() << ec.getErrorMessage();
            return ec;
        }
    } else {
        o = oResult.second;
        entry = updateServiceEntry(o.path());
    }

    if (!isFinalState(entry.getActiveState().toLocal8Bit())) {
        qCDebug(app) << "Service not in final state, starting timer for:" << buf;
        auto *timer = new CustomTimer { this };
        timer->start(o.path());
    }

    return ErrorContext();
}

ErrorContext ServiceManager::setServiceStartupMode(const QString &id, bool autoStart)
{
    qCDebug(app) << "Setting service startup mode for" << id << "to" << autoStart;
    ErrorContext ec {};

#ifdef USE_POLICYKIT1_AUTHORITy_API
    // check sysv script

    // enable/disable with EnableUnitFiles/DisableUnitFiles
    Systemd1ManagerInterface mgrIf(DBUS_SYSTEMD1_SERVICE, kSystemDObjectPath.path(),
                                   QDBusConnection::systemBus());
    if (autoStart) {
        auto re = mgrIf.EnableUnitFiles({ entry.getId() });
        ec = re.first;
        if (ec) {
            qCWarning(app) << "Failed to enable unit files:" << ec.getErrorName() << ec.getErrorMessage();
            return ec;
        }
        auto eResult = re.second;
    } else {
        auto re = mgrIf.DisableUnitFiles({ entry.getId() });
        ec = re.first;
        if (ec) {
            qCWarning(app) << "Failed to disable unit files:" << ec.getErrorName() << ec.getErrorMessage();
            return ec;
        }
        auto dResult = re.second;
    }
#else
    qCDebug(app) << "Not using PolicyKit1, using pkexec/dbus backend";
    auto errfmt = [=](ErrorContext &pe, decltype(errno) err, const QString &title, const QString &message = {}) -> ErrorContext & {
        pe.setCode(ErrorContext::kErrorTypeSystem);
        pe.setSubCode(err);
        pe.setErrorName(title);
        auto errmsg = (err != 0 ? QString("Error: [%1] %2").arg(err).arg(strerror(err)) : QString("Error: "));
        if (!message.isEmpty()) {
            errmsg = QString("%1 - %2").arg(errmsg).arg(message);
        }
        pe.setErrorMessage(errmsg);
        return pe;
    };
    const QString title = QApplication::translate("Service.Action.Set.Startup.Mode",
                                                  "Failed to set service startup type");
    int rc {};
    struct stat sbuf;

    // check pkexec existence
    errno = 0;
    sbuf = {};
    rc = stat(BIN_PKEXEC_PATH, &sbuf);
    if (rc == -1) {
        qCWarning(app) << "pkexec not found at:" << BIN_PKEXEC_PATH;
        ec = errfmt(ec, errno, title, BIN_PKEXEC_PATH);
        return ec;
    }

    // check systemctl existence
    errno = 0;
    sbuf = {};
    rc = stat(BIN_SYSTEMCTL_PATH, &sbuf);
    if (rc == -1) {
        qCWarning(app) << "systemctl not found at:" << BIN_SYSTEMCTL_PATH;
        ec = errfmt(ec, errno, title, BIN_SYSTEMCTL_PATH);
        return ec;
    }

    QFile file("/var/lib/deepin/developer-mode/enabled");
    bool developerMode = false;
    if (file.open(QIODevice::ReadOnly)) {
        QString lineStr = file.readLine();
        developerMode = (!lineStr.isEmpty() && lineStr.trimmed() == "1");
    }
    qCDebug(app) << "Developer mode:" << (developerMode ? "enabled" : "disabled");

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    // {BIN_PKEXEC_PATH} {BIN_SYSTEMCTL_PATH} {enable/disable} {service}
    QString action = autoStart ? "enable" : "disable";
    bool useProcess = true;
    if (developerMode) {
        qCDebug(app) << "Using pkexec for service control";
        proc.start(BIN_PKEXEC_PATH, { BIN_SYSTEMCTL_PATH, action, id });
    } else {
        // Bug 241793 非开发者模式，使用后端DBus服务设置启动方式
#if 0
        proc.start(BIN_SYSTEMCTL_PATH, {action, id});
#else
        useProcess = false;
        QString errorString;
        bool dbusRet = setServiceEnable(id, autoStart, errorString);
        if (!dbusRet) {
            errno = 0;
            ErrorContext errCtx {};
            errCtx = errfmt(errCtx, errno, title, errorString);
            return errCtx;
        }
#endif
    }

    ErrorContext le {};
    if (useProcess) {
        proc.waitForFinished(-1);
        auto exitStatus = proc.exitStatus();

        if (exitStatus == QProcess::CrashExit) {
            qCWarning(app) << "Process crashed while setting service startup mode";
            errno = 0;
            le = errfmt(le, errno, title, QApplication::translate("Service.Action.Set.Startup.Mode", "Error: Failed to set service startup type due to the crashed sub process."));
            return le;
        } else {
            auto exitCode = proc.exitCode();
            if (exitCode == 127 || exitCode == 126) {
                qCWarning(app) << "Permission denied while setting service startup mode";
                errno = EPERM;
                le = errfmt(le, errno, title);
                return le;
            } else if (exitCode != 0) {
                auto buf = proc.readAllStandardOutput();
                qCWarning(app) << "Failed to set service startup mode. Output:" << buf;
                errno = 0;
                le = errfmt(le, errno, title, buf);
                return le;
            }
        }
    }

    {
        // success - refresh service stat -send signal

        // special case, do nothing there
        if (id.endsWith("@")) {
            qCDebug(app) << "Service id ends with '@', returning early";
            return ErrorContext();
        }

        Systemd1ManagerInterface mgrIf(DBUS_SYSTEMD1_SERVICE,
                                       kSystemDObjectPath.path(),
                                       QDBusConnection::systemBus());
        auto buf = normalizeServiceId(id, {});
        auto re = mgrIf.GetUnit(buf);
        le = re.first;
        if (le) {
            if (le.getCode() == 3) {
                qCDebug(app) << "GetUnit failed with code 3, normalizing path for" << buf;
                auto o = Systemd1UnitInterface::normalizeUnitPath(buf);
                updateServiceEntry(o.path());
            } else {
                qCWarning(app) << "Failed to get unit after setting startup mode:" << buf;
                return le;
            }
        } else {
            updateServiceEntry(re.second.path());
        }
    }
#endif
    return ErrorContext();
}

SystemServiceEntry ServiceManager::updateServiceEntry(const QString &opath)
{
    qCDebug(app) << "Updating service entry for path:" << opath;
    ErrorContext ec;
    SystemServiceEntry entry {};

    Systemd1ManagerInterface mgrIf(DBUS_SYSTEMD1_SERVICE,
                                   kSystemDObjectPath.path(),
                                   QDBusConnection::systemBus());
    Systemd1UnitInterface unitIf(DBUS_SYSTEMD1_SERVICE,
                                 opath,
                                 QDBusConnection::systemBus());
    Systemd1ServiceInterface svcIf(DBUS_SYSTEMD1_SERVICE,
                                   opath,
                                   QDBusConnection::systemBus());

    auto idResult = unitIf.getId();
    ec = idResult.first;
    QString id = idResult.second;
    auto sname = id;
    if (ec) {
        qCWarning(app) << "Failed to get ID for path:" << opath << "Error:" << ec.getErrorName() << ec.getErrorMessage();
    } else {
        qCDebug(app) << "Successfully got ID:" << id << "for path:" << opath;
        if (sname.endsWith(UnitTypeServiceSuffix)) {
            sname.chop(strlen(UnitTypeServiceSuffix));
        }
        entry.setId(id);
        entry.setSName(sname);
    }

    auto loadStateResult = unitIf.getLoadState();
    ec = loadStateResult.first;
    if (ec) {
        qCWarning(app) << "Failed to get load state for:" << id << "Error:" << ec.getErrorName() << ec.getErrorMessage();
    } else {
        qCDebug(app) << "Successfully got load state:" << loadStateResult.second << "for:" << id;
        entry.setLoadState(loadStateResult.second);
    }

    auto activeStateResult = unitIf.getActiveState();
    ec = activeStateResult.first;
    if (ec) {
        qCWarning(app) << "Failed to get active state for:" << id << "Error:" << ec.getErrorName() << ec.getErrorMessage();
    } else {
        qCDebug(app) << "Successfully got active state:" << activeStateResult.second << "for:" << id;
        entry.setActiveState(activeStateResult.second);
    }

    auto subStateResult = unitIf.getSubState();
    ec = subStateResult.first;
    if (ec) {
        qCWarning(app) << "Failed to get sub state for:" << id << "Error:" << ec.getErrorName() << ec.getErrorMessage();
    } else {
        qCDebug(app) << "Successfully got sub state:" << subStateResult.second << "for:" << id;
        entry.setSubState(subStateResult.second);
    }

    auto unitFileStateResult = mgrIf.GetUnitFileState(id);
    ec = unitFileStateResult.first;
    if (ec) {
        qCWarning(app) << "Failed to get unit file state for:" << id << "Error:" << ec.getErrorName() << ec.getErrorMessage();
    } else {
        qCDebug(app) << "Successfully got unit file state:" << unitFileStateResult.second << "for:" << id;
        entry.setState(unitFileStateResult.second);
        entry.setStartupType(ServiceManager::getServiceStartupType(
                entry.getSName(),
                entry.getState()));
    }

    auto descResult = unitIf.getDescription();
    ec = descResult.first;
    if (ec) {
        qCWarning(app) << "Failed to get description for:" << id << "Error:" << ec.getErrorName() << ec.getErrorMessage();
    } else {
        qCDebug(app) << "Successfully got description for:" << id;
        entry.setDescription(descResult.second);
    }

    auto mainPIDResult = svcIf.getMainPID();
    ec = mainPIDResult.first;
    if (ec) {
        qCWarning(app) << "Failed to get main PID for:" << id << "Error:" << ec.getErrorName() << ec.getErrorMessage();
    } else {
        qCDebug(app) << "Successfully got main PID:" << mainPIDResult.second << "for:" << id;
        entry.setMainPID(mainPIDResult.second);
    }

    auto canStartResult = unitIf.canStart();
    ec = canStartResult.first;
    if (ec) {
        qCWarning(app) << "Failed to check if unit can start:" << id << "Error:" << ec.getErrorName() << ec.getErrorMessage();
    } else {
        qCDebug(app) << "Successfully checked if unit can start:" << canStartResult.second << "for:" << id;
        entry.setCanStart(canStartResult.second);
    }

    auto canStopResult = unitIf.canStop();
    ec = canStopResult.first;
    if (ec) {
        qCWarning(app) << "Failed to check if unit can stop:" << id << "Error:" << ec.getErrorName() << ec.getErrorMessage();
    } else {
        qCDebug(app) << "Successfully checked if unit can stop:" << canStopResult.second << "for:" << id;
        entry.setCanStop(canStopResult.second);
    }

    auto canReloadResult = unitIf.canReload();
    ec = canReloadResult.first;
    if (ec) {
        qCWarning(app) << "Failed to check if unit can reload:" << id << "Error:" << ec.getErrorName() << ec.getErrorMessage();
    } else {
        qCDebug(app) << "Successfully checked if unit can reload:" << canReloadResult.second << "for:" << id;
        entry.setCanReload(canReloadResult.second);
    }

    Q_EMIT serviceStatusUpdated(entry);

    return entry;
}
