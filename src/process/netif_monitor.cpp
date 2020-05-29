#include "netif_monitor.h"

std::atomic<NetifMonitor *> NetifMonitor::m_instance;
std::mutex NetifMonitor::m_mutex;

NetifMonitor::NetifMonitor(QObject *parent) : QThread(parent)
{
    m_monitorJob = new NetifMonitorJob();
    m_monitorJob->moveToThread(&m_jobThread);
    connect(&m_jobThread, &QThread::finished, m_monitorJob, &QObject::deleteLater);
    m_jobThread.start();
}

NetifMonitor::~NetifMonitor()
{
    m_jobThread.quit();
    m_jobThread.wait();
}

void NetifMonitor::run()
{

}
