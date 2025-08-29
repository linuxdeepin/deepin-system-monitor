// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netif_monitor.h"
#include "ddlog.h"
#include "common/thread_manager.h"
#include "netif_monitor_thread.h"

#include <QTimerEvent>
#include <QDebug>

using namespace DDLog;

namespace core {
namespace system {
NetifMonitor::NetifMonitor(QObject *parent)
    : QObject(parent)
{
    qCDebug(app) << "NetifMonitor constructor";
    // packet monitor job
    m_netifCapture = new NetifPacketCapture(this);
    m_netifCapture->moveToThread(&m_packetMonitorThread);
    // delete monitor job after thread finished
    connect(&m_packetMonitorThread, &QThread::finished, m_netifCapture, &QObject::deleteLater);
    // start monitor job when thread started
    connect(&m_packetMonitorThread, &QThread::started, m_netifCapture, &NetifPacketCapture::startNetifMonitorJob);
}

NetifMonitor *NetifMonitor::instance()
{
    return ThreadManager::instance()->thread<NetifMonitorThread>(BaseThread::kNetifMonitorThread)->netifJobInstance();
}

NetifMonitor::~NetifMonitor()
{
    // qCDebug(app) << "NetifMonitor destructor";
    // async request monitor job quit
    m_netifCapture->requestQuit();
    // monitor thread quit
    m_packetMonitorThread.quit();
    m_packetMonitorThread.wait();
}

void NetifMonitor::startNetmonitorJob()
{
    qCDebug(app) << "Starting netmonitor job thread";
    m_packetMonitorThread.start();
}

void NetifMonitor::handleNetData()
{
    qCDebug(app) << "handleNetData loop started";
    while (!m_quitRequested.load()) {
        m_pktqLock.lock();      // +++m_pktqLock+++
        // packets available for processing when pending queue mutex unlocked
        // qCDebug(app) << "Waiting for packets...";
        m_pktqWatcher.wait(&m_pktqLock);
        // qCDebug(app) << "Woke up from wait";

        // check if quit requested again after wakeup by another thread, break the loop if need
        if (m_quitRequested.load()) {
            // qCDebug(app) << "Quit requested, breaking handleNetData loop";
            m_pktqLock.unlock();
            break;
        }

        // qCDebug(app) << "Moving" << m_pendingPackets.size() << "packets from pending queue";
        // append pending queue packets to local queue, so we dont lock the mutex for too long
        m_localPendingPackets.append(m_pendingPackets);
        m_pendingPackets.clear();
        m_pktqLock.unlock();    // ---m_pktqLock---

        // process payload queue
        while (!m_localPendingPackets.isEmpty()) {
            auto payload = m_localPendingPackets.dequeue();
            // qCDebug(app) << "Processing packet for inode" << payload->ino << "direction" << payload->direction << "size" << payload->payload;

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
                    hist->tx_bytes += (payload->payload * 2);
                    hist->tx_packets++;
                }
                // qCDebug(app) << "Updated existing stat for inode" << ino << ": rx_bytes=" << hist->rx_bytes << "tx_bytes=" << hist->tx_bytes;
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
                // qCDebug(app) << "Created new stat for inode" << stat->ino << ": rx_bytes=" << stat->rx_bytes << "tx_bytes=" << stat->tx_bytes;
            }
            m_sockIOStatMapLock.unlock();   // ---m_sockIOStatMapLock---
        }
    }
    qCDebug(app) << "handleNetData loop finished";
}

}
}
