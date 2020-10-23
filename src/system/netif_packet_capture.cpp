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
#include "netif_packet_capture.h"

#include "netif_info_cache.h"
#include "netif_packet_parser.h"
#include "system_stat.h"
#include "netif_monitor.h"
#include "common/hash.h"
#include "utils.h"

#include <QDebug>
#include <QDateTime>
#include <QMutexLocker>

#include <arpa/inet.h>

// TODO: tweaking parameters, better use global flag to change settings dynamically
#define PACKET_DISPATCH_IDLE_TIME 200
#define PACKET_DISPATCH_BATCH_COUNT 64
#define PACKET_DISPATCH_QUEUE_LWAT 64
#define PACKET_DISPATCH_QUEUE_HWAT 256

#define SOCKSTAT_REFRESH_INTERVAL 2
#define IFADDRS_HASH_CACHE_REFRESH_INTERVAL 10

class PacketCaptureHandle : public QObject
{
    Q_OBJECT
public:
    explicit PacketCaptureHandle(NetifPacketCapture *capture, QObject *parent = nullptr)
        : QObject(parent)
        , m_timer(new QTimer(this))
        , m_capture(capture)
    {
        connect(m_timer, &QTimer::timeout, this, &PacketCaptureHandle::dispatchPackets);
    }

    bool initPcap()
    {
        char errbuf[PCAP_ERRBUF_SIZE] {};
        char *dev {};
        int rc = 0;

        // create & initialize
        dev = pcap_lookupdev(errbuf);
        if (!dev) {
            qDebug() << "pcap_lookupdev failed: " << errbuf;
            return false;
        }

        m_handle = pcap_create(dev, errbuf);
        if (!m_handle) {
            qDebug() << "pcap_create failed: " << errbuf;
            return false;
        }
        rc = pcap_setnonblock(m_handle, 1, errbuf);
        if (rc == -1) {
            qDebug() << "pcap_setnonblock failed: " << errbuf;
            pcap_close(m_handle);
            return false;
        }

        // pcap_compile crashes everytime without any reason, need researching...
#if 0
        char pattern[] = "TCP and UDP";
        struct bpf_program pgm;

        rc = pcap_compile(m_handle, &pgm, pattern, 1, PCAP_NETMASK_UNKNOWN);
        if (rc == -1) {
            qDebug() << "pcap_compile failed: " << pcap_geterr(m_handle);
            pcap_close(m_handle);
            return;
        }
        rc = pcap_setfilter(m_handle, &pgm);
        if (rc == -1) {
            qDebug() << "pcap_setfilter failed: " << pcap_geterr(m_handle);
            pcap_close(m_handle);
            return;
        }
#endif

        rc = pcap_activate(m_handle);
        if (rc > 0) {
            qDebug() << "pcap_activate warning: " << pcap_statustostr(rc);
        } else if (rc < 0) {
            qDebug() << "pcap_setnonblock failed: " << pcap_statustostr(rc);
            pcap_close(m_handle);
            return false;
        }
        return true;
    }

public slots:
    void cron()
    {
        bool ok = initPcap();
        if (!ok)
            return;
        m_timer->start();
    }

private slots:
    void dispatchPackets()
    {
        auto npkts = m_localPendingPackets.size();
        if (npkts > 0) {
            if (npkts >= PACKET_DISPATCH_QUEUE_LWAT && npkts < PACKET_DISPATCH_QUEUE_HWAT) {
                QMutexLocker lock(&m_capture->m_pktqLock);
                m_capture->m_pendingPackets.append(m_localPendingPackets);
                lock.unlock();

                emit m_capture->packetQueueUpdated();

                m_localPendingPackets.clear();
            } else if (npkts >= PACKET_DISPATCH_QUEUE_HWAT) {
                QMutexLocker lock(&m_capture->m_pktqLock);
                m_capture->m_pktqLock.lock();
                m_capture->m_pendingPackets.append(m_localPendingPackets);
                lock.unlock();

                emit m_capture->packetQueueUpdated();

                m_localPendingPackets.clear();
            }
        }

        time_t last_sockstat {};
        time_t last_ifaddrs_refresh {};

        while (true) {
            auto quit = QThread::currentThread()->isInterruptionRequested();
            if (quit) {
                m_timer->stop();
                break;
            }

            if (m_reInitPcapHandle.load()) {
                pcap_close(m_handle);
                bool ok = initPcap();
                if (!ok)
                    break;
                m_reInitPcapHandle.store(false);
            }

            // refresh m_sockStat cache every 2 seconds
            time_t now = time(nullptr);
            if (!last_sockstat || (now - last_sockstat) >= SOCKSTAT_REFRESH_INTERVAL) {
                m_sockStats.clear();
                SystemStat::readSockStat(m_sockStats);
                last_sockstat = now;
            }

            // refresh m_ifaddrsHashCache every 10 seconds in case user change ip address on the fly
            if (!last_ifaddrs_refresh || (now - last_ifaddrs_refresh) >= IFADDRS_HASH_CACHE_REFRESH_INTERVAL) {
                refreshIfAddrsHashCache();
                last_ifaddrs_refresh = now;
            }

            auto nr = pcap_dispatch(m_handle,
                                    PACKET_DISPATCH_BATCH_COUNT,
                                    pcap_callback,
                                    reinterpret_cast<u_char *>(this));
            if (nr == 0) {
                // no packets are available, idle this loop for a fraction second
                m_timer->start(PACKET_DISPATCH_IDLE_TIME);
                return;
            } else if (nr == -1) {
                qDebug() << "pcap_dispatch failed: " << pcap_geterr(m_handle);
                m_timer->stop();
                break;
            } else if (nr == -2) {
                // breakloop requested (can only happen inside the callback function)
                m_timer->stop();
                break;
            }
        }

        pcap_close(m_handle);
    }
    void refreshIfAddrsHashCache()
    {
        NetIFAddrsMap addrsMap;

        auto ok = SystemStat::readNetIfAddrs(addrsMap);
        if (ok) {
            NetIFAddrsMap::const_iterator it = addrsMap.constBegin();
            while (it != addrsMap.constEnd()) {
                auto ifaddr = it.value();

                uint64_t ifaddr_hash[2] {};
                if (ifaddr->family == AF_INET) {
                    char addr_str[INET_ADDRSTRLEN + 1] {};
                    inet_ntop(AF_INET, &ifaddr->addr.in4, addr_str, INET_ADDRSTRLEN);

                    utils::hash(addr_str, int(strlen(addr_str)), utils::global_seed, ifaddr_hash);
                    m_ifaddrsHashCache[ifaddr_hash[0]] = 0;

                } else if (ifaddr->family == AF_INET6) {
                    char addr6_str[INET6_ADDRSTRLEN + 1] {};
                    inet_ntop(AF_INET6, &ifaddr->addr.in6, addr6_str, INET6_ADDRSTRLEN);

                    utils::hash(addr6_str, int(strlen(addr6_str)), utils::global_seed, ifaddr_hash);
                    m_ifaddrsHashCache[ifaddr_hash[0]] = 0;
                }

                ++it;
            }
        }
    }

private:
    friend class NetifPacketCapture;
    friend void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);

    SockStatMap m_sockStats {};
    QMap<uint64_t, int> m_ifaddrsHashCache; // key: hash code generated from ifaddr; value: not used

    QTimer *m_timer {};
    pcap_t *m_handle {};
    PacketPayloadQueue m_localPendingPackets {};
    NetifPacketCapture *m_capture {};
    std::atomic<bool> m_reInitPcapHandle {false};
    char __unused__[7]; // padding
};

void pcap_callback(u_char *context, const struct pcap_pkthdr *hdr, const u_char *packet)
{
    QByteArray fmtbuf;
    uint64_t hash {}, cchash[2] {};
    QString pattern {};

    // packet payload calc
    if (!context)
        return;

    auto *handle = reinterpret_cast<PacketCaptureHandle *>(context);
    Q_ASSERT(handle != nullptr);
    auto *capture = handle->m_capture;
    Q_ASSERT(capture != nullptr);

    // calc packet payload
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();
    auto ok = NetifPacketParser::parsePacket(hdr, packet, payload);
    if (!ok)
        return;

    // calc hash from caputured packet's src:sport-dest:dport
    if (payload->sa_family == AF_INET) {
        char saddr_str[INET_ADDRSTRLEN + 1] {}, daddr_str[INET_ADDRSTRLEN + 1] {};
        inet_ntop(AF_INET, &payload->s_addr.in4, saddr_str, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &payload->d_addr.in4, daddr_str, INET_ADDRSTRLEN);

        pattern = QString("%1:%2-%3:%4").arg(saddr_str).arg(payload->s_port).arg(daddr_str).arg(payload->d_port);

        uint64_t saddr_hpair[2] {}, daddr_hpair[2] {};
        utils::hash(saddr_str, int(strlen(saddr_str)), utils::global_seed, saddr_hpair);
        utils::hash(daddr_str, int(strlen(daddr_str)), utils::global_seed, daddr_hpair);
        if (handle->m_ifaddrsHashCache.contains(saddr_hpair[0])) {
            payload->direction = kOutboundPacket;
        } else if (handle->m_ifaddrsHashCache.contains(daddr_hpair[0])) {
            payload->direction = kInboundPacket;
        } else {
            return;
        }

    } else if (payload->sa_family == AF_INET6) {
        char saddr6_str[INET6_ADDRSTRLEN + 1] {}, daddr6_str[INET6_ADDRSTRLEN + 1] {};
        inet_ntop(AF_INET6, &payload->s_addr.in6, saddr6_str, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &payload->d_addr.in6, daddr6_str, INET6_ADDRSTRLEN);

        pattern = QString("%1:%2-%3:%4").arg(saddr6_str).arg(payload->s_port).arg(daddr6_str).arg(payload->d_port);

        uint64_t saddr6_hpair[2] {}, daddr6_hpair[2] {};
        utils::hash(saddr6_str, int(strlen(saddr6_str)), utils::global_seed, saddr6_hpair);
        utils::hash(daddr6_str, int(strlen(daddr6_str)), utils::global_seed, daddr6_hpair);
        if (handle->m_ifaddrsHashCache.contains(saddr6_hpair[0])) {
            payload->direction = kOutboundPacket;
        } else if (handle->m_ifaddrsHashCache.contains(daddr6_hpair[0])) {
            payload->direction = kInboundPacket;
        } else {
            return;
        }

    } else {
        // unexpected here
    }

    fmtbuf = pattern.toLocal8Bit();
    utils::hash(fmtbuf.constData(), fmtbuf.length(), utils::global_seed, cchash);
    hash = cchash[0];

    // get ino from map
    if (handle->m_sockStats.contains(hash)) {
        QMultiMap<uint64_t, SockStat>::const_iterator it = handle->m_sockStats.find(hash);
        while (it != handle->m_sockStats.cend() && it.key() == hash) {
            // TODO: UDP traffic identify method refine
            // UDP socks may have same kernel hash slot (sl: hash generated with same src:port + dest:port),
            // which means there's no way to distinguish which packet sent/received by which socket, what
            // makes it very tricky to get the real UDP traffic for specific process, we assume
            // socks with same sl are created by same process for temporary, need a much fine way to
            // distinguish the traffic at a later time.
            payload->ino = it.value()->ino;
            break;
        }
    } else {
        // no matching sockets in /proc tcp/udp table, which means we cant grab inode from socket table,
        // the only thing we can do here is ignore this packet.
        return;
    }

    handle->m_localPendingPackets.enqueue(payload);
    auto npkts = handle->m_localPendingPackets.size();
    if (npkts >= PACKET_DISPATCH_QUEUE_LWAT && npkts < PACKET_DISPATCH_QUEUE_HWAT) {
        QMutexLocker lock(&capture->m_pktqLock);
        capture->m_pendingPackets.append(handle->m_localPendingPackets);
        lock.unlock();

        handle->m_localPendingPackets.clear();

        emit capture->packetQueueUpdated();
    } else if (npkts >= PACKET_DISPATCH_QUEUE_HWAT) {
        QMutexLocker lock(&capture->m_pktqLock);
        capture->m_pendingPackets.append(handle->m_localPendingPackets);
        capture->m_pktqLock.unlock();
        lock.unlock();

        handle->m_localPendingPackets.clear();

        emit capture->packetQueueUpdated();
    }
}

NetifPacketCapture::NetifPacketCapture(QObject *parent)
    : QThread(parent)
    , m_captureHandle(new PacketCaptureHandle(this))
{
    m_captureHandle->moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, m_captureHandle, &PacketCaptureHandle::cron);
    connect(&m_thread, &QThread::finished, this, &QObject::deleteLater);
    connect(this, &NetifPacketCapture::packetQueueUpdated, this, &NetifPacketCapture::updateSockIOStatMap);

    auto *cache = new NetifInfoCache(this);
    connect(cache, &NetifInfoCache::linkStateChanged, this, [=]() {
        m_captureHandle->m_reInitPcapHandle.store(true);
    });

    m_thread.start();
}

NetifPacketCapture::~NetifPacketCapture()
{
    m_thread.requestInterruption();
    m_thread.quit();
    m_thread.wait();
}

void NetifPacketCapture::run()
{
    exec();
}

void NetifPacketCapture::updateSockIOStatMap()
{
    m_pktqLock.lock(); // +++m_pktqLock+++
    m_localPendingPackets.append(m_pendingPackets);
    m_pendingPackets.clear();
    m_pktqLock.unlock(); // ---m_pktqLock---

    // process payload queue
    while (!m_localPendingPackets.isEmpty()) {
        auto payload = m_localPendingPackets.dequeue();

        // lock sockiostatmap
        m_sockIOStatMapLock.lock(); // +++m_sockIOStatMapLock+++

        // sum up sock io stat by inode
        auto ino = payload->ino;
        if (m_sockIOStatMap.contains(ino)) {
            auto &hist = m_sockIOStatMap[ino];
            if (payload->direction == kInboundPacket) {
                hist->rx_bytes += payload->payload;
                hist->rx_packets++;
            } else if (payload->direction == kOutboundPacket) {
                hist->tx_bytes += payload->payload;
                hist->tx_packets++;
            }
        } else {
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

        // unlock sockiostatmap
        m_sockIOStatMapLock.unlock(); // ---m_sockIOStatMapLock---
    }
}

#include "netif_packet_capture.moc"
