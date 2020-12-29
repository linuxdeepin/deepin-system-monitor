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
#include "netif.h"

#include "nl_addr.h"
#include "nl_link.h"

#include <netlink/route/link.h>
#include <netlink/addr.h>

namespace core {
namespace system {

NetifInfo::NetifInfo()
    : d(new NetifInfoPrivate())
{
}
NetifInfo::NetifInfo(const NetifInfo &other)
    : d(other.d)
{
}
NetifInfo &NetifInfo::operator=(const NetifInfo &rhs)
{
    if (this == &rhs)
        return *this;

    d = rhs.d;
    return *this;
}
NetifInfo::~NetifInfo()
{
}

void NetifInfo::updateLinkInfo(const NLLink *link)
{
    if (!link)
        return;

    d->index = link->ifindex();
    d->ifname = link->ifname();
    d->alias = link->alias();

    d->mtu = link->mtu();
    d->flags = link->flags();
    d->arp_type = link->arp_type();
    d->txqlen = link->txqlen();
    d->carrier_changes = link->carrier_changes();
    d->carrier = link->carrier();
    d->oper_stat = link->oper_stat();
    d->link_mode = link->link_mode();
    d->hw_addr = link->addr();
    d->hw_bcast = link->bcast();

    // ====link params====

    QByteArray conn_type;
    QString brand;

    // ====stats====

    d->rx_packets = link->rx_packets();
    d->rx_bytes = link->rx_bytes();
    d->rx_errors = link->rx_errors();
    d->rx_dropped = link->rx_dropped();
    d->rx_fifo = link->rx_fifo();
    d->rx_frame = link->rx_frame();

    d->tx_packets = link->tx_packets();
    d->tx_bytes = link->tx_bytes();
    d->tx_errors = link->tx_errors();
    d->tx_dropped = link->tx_dropped();
    d->tx_fifo = link->tx_fifo();
    d->tx_carrier = link->tx_carrier();

    d->collisions = link->collisions();
}

void NetifInfo::updateAddrInfo(const QList<INetAddr> &addrList)
{
}

} // namespace system
} // namespace core
