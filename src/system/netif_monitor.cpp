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
#include "netif_monitor.h"
#include <QTimerEvent>
#include <QDebug>
namespace core {
namespace system {
NetifMonitor::NetifMonitor(QObject *parent)
    : QObject(parent)
    , m_netifCapture(new NetifPacketCapture(this))
{
}


NetifMonitor::~NetifMonitor()
{
    m_basictimer.stop();
    delete  m_netifCapture;
}
void NetifMonitor::startNetmonitorJob()
{
    m_basictimer.stop();
    m_basictimer.start(2000, Qt::VeryCoarseTimer, this);
}

void NetifMonitor::handleNetData()
{
    while (!m_quitRequested.load()) {
        m_pktqLock.lock();      // +++m_pktqLock+++
        // packets available for processing when pending queue mutex unlocked
        m_pktqWatcher.wait(&m_pktqLock);

        // check if quit requested again after wakeup by another thread, break the loop if need
        if (m_quitRequested.load())
            break;

        // append pending queue packets to local queue, so we dont lock the mutex for too long
        m_localPendingPackets.append(m_pendingPackets);
        m_pendingPackets.clear();
        m_pktqLock.unlock();    // ---m_pktqLock---

        // process payload queue
        while (!m_localPendingPackets.isEmpty()) {
            auto payload = m_localPendingPackets.dequeue();

            // lock sockiostatmap
            m_sockIOStatMapLock.lock();     // +++m_sockIOStatMapLock+++

            // sum up sock io stat by inode
            auto ino = payload->ino;
            if (m_sockIOStatMap.contains(ino)) {
                // sum up sock io stat if already exists with same inode stat
                auto &hist = m_sockIOStatMap[ino];
                if (payload->direction == kInboundPacket) {
                    hist->rx_bytes += payload->payload;
                    hist->rx_packets++;
                } else if (payload->direction == kOutboundPacket) {
                    hist->tx_bytes += payload->payload;
                    hist->tx_packets++;
                }
            } else {
                // add new sock io stat if sock ino no exists in cache before
                auto stat = QSharedPointer<struct sock_io_stat_t>::create();
                stat->ino = payload->ino;
                if (payload->direction == kInboundPacket) {
                    stat->rx_bytes = payload->payload;
                    stat->rx_packets++;
                } else if (payload->direction == kOutboundPacket) {
                    stat->tx_bytes = payload->payload;
                    stat->tx_packets++;
                }
                m_sockIOStatMap[stat->ino] = stat;
            }
            m_sockIOStatMapLock.unlock();   // ---m_sockIOStatMapLock---
        }
    }
}

void NetifMonitor::requestInterrupt()
{
    m_basictimer.stop();
}

void NetifMonitor::timerEvent(QTimerEvent *event)
{
    QObject::timerEvent(event);
    if (event->timerId() == m_basictimer.timerId()) {
        m_netifCapture->dispatchPackets();
    }
}

}
}
