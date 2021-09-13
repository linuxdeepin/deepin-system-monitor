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

#include <QCoreApplication>
#include <QtDBus/QDBusInterface>
#include <QDBusConnection>
#include <QDebug>

#include "systemmonitorservice.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qApp->setOrganizationName("deepin");

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
