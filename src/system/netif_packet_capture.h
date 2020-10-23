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
#ifndef NETIF_PACKET_CAPTURE_H
#define NETIF_PACKET_CAPTURE_H

#include "packet.h"

#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QMutexLocker>
#include <QSharedPointer>
#include <QQueue>
#include <QMap>
#include <QDebug>

#include <pcap.h>

#include <atomic>

void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);

struct sock_io_stat_t {
    ino_t ino; // socket inode
    qulonglong rx_bytes;
    qulonglong rx_packets;
    qulonglong tx_bytes;
    qulonglong tx_packets;
};
using SockIOStat = QSharedPointer<struct sock_io_stat_t>;

class PacketCaptureHandle;
class NetifInfoCache;
class NetifPacketCapture : public QThread
{
    Q_OBJECT
public:
    explicit NetifPacketCapture(QObject *parent = nullptr);
    ~NetifPacketCapture() override;

    inline void requestQuit()
    {
        m_quitRequested.store(true);
    }

    inline bool getSockIOStatByInode(ino_t ino, SockIOStat &stat)
    {
        bool ok = false;

        QMutexLocker lock(&m_sockIOStatMapLock);
        if (m_sockIOStatMap.contains(ino)) {
            stat = m_sockIOStatMap[ino];
            m_sockIOStatMap.remove(ino);
            ok = true;
        }
        lock.unlock();

        return ok;
    }

public Q_SLOTS:
    void run() override;
    void updateSockIOStatMap();

signals:
    void packetQueueUpdated();

private:
    friend class PacketCaptureHandle;
    friend void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);

    PacketPayloadQueue m_pendingPackets {};
    QMutex m_pktqLock {};
    PacketPayloadQueue m_localPendingPackets {}; // local cache

    QMap<ino_t, SockIOStat> m_sockIOStatMap {};
    QMutex m_sockIOStatMapLock {};

    QThread m_thread;
    PacketCaptureHandle *m_captureHandle {};

    std::atomic_bool m_quitRequested {false};
};

#endif // NETIF_PACKET_CAPTURE_H
