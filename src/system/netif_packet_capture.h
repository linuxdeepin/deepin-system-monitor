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

#include <QObject>
#include "packet.h"
#include <QTimer>
#include <QMap>


namespace core {
namespace system {
class NetifMonitor;
class NetifPacketCapture : public QObject
{
    Q_OBJECT
public:
    explicit NetifPacketCapture(NetifMonitor *netInfmontor, QObject *parent = nullptr);
    inline void requestQuit()
    {
        m_quitRequested.store(true);
    }
    /**
     * @brief Packet dispatch handler
     */
    void dispatchPackets();
    pcap_t  *getHandle() { return m_handle;}
signals:

public slots:
    /**
     * @brief Start monitor job
     */
    void startNetifMonitorJob();
private:

    /**
     * @brief Refresh network interface address hash cache
     */
    void refreshIfAddrsHashCache();
private:
    // socket io stat cache
    SockStatMap     m_sockStats {};
    // network interface name hash cache
    QMap<uint64_t, int> m_ifaddrsHashCache; // key: hash code generated from ifaddr; value: not used

    // network interface monitor
    NetifMonitor       *m_netifMonitor         {};
    // pcap handler instance
    pcap_t             *m_handle               {};
    // local pending packet queue
    PacketPayloadQueue  m_localPendingPackets  {};

    // request quit atomic flag
    std::atomic_bool m_quitRequested {false};
    bool go {false};

    // packet dispatch timer
    QTimer *m_timer {};

    friend void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);

};

} // namespace system
} // namespace core

#endif // NETIF_PACKET_CAPTURE_H
