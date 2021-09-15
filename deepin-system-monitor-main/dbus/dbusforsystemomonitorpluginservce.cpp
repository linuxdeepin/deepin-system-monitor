#include "dbusforsystemomonitorpluginservce.h"
#include "detailwidgetmanager.h"
#include "application.h"
#include <QApplication>
DBusForSystemoMonitorPluginServce::DBusForSystemoMonitorPluginServce(QObject *parent) : QObject (parent)
{

}

void DBusForSystemoMonitorPluginServce::slotJumpDetailWidget(const QString &msgCode)
{
    // 判断是否有模态窗口，如果没有则跳转到指定界面
    if (qApp->activeModalWidget() == nullptr && gApp->getCurrentTaskState() == Application::TaskState::kTaskFinished) {
        DetailWidgetManager::getInstance().jumpDetailWidget(msgCode);
    }

}

void DBusForSystemoMonitorPluginServce::slotJumpProcessWidget(const QString &msgCode)
{
    // 判断是否有模态窗口，如果没有则跳转到指定界面
    if (qApp->activeModalWidget() == nullptr && gApp->getCurrentTaskState() == Application::TaskState::kTaskFinished) {
        DetailWidgetManager::getInstance().jumpProcessWidget(msgCode);
    }
}
