// Copyright (C) 2011 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemmonitorservice.h"

#include <QCoreApplication>
#include <QtDBus/QDBusInterface>
#include <QDBusConnection>
#include <QDebug>
#include <QTranslator>
#include <QLocale>
#include <QFile>

#define QM_PATH "/usr/share/deepin-system-monitor-daemon/translations"
#define QM_FILE "deepin-system-monitor-daemon_%1.qm"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qApp->setOrganizationName("deepin");

    // 加载 QM 文件，QCoreApplication 需要使用以下方式加载翻译
    QLocale local;
    QString qmFile = QString(QM_FILE).arg(local.name()); 
    QTranslator trans;
    trans.load(qmFile, QM_PATH);
    a.installTranslator(&trans);

    SystemMonitorService services(&a);

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (connection.registerService(DAEMON_DBUS_NAME) == false) {
        // 如果存在 com.deepin.SystemMonitor.Daemon dbus service 运行，注册dbus服务会失败,将退出程序
        // 保证单例运行
        qDebug() << __FUNCTION__ << __LINE__ << QString(" , register dbus service fail. %1 exist. will exit(0).").arg(DAEMON_DBUS_NAME);
        exit(0);
    }

    connection.registerObject(DAEMON_DBUS_PATH, &services);
    return a.exec();
}
