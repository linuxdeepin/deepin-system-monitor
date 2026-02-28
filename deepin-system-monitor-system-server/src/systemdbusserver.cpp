// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemdbusserver.h"
#include "ddlog.h"

#include <QCoreApplication>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QProcess>
#include <QTimer>
#include <QDebug>

#include <polkit-qt5-1/PolkitQt1/Authority>
#include <polkit-qt5-1/PolkitQt1/Subject>

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
        return true;
    } else {
        qWarning() << qPrintable("Policy authorization check failed!");
        return false;
    }
}

SystemDBusServer::SystemDBusServer(QObject *parent)
    : QObject(parent)
{
    // name: 配置文件中的服务名称 org.deepin.SystemMonitorSystemServer
    QDBusConnection dbus = QDBusConnection::systemBus();
    if (dbus.registerService("org.deepin.SystemMonitorSystemServer")) {
        QDBusConnection::RegisterOptions opts =
                QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties;
        if (!dbus.registerObject("/org/deepin/SystemMonitorSystemServer", this, opts)) {
            qWarning() << qPrintable("Register dbus object failed") << dbus.lastError().message();
        }
    } else {
        qWarning() << qPrintable("Register dbus failed") << dbus.lastError().message();
    }
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [=]() { qApp->exit(0); });
}

/**
   @brief 延时退出执行服务
 */
void SystemDBusServer::exitDBusServer(int msec)
{
    qApp->processEvents();
    m_timer.start(msec);
}

/**
   @brief 设置 systemctl 服务 \a serviceName 的启动方式为 \b enable ，仅用于非开发者模式
 */
QString SystemDBusServer::setServiceEnable(const QString &serviceName, bool enable)
{
    QString ret = setServiceEnableImpl(serviceName, enable);
    exitDBusServer(8000);
    return ret;
}

/**
   @brief 执行设置 systemctl 服务 \a serviceName 启动方式 \a enable ，将返回详细处理结果
 */
QString SystemDBusServer::setServiceEnableImpl(const QString &serviceName, bool enable)
{
    // 不允许包含';' ' '字符，服务名称长度同样限制
    if (serviceName.isEmpty() || (serviceName.size() > SHRT_MAX) || serviceName.contains(QRegExp("[; ]"))) {
        qWarning() << qPrintable("Invalid service name");
        return QString(strerror(EINVAL));
    }

    // 判断服务是否存在
    QProcess listPorcess;
    listPorcess.start("systemctl", { "list-unit-files", "--type=service", "--no-pager" });
    listPorcess.waitForFinished();
    QByteArray serviceList = listPorcess.readAll();
    if (!serviceList.contains(serviceName.toUtf8())) {
        qWarning() << qPrintable("Service not exists");
        return QString(strerror(EINVAL));
    }

    // 鉴权处理
    if (!checkAuthorization(message().service(), s_PolkitActionSet)) {
        qWarning() << qPrintable("Polkit authorization failed");
        return QString(strerror(EPERM));
    }

    // 执行设置
    QProcess process;
    process.start("systemctl", { enable ? "enable" : "disable", serviceName });
    process.waitForFinished();
    QString errorRet = process.readAll();

    // 检测是否执行成功
    process.start("systemctl", { "is-enabled", serviceName });
    process.waitForFinished();
    QString checkRet = process.readAll();
    if (enable && ("enabled" == checkRet)) {
        return {};
    } else if (!enable && ("disabled" == checkRet)) {
        return {};
    }

    // 返回命令执行结果
    return errorRet;
}
