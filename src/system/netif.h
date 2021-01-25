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
#ifndef NETIF_H
#define NETIF_H

#include "private/netif_p.h"

#include <QByteArray>
#include <QSharedDataPointer>
#include <QList>

#include <netlink/route/link.h>

#include <memory>

namespace core {
namespace system {

struct inet_addr_t {
    int family;
    QByteArray addr;
};
struct inet_addr4_t : public inet_addr_t {
    QByteArray mask;
    QByteArray bcast;
};
struct inet_addr6_t : public inet_addr_t {
    int prefixlen;
    int scope;
};
using INetAddr = std::shared_ptr<struct inet_addr_t>;

class NetifInfoData;
class NetifInfoDB;
class NLLink;
class NLAddr;
class wireless;

class NetifInfo
{
public:
    NetifInfo();
    NetifInfo(const NetifInfo &other);
    NetifInfo &operator=(const NetifInfo &rhs);
    ~NetifInfo();

    // link params
    int index() const;
    QByteArray ifname() const;
    QByteArray alias() const;
    uint mtu() const;
    uint flags() const;
    uint arpType() const;
    uint txqlen() const;
    quint8 operState() const;
    quint8 linkMode() const;
    quint8 carrier() const;
    uint carrierChanges() const;
    QByteArray connectionType() const;
    QByteArray linkAddress() const;
    QByteArray linkBroadcast() const;
    QString brand() const;

    // wireless link only
    QByteArray essid() const;
    uint8_t linkQuality() const;
    uint8_t signalLevel() const;
    uint8_t noiseLevel() const;

    // link stats
    qulonglong rxPackets() const;
    qulonglong rxBytes() const;
    qulonglong rxErrors() const;
    qulonglong rxDropped() const;
    qulonglong rxFIFO() const;
    qulonglong rxFrame() const;

    qulonglong txPackets() const;
    qulonglong txBytes() const;
    qulonglong txErrors() const;
    qulonglong txDropped() const;
    qulonglong txFIFO() const;
    qulonglong txCarrier() const;

    qulonglong collisions() const;

    // address list
    QList<INetAddr> addrList() const;
    qreal m_recv_bps = 0;             // 接收速度
    qreal m_sent_bps = 0;             // 发送速度
    qreal recv_bps();
    qreal sent_bps();
    qreal set_recv_bps(qreal recv_bps);
    qreal set_sent_bps(qreal sent_bps);

protected:
    void updateLinkInfo(const NLLink *link);
    void updateAddrInfo(const QList<INetAddr> &addrList);
    void updateWirelessInfo(QByteArray ifname); // ioctl
    void updateBrandInfo(); // udev

private:
    QSharedDataPointer<NetifInfoPrivate> d;

    friend class NetifInfoDB;
};

inline int NetifInfo::index() const
{
    return d->index;
}

inline QByteArray NetifInfo::ifname() const
{
    return d->ifname;
}

inline QByteArray NetifInfo::alias() const
{
    return d->alias;
}

inline uint NetifInfo::mtu() const
{
    return d->mtu;
}

inline uint NetifInfo::flags() const
{
    return d->flags;
}

inline uint NetifInfo::arpType() const
{
    return d->arp_type;
}

inline uint NetifInfo::txqlen() const
{
    return d->txqlen;
}

inline quint8 NetifInfo::operState() const
{
    return d->oper_stat;
}

inline quint8 NetifInfo::linkMode() const
{
    return d->link_mode;
}

inline quint8 NetifInfo::carrier() const
{
    return d->carrier;
}

inline uint NetifInfo::carrierChanges() const
{
    return d->carrier_changes;
}

inline QByteArray NetifInfo::connectionType() const
{
    return d->conn_type;
}

inline QByteArray NetifInfo::linkAddress() const
{
    return d->hw_addr;
}

inline QByteArray NetifInfo::linkBroadcast() const
{
    return d->hw_bcast;
}

inline QString NetifInfo::brand() const
{
    return d->brand;
}

inline QByteArray NetifInfo::essid() const
{
    if (d->iw_info)
        return d->iw_info->essid;
    else
        return {};
}

inline uint8_t NetifInfo::linkQuality() const
{
    if (d->iw_info)
        return d->iw_info->qual.qual;
    else
        return 0;
}

inline uint8_t NetifInfo::signalLevel() const
{
    if (d->iw_info)
        return d->iw_info->qual.level;
    else
        return 0;
}

inline uint8_t NetifInfo::noiseLevel() const
{
    if (d->iw_info)
        return d->iw_info->qual.noise;
    else
        return 0;
}

inline qulonglong NetifInfo::rxPackets() const
{
    return d->rx_packets;
}

inline qulonglong NetifInfo::rxBytes() const
{
    return d->rx_bytes;
}

inline qulonglong NetifInfo::rxErrors() const
{
    return d->rx_errors;
}

inline qulonglong NetifInfo::rxDropped() const
{
    return d->rx_dropped;
}

inline qulonglong NetifInfo::rxFIFO() const
{
    return d->rx_fifo;
}

inline qulonglong NetifInfo::rxFrame() const
{
    return d->rx_frame;
}

inline qulonglong NetifInfo::txPackets() const
{
    return d->tx_packets;
}

inline qulonglong NetifInfo::txBytes() const
{
    return d->tx_bytes;
}

inline qulonglong NetifInfo::txErrors() const
{
    return d->tx_errors;
}

inline qulonglong NetifInfo::txDropped() const
{
    return d->tx_dropped;
}

inline qulonglong NetifInfo::txFIFO() const
{
    return d->tx_fifo;
}

inline qulonglong NetifInfo::txCarrier() const
{
    return d->tx_carrier;
}

inline qulonglong NetifInfo::collisions() const
{
    return d->collisions;
}

inline QList<INetAddr> NetifInfo::addrList() const
{
    return d->addrs;
}

} // namespace system
} // namespace core

Q_DECLARE_TYPEINFO(core::system::NetifInfo, Q_MOVABLE_TYPE);

#endif // NETIF_H
