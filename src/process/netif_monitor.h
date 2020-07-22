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
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NETIF_MONITOR_H
#define NETIF_MONITOR_H

#include "netif_packet_parser.h"

#include <QThread>
#include <QSharedPointer>
#include <QMap>
#include <QMutex>
#include <QWaitCondition>
#include <QMutexLocker>

class NetifMonitorJob;

struct sock_io_stat_t {
    ino_t      ino;        // socket inode
    qulonglong rx_bytes;
    qulonglong rx_packets;
    qulonglong tx_bytes;
    qulonglong tx_packets;
};

using SockIOStat = QSharedPointer<struct sock_io_stat_t>;

class NetifMonitor : public QThread
{
    Q_OBJECT

public:
    explicit NetifMonitor(QObject *parent = nullptr);
    ~NetifMonitor() override;

    inline void requestQuit()
    {
        m_quitRequested.store(true);
        m_pktqWatcher.wakeAll();
    }

    inline bool getSockIOStatByInode(ino_t ino, SockIOStat &stat)
    {
        bool ok = false;

        m_sockIOStatMapLock.lock();

        if (m_sockIOStatMap.contains(ino)) {
            stat = m_sockIOStatMap[ino];
            m_sockIOStatMap.remove(ino);
            ok = true;
        }

        m_sockIOStatMapLock.unlock();

        return ok;
    }

    void run() override;

public Q_SLOTS:

private:
    PacketPayloadQueue  m_pendingPackets        {};
    QMutex              m_pktqLock              {};
    QWaitCondition      m_pktqWatcher           {};
    PacketPayloadQueue  m_localPendingPackets   {}; // local cache

    QMap<ino_t, SockIOStat> m_sockIOStatMap     {};
    QMutex                  m_sockIOStatMapLock {};

    QThread             m_packetMonitorThread;
    NetifMonitorJob    *m_packetMonitorJob     {};

    std::atomic_bool m_quitRequested {false};

    friend void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);
    friend class NetifMonitorJob;
};

#endif // NETIF_MONITOR_H
