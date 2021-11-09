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
    // 判断是否有模态窗口，如果没有则跳转到指定界面
    if (qApp->activeModalWidget() == nullptr && gApp->getCurrentTaskState() == Application::TaskState::kTaskFinished) {
        // 窗口置顶显示
        gApp->raiseWindow();
        DetailWidgetManager::getInstance().jumpDetailWidget(msgCode);
    }
}

void DBusForSystemoMonitorPluginServce::slotJumpProcessWidget(const QString &msgCode)
{
    // 判断是否有模态窗口，如果没有则跳转到指定界面
    if (qApp->activeModalWidget() == nullptr && gApp->getCurrentTaskState() == Application::TaskState::kTaskFinished) {
        // 窗口置顶显示
        gApp->raiseWindow();
        DetailWidgetManager::getInstance().jumpProcessWidget(msgCode);
    }
}

void DBusForSystemoMonitorPluginServce::slotRaiseWindow()
{
    // 判断是否有模态窗口，如果没有则跳转到指定界面
    if (qApp->activeModalWidget() == nullptr && gApp->getCurrentTaskState() == Application::TaskState::kTaskFinished) {
        // 窗口置顶显示
        gApp->raiseWindow();
    }
}
