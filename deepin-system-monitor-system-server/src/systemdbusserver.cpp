// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemdbusserver.h"
#include "ddlog.h"

#include <QCoreApplication>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QStandardPaths>
#include <QProcess>
#include <QTimer>
#include <QDebug>
#include <QRegularExpression>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <polkit-qt5-1/PolkitQt1/Authority>
#include <polkit-qt5-1/PolkitQt1/Subject>
#else
#include <polkit-qt6-1/PolkitQt1/Authority>
#include <polkit-qt6-1/PolkitQt1/Subject>
#endif

using namespace DDLog;

const QString s_PolkitActionSet = "org.deepin.systemmonitor.systemserver.set";

/**
   @brief polkit 鉴权，通过配置文件处理
 */
bool checkAuthorization(const QString &appBusName, const QString &action)
{
    PolkitQt1::Authority::Result ret = PolkitQt1::Authority::instance()->checkAuthorizationSync(
            action, PolkitQt1::SystemBusNameSubject(appBusName), PolkitQt1::Authority::AllowUserInteraction);
    if (PolkitQt1::Authority::Yes == ret) {
        qCDebug(app) << "Authorization check passed";
        return true;
    } else {
        qCWarning(app) << "Policy authorization check failed for bus:" << appBusName;
        return false;
    }
}

/**
   @return 返回PID对应的可执行程序名称
 */
static QString getProcIdExe(qint64 id)
{
    qCDebug(app) << "Getting executable for PID:" << id;
    QString execName;
    if (id > 0) {
        // Read contents of virtual /proc/{pid}/cmdline file
        QString exeSymlinkPath = QString("/proc/%1/exe").arg(id);
        char *actualpath = realpath(exeSymlinkPath.toStdString().c_str(), nullptr);
        execName = QString(actualpath);
        free(actualpath); // free the memory allocated by realpath
    }
    qCDebug(app) << "Executable for PID" << id << "is:" << execName;
    return execName;
}

SystemDBusServer::SystemDBusServer(QObject *parent)
    : QObject(parent)
{
    qCDebug(app) << "SystemDBusServer created";
    // name: 配置文件中的服务名称 org.deepin.SystemMonitorSystemServer
    QDBusConnection dbus = QDBusConnection::systemBus();
    if (dbus.registerService("org.deepin.SystemMonitorSystemServer")) {
        qCDebug(app) << "Successfully registered service org.deepin.SystemMonitorSystemServer";
        QDBusConnection::RegisterOptions opts =
                QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties;
        if (!dbus.registerObject("/org/deepin/SystemMonitorSystemServer", this, opts)) {
            qCWarning(app) << "Failed to register DBus object:" << dbus.lastError().message();
        }
    } else {
        qCWarning(app) << "Failed to register DBus service:" << dbus.lastError().message();
    }
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [=]() { 
        qCDebug(app) << "Timer timeout, exiting application";
        qApp->exit(0); 
    });
}

/**
   @brief 延时退出执行服务
 */
void SystemDBusServer::exitDBusServer(int msec)
{
    qCDebug(app) << "Scheduling exit in" << msec << "ms";
    qApp->processEvents();
    m_timer.start(msec);
}

/**
   @brief 设置 systemctl 服务 \a serviceName 的启动方式为 \b enable ，仅用于非开发者模式
 */
QString SystemDBusServer::setServiceEnable(const QString &serviceName, bool enable)
{
    qCDebug(app) << "setServiceEnable called for service:" << serviceName << "enable:" << enable;
    QString ret = setServiceEnableImpl(serviceName, enable);
    exitDBusServer(8000);
    return ret;
}

/**
   @brief 执行设置 systemctl 服务 \a serviceName 启动方式 \a enable ，将返回详细处理结果
 */
QString SystemDBusServer::setServiceEnableImpl(const QString &serviceName, bool enable)
{
    qCDebug(app) << "setServiceEnableImpl called for service:" << serviceName << "enable:" << enable;
    // 调用者限制前台系统监视器程序
    if (!checkCaller()) {
        qCWarning(app) << "Unauthorized caller attempt to modify service:" << serviceName;
        return QString(strerror(EPERM));
    }

    // 不允许包含';' ' '字符，服务名称长度同样限制
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (serviceName.isEmpty() || (serviceName.size() > SHRT_MAX) || serviceName.contains(QRegExp("[; ]"))) {
#else
    if (serviceName.isEmpty() || (serviceName.size() > SHRT_MAX) || serviceName.contains(QRegularExpression("[; ]"))) {
#endif
        qCWarning(app) << "Invalid service name:" << serviceName;
        return QString(strerror(EINVAL));
    }

    // 判断服务是否存在
    QProcess listPorcess;
    listPorcess.start("systemctl", { "list-unit-files", "--type=service", "--no-pager" });
    listPorcess.waitForFinished();
    QByteArray serviceList = listPorcess.readAll();
    qCDebug(app) << "Checking for service existence...";
    if (!serviceList.contains(serviceName.toUtf8())) {
        qCWarning(app) << "Service does not exist:" << serviceName;
        return QString(strerror(EINVAL));
    }
    qCDebug(app) << "Service" << serviceName << "exists.";

    // 鉴权处理
    if (!checkAuthorization(message().service(), s_PolkitActionSet)) {
        qCWarning(app) << "Polkit authorization failed for service:" << serviceName;
        return QString(strerror(EPERM));
    }

    // 执行设置
    QProcess process;
    qCDebug(app) << "Executing 'systemctl" << (enable ? "enable" : "disable") << serviceName << "'";
    process.start("systemctl", { enable ? "enable" : "disable", serviceName });
    process.waitForFinished();
    QString errorRet = process.readAll();

    // 检测是否执行成功
    qCDebug(app) << "Checking service status after operation...";
    process.start("systemctl", { "is-enabled", serviceName });
    process.waitForFinished();
    QString checkRet = process.readAll();
    if (enable && ("enabled" == checkRet)) {
        qCDebug(app) << "Service successfully enabled:" << serviceName;
        return {};
    } else if (!enable && ("disabled" == checkRet)) {
        qCDebug(app) << "Service successfully disabled:" << serviceName;
        return {};
    }

    // 返回命令执行结果
    qCWarning(app) << "Service state change failed:" << errorRet;
    return errorRet;
}

/**
   @return DBus 调用者的PID
 */
qint64 SystemDBusServer::dbusCallerPid() const
{
    if (!calledFromDBus()) {
        qCDebug(app) << "Not called from DBus";
        return 0;
    }

    auto interface = connection().interface();
    if (interface) {
        return static_cast<qint64>(interface->servicePid(message().service()).value());
    }

    qCDebug(app) << "Failed to get DBus caller PID";
    return 0;
}

/**
   @return 返回调用者是否为前台的系统监视器程序
 */
bool SystemDBusServer::checkCaller() const
{
    if (!calledFromDBus()) {
        qCDebug(app) << "Not called from DBus";
        return false;
    }

    qint64 callerPid = dbusCallerPid();
    QString callerExe = getProcIdExe(callerPid);
    QString dbmExe = QStandardPaths::findExecutable("deepin-system-monitor", { "/usr/bin" });

    qCDebug(app) << "Checking caller authorization - PID:" << callerPid << "Executable:" << callerExe;
    if (callerExe != dbmExe) {
        qCWarning(app) << "Unauthorized caller:" << callerExe;
        return false;
    }

    qCDebug(app) << "Caller authorized:" << callerExe;
    return true;
}
