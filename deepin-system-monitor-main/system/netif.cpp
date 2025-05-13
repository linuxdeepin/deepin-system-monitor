// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netif.h"

#include "nl_addr.h"
#include "nl_link.h"
#include "wireless.h"
#include "nl_hwaddr.h"

#include <QProcess>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <netlink/route/link.h>
#include <netlink/addr.h>
#include <linux/sockios.h>
#include<unistd.h>
#include<sys/ioctl.h>

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


void NetifInfo::set_recv_bps(qreal recv_bps)
{
    d->recv_bps = recv_bps;
}

void NetifInfo::set_sent_bps(qreal sent_bps)
{
    d->sent_bps = sent_bps;
}

void NetifInfo::updateHWAddr(const QByteArray ifname)
{
    NLHWAddr nl_hwaddr(ifname);
    d->conn_type = nl_hwaddr.conn_type();

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

    this->updateWirelessInfo();
    this->updateBrandInfo();
    this->updateHWAddr(d->ifname);
}

void NetifInfo::updateWirelessInfo()
{
    wireless wireless1(d->ifname);
    if (wireless1.is_wireless()) {
        d->isWireless = true;
        d->iw_info->essid = wireless1.essid();
        d->iw_info->qual.qual = wireless1.link_quality();
        d->iw_info->qual.level = wireless1.signal_levle();
        d->iw_info->qual.noise = wireless1.noise_level();
        // 速率
        auto speed = getWirelessSpeed(d->ifname);
        if (speed >= 0)
            d->speed = static_cast<uint>(speed);
    } else {
        d->isWireless = false;
    }
}

void NetifInfo::updateBrandInfo()
{
    struct ifreq ifr;
    struct ethtool_cmd ecmd;

    ecmd.cmd = 0x00000001;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, d->ifname);

    ifr.ifr_data = reinterpret_cast<caddr_t>(&ecmd);
    int fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (ioctl(fd, SIOCETHTOOL, &ifr) == 0) {
        d->speed   = ecmd.speed;
    }
    close(fd);
}

double NetifInfo::getWirelessSpeed(const QString &interface) {
    QProcess process;
    process.start("iw", QStringList() << "dev" << interface << "link");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();

    // 解析发送速率，实测发现控制中心显示的速率是 tx bitrate，所以这里只捕获 tx bitrate
    QRegularExpression txRegex("tx bitrate:\\s+(\\d+\\.?\\d*)\\s+(\\w+)");
    QRegularExpressionMatch txMatch = txRegex.match(output);
    
    if (txMatch.hasMatch()) {
        double rate = txMatch.captured(1).toDouble();
        return static_cast<long>(rate);
    }
    
    return -1;  // 表示无法获取速率
}


} // namespace system
} // namespace core
