#include "dbusforsystemomonitorpluginservce.h"
#include "detailwidgetmanager.h"
DBusForSystemoMonitorPluginServce::DBusForSystemoMonitorPluginServce(QObject *parent) : QObject (parent)
{

}

void DBusForSystemoMonitorPluginServce::slotJumpDetailWidget(const QString &msgCode)
{
    //do something
    DetailWidgetManager::getInstance().jumpDetailWidget(msgCode);
}

void DBusForSystemoMonitorPluginServce::slotJumpProcessWidget(const QString &msgCode)
{
    DetailWidgetManager::getInstance().jumpProcessWidget(msgCode);
}
