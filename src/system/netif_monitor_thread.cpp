/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "netif_monitor_thread.h"
#include "netif_monitor.h"

namespace core {
namespace system {

NetifMonitorThread::NetifMonitorThread(QObject *parent)
    : BaseThread(parent)
    , m_netifMonitor(new NetifMonitor)
{
    m_netifMonitor->moveToThread(&m_netIfmoniterThread);
    connect(&m_netIfmoniterThread, &QThread::finished, this, &QObject::deleteLater);
    connect(&m_netIfmoniterThread, &QThread::started, m_netifMonitor, &NetifMonitor::startNetmonitorJob);
    m_netIfmoniterThread.start();
}

NetifMonitorThread::~NetifMonitorThread()
{
    m_netifMonitor->requestQuit();
    m_netIfmoniterThread.quit();
    m_netIfmoniterThread.wait();

    m_netifMonitor->deleteLater();
    quit();
    wait();
}

NetifMonitor *NetifMonitorThread::netifJobInstance() const
{
    return m_netifMonitor;
}

void NetifMonitorThread::run()
{
    m_netifMonitor->handleNetData();
}

} // namespace system
} // namespace core
