/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan<yukuan@uniontech.com>
* Maintainer: yukuan<yukuan@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DBUS_OBJECT_H
#define DBUS_OBJECT_H


#include <QObject>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>

class DBusObject : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.systemMonitor")
public:
    static DBusObject *instance();

    static void destory();

    /**
     * @brief registerOrNotify
     * 注册服务，成功返回true,如果失败则通知已经存在的服务并返回false
     * @return
     */
    bool registerOrNotify();

    /**
     * @brief unRegister
     * 反注册，当应用准备销毁时调用
     */
    void unRegister();


public slots:
    /**
     * @brief handleWindow
     * 接收DBus激活窗口响应接口
     */
    Q_SCRIPTABLE void handleWindow();

private:
    Q_DISABLE_COPY(DBusObject)

    static DBusObject *s_instance;

    explicit DBusObject(QObject *parent = nullptr);

    ~DBusObject();

};

#endif // DBUS_OBJECT_H
