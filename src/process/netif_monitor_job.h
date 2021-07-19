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

#ifndef NETIF_MONITOR_JOB_H
#define NETIF_MONITOR_JOB_H

#include "netif_packet_parser.h"
#include "system_stat.h"

#include <atomic>

class NetifMonitor;
class QTimer;
class QThread;

/**
 * @brief Packet process callback function
 * @param context User context
 * @param hdr  Packet header
 * @param packet Packet
 */
void pcap_callback(u_char *context, const struct pcap_pkthdr *hdr, const u_char *packet);

/**
 * @brief Packet monitor job class
 */
class NetifMonitorJob : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Monitor job constructor
     * @param parent Parent object
     */
    explicit NetifMonitorJob(NetifMonitor *, QObject *parent = nullptr);

    /**
     * @brief Request monitor job quit
     */
    inline void requestQuit()
    {
        m_quitRequested.store(true);
    }

public Q_SLOTS:
    /**
     * @brief Start monitor job
     */
    void startMonitorJob();

private:
    /**
     * @brief Packet dispatch handler
     */
    void dispatchPackets();
    /**
     * @brief Refresh network interface address hash cache
     */
    void refreshIfAddrsHashCache();

private:
    // socket io stat cache
    SockStatMap     m_sockStats {};
    // network interface name hash cache
    QMap<uint64_t, int> m_ifaddrsHashCache; // key: hash code generated from ifaddr; value: not used

    // packet dispatch timer
    QTimer *m_timer {};

    // network interface monitor
    NetifMonitor       *m_netifMonitor         {};
    // pcap handler instance
    pcap_t             *m_handle               {};
    // pcap all dev
    pcap_if_t          *m_alldev               {};
    // local pending packet queue
    PacketPayloadQueue  m_localPendingPackets  {};

    // request quit atomic flag
    std::atomic_bool m_quitRequested {false};

    friend void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);
};

#endif // NETIF_MONITOR_JOB_H
