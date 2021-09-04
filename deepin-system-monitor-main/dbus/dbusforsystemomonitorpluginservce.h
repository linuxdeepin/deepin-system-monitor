/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan<yukuan@uniontech.com>
*
* Maintainer: yukuan<yukuan@uniontech.com>
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

#ifndef DBUSFORSYSTEMOMONITORPLUGINSERVCE_H
#define DBUSFORSYSTEMOMONITORPLUGINSERVCE_H

#include <QObject>
#include <QDBusContext>
class DBusForSystemoMonitorPluginServce : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.SystemMonitorMain")
public:
    DBusForSystemoMonitorPluginServce(QObject *parent = nullptr);

public slots:
    //!
    //! \brief slotJumpDetailWidget 通过注册的DBUS接口，获取当前消息CODE，判断需要跳转到详情界面的画面
    //! \param msgCode
    //!
    Q_SCRIPTABLE void slotJumpDetailWidget(const QString &msgCode);

    Q_SCRIPTABLE void slotJumpProcessWidget(const QString &msgCode);
};

#endif // DBUSFORSYSTEMOMONITORPLUGINSERVCE_H
