/*
* Copyright (C) 2011 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      wangchao <wangchao@uniontech.com>
* Maintainer:  wangchao <wangchao@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
        // 如果存在 wc service 运行，注册dbus服务会失败,将退出程序
        // 保证单例运行
        qDebug() << __FUNCTION__ << __LINE__ << " , registerService fail. wc service exist. will exit(0).";
        exit(0);
    }

    connection.registerObject(DAEMON_DBUS_PATH, &services);
    return a.exec();
}
