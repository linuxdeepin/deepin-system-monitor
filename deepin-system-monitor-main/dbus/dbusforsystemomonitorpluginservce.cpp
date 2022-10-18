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

#include "dbusforsystemomonitorpluginservce.h"
#include "detailwidgetmanager.h"
#include "application.h"
#include <QApplication>
#include <QTimer>
DBusForSystemoMonitorPluginServce::DBusForSystemoMonitorPluginServce(QObject *parent) : QObject (parent)
{

}

void DBusForSystemoMonitorPluginServce::slotJumpDetailWidget(const QString &msgCode)
{
    // 判断是否有模态窗口，如果没有则跳转到指定界面(现不判断模态窗口)
    //if (qApp->activeModalWidget() == nullptr && gApp->getCurrentTaskState() == Application::TaskState::kTaskFinished) {
    if (gApp->getCurrentTaskState() == Application::TaskState::kTaskFinished) {
        // 窗口置顶显示
        gApp->raiseWindow();
        DetailWidgetManager::getInstance().jumpDetailWidget(msgCode);
    }
}

void DBusForSystemoMonitorPluginServce::slotJumpProcessWidget(const QString &msgCode)
{
    // 判断是否有模态窗口，如果没有则跳转到指定界面
    if (gApp->getCurrentTaskState() == Application::TaskState::kTaskFinished) {
        // 窗口置顶显示
        gApp->raiseWindow();
        DetailWidgetManager::getInstance().jumpProcessWidget(msgCode);
    }
}

void DBusForSystemoMonitorPluginServce::slotRaiseWindow()
{
    // 判断是否有模态窗口，如果没有则跳转到指定界面
    if (gApp->getCurrentTaskState() == Application::TaskState::kTaskFinished) {
        // 窗口置顶显示
        gApp->raiseWindow();
    }
}
