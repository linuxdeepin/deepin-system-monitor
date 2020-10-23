#include "netif_monitor.h"

#include "netif_info_cache.h"

#include <QDebug>

NetifMonitor::NetifMonitor(QObject *parent)
    : QObject(parent)
    , m_netifPacketCapture(new NetifPacketCapture(this))
{
    m_netifPacketCapture->start();
}

NetifMonitor::~NetifMonitor()
{
    m_netifPacketCapture->requestQuit();
    m_netifPacketCapture->quit();
    m_netifPacketCapture->wait();
}
