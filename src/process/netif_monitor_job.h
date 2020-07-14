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

#ifndef NETIF_MONITOR_JOB_H
#define NETIF_MONITOR_JOB_H

#include "netif_packet_parser.h"
#include "system_stat.h"

#include <atomic>

class NetifMonitor;
class QTimer;
class QThread;

void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);

class NetifMonitorJob : public QObject
{
    Q_OBJECT

public:
    explicit NetifMonitorJob(NetifMonitor *, QObject *parent = nullptr);

    inline void requestQuit()
    {
        m_quitRequested.store(true);
    }

public Q_SLOTS:
    void startMonitorJob();

private:
    void dispatchPackets();
    void refreshIfAddrsHashCache();

private:
    SockStatMap     m_sockStats {};
    QMap<uint64_t, int> m_ifaddrsHashCache; // key: hash code generated from ifaddr; value: not used

    QTimer *m_timer {};

    NetifMonitor       *m_netifMonitor         {};
    pcap_t             *m_handle               {};
    PacketPayloadQueue  m_localPendingPackets  {};

    std::atomic_bool m_quitRequested {false};

    friend void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);
};

#endif // NETIF_MONITOR_JOB_H
