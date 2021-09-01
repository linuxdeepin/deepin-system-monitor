#include "systemmonitordbusadaptor.h"


SystemMonitorDBusAdaptor::SystemMonitorDBusAdaptor(QObject *parent) : QObject(parent)
{

}

SystemMonitorDBusAdaptor::~SystemMonitorDBusAdaptor()
{

}

void SystemMonitorDBusAdaptor::slotShowOrHideSystemMonitorPluginPopupWidget()
{
    emit sigSendShowOrHideSystemMonitorPluginPopupWidget();
}

