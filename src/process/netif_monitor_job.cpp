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

#include "netif_monitor_job.h"

#include "system_stat.h"
#include "netif_monitor.h"
#include "common/hash.h"
#include "utils.h"

#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QThread>

#include <pcap.h>

#include <arpa/inet.h>

// TODO: tweaking parameters, better use global flag to change settings dynamically
#define PACKET_DISPATCH_IDLE_TIME 200 // pcap dispatch interval
#define PACKET_DISPATCH_BATCH_COUNT 64 // packets to process in a batch
#define PACKET_DISPATCH_QUEUE_LWAT 64 // queue low water mark
#define PACKET_DISPATCH_QUEUE_HWAT 256 // queue high water mark

#define SOCKSTAT_REFRESH_INTERVAL 2 // socket stat refresh interval (2 seconds)
#define IFADDRS_HASH_CACHE_REFRESH_INTERVAL 10 // socket ifaddrs cache refresh interval (10 seconds)

// monitor job constructor
NetifMonitorJob::NetifMonitorJob(NetifMonitor *netifMonitor, QObject *parent) :
    QObject(parent),
    m_netifMonitor(netifMonitor)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    // dispatch packets on timerout signal
    connect(m_timer, &QTimer::timeout, this, &NetifMonitorJob::dispatchPackets);
}

//
void NetifMonitorJob::startMonitorJob()
{
    char errbuf[PCAP_ERRBUF_SIZE] {};
    char *dev {};
    int rc = 0;

    // the pcap_lookupdev function only return default first device, so we should find netx device to grab the pcap
    if (PCAP_ERROR == pcap_findalldevs(&m_alldev, errbuf)) {
        qInfo() << "pcap_findalldevs failed: " << errbuf;
        return;
    }
    QString deviceName;
    for (auto device = m_alldev; device != nullptr; device = m_alldev->next) {
        deviceName = QString(device->name);
        if (deviceName.contains(QString("uengine"), Qt::CaseInsensitive)) {
            // if device name is 'uengine' we need skip this device
            continue;
        } else {
            break;
        }
    }

//    // create & initialize pcap dev
//    dev = pcap_lookupdev(errbuf);
//    if (!dev) {
//        qDebug() << "pcap_lookupdev failed: " << errbuf;
//        return;
//    }

    // create pcap handler
    m_handle = pcap_create(deviceName.toStdString().c_str(), errbuf);
//    m_handle = pcap_create(dev, errbuf);
    if (!m_handle) {
        qDebug() << "pcap_create failed: " << errbuf;
        // need free all pcap device
        pcap_freealldevs(m_alldev);
        return;
    }
    // set non block dispatch mode
    rc = pcap_setnonblock(m_handle, 1, errbuf);
    if (rc == -1) {
        qDebug() << "pcap_setnonblock failed: " << errbuf;
        pcap_close(m_handle);
        // need free all pcap device
        pcap_freealldevs(m_alldev);
        return;
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

    // activate pcap handler
    rc = pcap_activate(m_handle);
    if (rc > 0) {
        qDebug() << "pcap_activate warning: " << pcap_statustostr(rc);
    } else if (rc < 0) {
        qDebug() << "pcap_setnonblock failed: " << pcap_statustostr(rc);
        pcap_close(m_handle);
        // need free all pcap device
        pcap_freealldevs(m_alldev);
        return;
    }

    // start packet dispatch job
    m_timer->start();
}

// packet processing callback
void pcap_callback(u_char *context, const struct pcap_pkthdr *hdr, const u_char *packet)
{
    QByteArray fmtbuf;
    uint64_t hash {}, cchash[2] {};
    QString pattern {};

    // packet payload calc
    if (!context)
        return;

    // get monitor & monitor job instance from user context
    auto *netifMonitorJob = reinterpret_cast<NetifMonitorJob *>(context);
    Q_ASSERT(netifMonitorJob != nullptr);
    auto *netifMonitor = netifMonitorJob->m_netifMonitor;
    Q_ASSERT(netifMonitor != nullptr);

    // parse packet & calculate payload
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();
    auto ok = NetifPacketParser::parsePacket(hdr, packet, payload);
    if (!ok)
        return;

    // calc hash from caputured packet's src:sport-dest:dport
    if (payload->sa_family == AF_INET) {
        // ipv4 packet payload
        char saddr_str[INET_ADDRSTRLEN + 1] {}, daddr_str[INET_ADDRSTRLEN + 1] {};
        // convert ipv4 source & destination address to string format
        inet_ntop(AF_INET, &payload->s_addr.in4, saddr_str, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &payload->d_addr.in4, daddr_str, INET_ADDRSTRLEN);

        // pattern that matches kernel sock stat table
        pattern = QString("%1:%2-%3:%4").arg(saddr_str).arg(payload->s_port).arg(daddr_str).arg(payload->d_port);

        uint64_t saddr_hpair[2] {}, daddr_hpair[2] {};
        // generate source address hash
        util::common::hash(saddr_str, int(strlen(saddr_str)), util::common::global_seed, saddr_hpair);
        // generate destination address hash
        util::common::hash(daddr_str, int(strlen(daddr_str)), util::common::global_seed, daddr_hpair);
        if (netifMonitorJob->m_ifaddrsHashCache.contains(saddr_hpair[0])) {
            payload->direction = kOutboundPacket;
        } else if (netifMonitorJob->m_ifaddrsHashCache.contains(daddr_hpair[0])) {
            payload->direction = kInboundPacket;
        } else {
            return;
        }

    } else if (payload->sa_family == AF_INET6) {
        // ipv6 packet payload
        char saddr6_str[INET6_ADDRSTRLEN + 1] {}, daddr6_str[INET6_ADDRSTRLEN + 1] {};
        inet_ntop(AF_INET6, &payload->s_addr.in6, saddr6_str, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &payload->d_addr.in6, daddr6_str, INET6_ADDRSTRLEN);

        // pattern that matches kernel sock stat table
        pattern = QString("%1:%2-%3:%4").arg(saddr6_str).arg(payload->s_port).arg(daddr6_str).arg(payload->d_port);

        uint64_t saddr6_hpair[2] {}, daddr6_hpair[2] {};
        // generate source address hash
        util::common::hash(saddr6_str, int(strlen(saddr6_str)), util::common::global_seed, saddr6_hpair);
        // generate destination address hash
        util::common::hash(daddr6_str, int(strlen(daddr6_str)), util::common::global_seed, daddr6_hpair);
        if (netifMonitorJob->m_ifaddrsHashCache.contains(saddr6_hpair[0])) {
            payload->direction = kOutboundPacket;
        } else if (netifMonitorJob->m_ifaddrsHashCache.contains(daddr6_hpair[0])) {
            payload->direction = kInboundPacket;
        } else {
            return;
        }

    } else {
        // unexpected here
    }

    fmtbuf = pattern.toLocal8Bit();
    util::common::hash(fmtbuf.constData(), fmtbuf.length(), util::common::global_seed, cchash);
    hash = cchash[0];

    // get ino from map
    if (netifMonitorJob->m_sockStats.contains(hash)) {
        QMultiMap<uint64_t, SockStat>::const_iterator it = netifMonitorJob->m_sockStats.find(hash);
        while (it != netifMonitorJob->m_sockStats.cend() && it.key() == hash) {
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

    netifMonitorJob->m_localPendingPackets.enqueue(payload);
    auto npkts = netifMonitorJob->m_localPendingPackets.size();
    if (npkts >= PACKET_DISPATCH_QUEUE_LWAT && npkts < PACKET_DISPATCH_QUEUE_HWAT) {
        // if dispatch queue is between low & high water mark, then we can just use try lock in relax way
        if (netifMonitor->m_pktqLock.tryLock()) {
            netifMonitor->m_pendingPackets.append(netifMonitorJob->m_localPendingPackets);
            netifMonitor->m_pktqLock.unlock();

            // clear local pending queue after move packets to monitor instance's queue
            netifMonitorJob->m_localPendingPackets.clear();

            netifMonitor->m_pktqWatcher.wakeAll();
        }
    } else if (npkts >= PACKET_DISPATCH_QUEUE_HWAT) {
        // otherwise we forcefully use lock instead
        netifMonitor->m_pktqLock.lock();
        netifMonitor->m_pendingPackets.append(netifMonitorJob->m_localPendingPackets);
        netifMonitor->m_pktqLock.unlock();

        // clear local pending queue after move packets to monitor instance's queue
        netifMonitorJob->m_localPendingPackets.clear();

        netifMonitor->m_pktqWatcher.wakeAll();
    }
}

// dispatch packet handler
void NetifMonitorJob::dispatchPackets()
{
    // check pending packets before dispatching packets
    auto npkts = m_localPendingPackets.size();
    if (npkts > 0) {
        if (npkts >= PACKET_DISPATCH_QUEUE_LWAT && npkts < PACKET_DISPATCH_QUEUE_HWAT) {
            // pending packets between low & high water mark
            if (m_netifMonitor->m_pktqLock.tryLock()) {
                m_netifMonitor->m_pendingPackets.append(m_localPendingPackets);
                m_netifMonitor->m_pktqLock.unlock();

                m_netifMonitor->m_pktqWatcher.wakeAll();

                m_localPendingPackets.clear();
            }
        } else if (npkts >= PACKET_DISPATCH_QUEUE_HWAT) {
            // acquire lock forcefully if too many packets pending
            m_netifMonitor->m_pktqLock.lock();
            m_netifMonitor->m_pendingPackets.append(m_localPendingPackets);
            m_netifMonitor->m_pktqLock.unlock();

            m_netifMonitor->m_pktqWatcher.wakeAll();

            m_localPendingPackets.clear();
        }
    }

    time_t last_sockstat {};
    time_t last_ifaddrs_refresh {};

    do {
        // quit requested, break the loop then
        auto quit = m_quitRequested.load();
        if (quit) {
            m_timer->stop();
            break;
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

        // start packet dispatching
        auto nr = pcap_dispatch(m_handle,
                                PACKET_DISPATCH_BATCH_COUNT,
                                pcap_callback,
                                reinterpret_cast<u_char *>(this));
        if (nr == 0) {
            // no packets are available, idle this loop for a fraction second
            m_timer->start(PACKET_DISPATCH_IDLE_TIME);
            return;
        } else if (nr == -1) {
            // error occurred while processing packets
            qDebug() << "pcap_dispatch failed: " << pcap_geterr(m_handle);
            m_timer->stop();
            break;
        } else if (nr == -2) {
            // breakloop requested (can only happen inside the callback function)
            m_timer->stop();
            break;
        }
    } while (true);

    // close pcap handle
    pcap_close(m_handle);
}

// refresh network interface hash cache
void NetifMonitorJob::refreshIfAddrsHashCache()
{
    NetIFAddrsMap addrsMap;

    // get network interface map
    auto ok = SystemStat::readNetIfAddrs(addrsMap);
    if (ok) {
        NetIFAddrsMap::const_iterator it = addrsMap.constBegin();
        // process each address in map
        while (it != addrsMap.constEnd()) {
            auto ifaddr = it.value();

            uint64_t ifaddr_hash[2] {};
            if (ifaddr->family == AF_INET) {
                char addr_str[INET_ADDRSTRLEN + 1] {};
                // convert to ipv4 format string
                inet_ntop(AF_INET, &ifaddr->addr.in4, addr_str, INET_ADDRSTRLEN);

                // calculate hash
                util::common::hash(addr_str, int(strlen(addr_str)), util::common::global_seed, ifaddr_hash);
                m_ifaddrsHashCache[ifaddr_hash[0]] = 0;

            } else if (ifaddr->family == AF_INET6) {
                char addr6_str[INET6_ADDRSTRLEN + 1] {};
                // convert to ipv6 format string
                inet_ntop(AF_INET6, &ifaddr->addr.in6, addr6_str, INET6_ADDRSTRLEN);

                // calculate hash
                util::common::hash(addr6_str, int(strlen(addr6_str)), util::common::global_seed, ifaddr_hash);
                m_ifaddrsHashCache[ifaddr_hash[0]] = 0;
            }

            ++it;
        }
    }
}
