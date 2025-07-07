// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusserver.h"
#include "ddlog.h"

#include <QDBusMessage>
#include <QDBusConnection>
#include <QProcess>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>

using namespace DDLog;

DBusServer::DBusServer(QObject *parent)
    : QObject(parent)
{
    qCDebug(app) << "DBusServer created";
    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (dbus.registerService("com.deepin.SystemMonitorServer")) {
        qCDebug(app) << "Successfully registered service com.deepin.SystemMonitorServer";
        QDBusConnection::RegisterOptions opts =
            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals |
            QDBusConnection::ExportAllProperties;
        dbus.registerObject("/com/deepin/SystemMonitorServer", this, opts);
    } else {
        qCWarning(app) << "Failed to register service com.deepin.SystemMonitorServer";
    }
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [ = ]() {
        qCDebug(app) << "Exit timer timed out, quitting application";
        qApp->exit(0);
    });
}

void DBusServer::exitDBusServer(int msec)
{
    qCDebug(app) << "Scheduling exit in" << msec << "ms";
    qApp->processEvents();
    m_timer.start(msec);
}

void DBusServer::showCpuAlarmNotify(const QString &argument)
{
    qCDebug(app) << "showCpuAlarmNotify called with argument:" << argument;
    QProcess::startDetached("/usr/bin/deepin-system-monitor", QStringList() << "alarm" << "cpu" << argument);
    exitDBusServer(8000);
}

void DBusServer::showMemoryAlarmNotify(const QString &argument)
{
    qCDebug(app) << "showMemoryAlarmNotify called with argument:" << argument;
    QProcess::startDetached("/usr/bin/deepin-system-monitor", QStringList() << "alarm" << "memory" << argument);
    exitDBusServer(8000);
}

void DBusServer::showDeepinSystemMoniter()
{
    qCDebug(app) << "showDeepinSystemMoniter called";
    // 显示系统监视器
    auto launchProcessByAM = [](){
        qCDebug(app) << "Launching main window via ApplicationManager D-Bus call";
        QDBusMessage message = QDBusMessage::createMethodCall(
                "org.desktopspec.ApplicationManager1",
                "/org/desktopspec/ApplicationManager1/deepin_2dsystem_2dmonitor",
                "org.desktopspec.ApplicationManager1.Application",
                "Launch");

        message << QString("") << QStringList() << QVariantMap();

        QDBusMessage reply = QDBusConnection::sessionBus().call(message);
        if (reply.type() != QDBusMessage::ReplyMessage) {
            qWarning() << "Launch deepin-system-monitor main process error:" << reply.errorMessage();
            return;
        }
    };
    launchProcessByAM();
    // QString cmd("qdbus com.deepin.SystemMonitorMain /com/deepin/SystemMonitorMain com.deepin.SystemMonitorMain.slotRaiseWindow");
    QString cmd("gdbus call -e -d  com.deepin.SystemMonitorMain -o /com/deepin/SystemMonitorMain -m com.deepin.SystemMonitorMain.slotRaiseWindow");
    qCDebug(app) << "Executing command to raise window:" << cmd;
    QTimer::singleShot(100, this, [ = ]() {
        QProcess::startDetached(cmd);
        qApp->processEvents();
        exitDBusServer(8000);
    });
}
