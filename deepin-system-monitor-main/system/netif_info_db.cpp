// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netif_info_db.h"
#include "ddlog.h"

#include "nl_link.h"
#include "nl_addr.h"
#include "netlink.h"

#include <QReadLocker>
#include <QWriteLocker>
#include "common/thread_manager.h"
#include "netif_monitor_thread.h"
#include "system/sys_info.h"

#include <memory>
using namespace common::core;
using namespace DDLog;

namespace core {
namespace system {

NetifInfoDB::NetifInfoDB()
    : m_netlink(new Netlink())
{
    qCDebug(app) << "NetifInfoDB constructor";
}

void NetifInfoDB::update_addr()
{
    qCDebug(app) << "Updating network addresses...";
    AddrIterator iter = m_netlink->addrIterator();
    m_addrIpv4DB.clear();
    m_addrIpv6DB.clear();

    while (iter.hasNext()) {
        auto it = iter.next();
        if (it->family() == AF_INET) {
            std::shared_ptr<struct inet_addr4_t> ip4net = std::make_shared<struct inet_addr4_t>();
            it->local();
            ip4net->family = it->family();
            ip4net->addr = it->netaddr();
            ip4net->mask = it->netmask();
            ip4net->bcast = it->broadcast();
            m_addrIpv4DB.insert(it->ifindex(), ip4net);
        } else if (it->family() == AF_INET6) {
            std::shared_ptr<struct inet_addr6_t> ip6net = std::make_shared<struct inet_addr6_t>();
            it->local();
            ip6net->family = it->family();
            ip6net->addr = it->netaddr();
            ip6net->scope = it->scope();
            ip6net->prefixlen = it->prefixlen();
            m_addrIpv6DB.insert(it->ifindex(), ip6net);
        }
    }
    qCDebug(app) << "Finished updating addresses. Found" << m_addrIpv4DB.size() << "IPv4 and" << m_addrIpv6DB.size() << "IPv6 addresses.";
}
// 更新网络信息
void NetifInfoDB::update_netif_info()
{
    qCDebug(app) << "Updating network interface info...";
    LinkIterator iter = m_netlink->linkIterator();
    QMap<QByteArray, NetifInfoPtr> old_infoDB = m_infoDB;

    timevalList[kLastStat] = timevalList[kCurrentStat];
    timevalList[kCurrentStat] = SysInfo::instance()->uptime();

    m_infoDB.clear();
    while (iter.hasNext()) {
        auto it = iter.next();

        if (it->ifname() == "lo") {
            qCDebug(app) << "Skipping loopback interface";
            continue;
        }
        NetifInfoPtr item = std::make_shared<NetifInfo>();
        item->updateLinkInfo(it.get());
        item->updateAddr4Info(m_addrIpv4DB.values(it->ifindex()));
        item->updateAddr6Info(m_addrIpv6DB.values(it->ifindex()));

        // 更新速率
        if (old_infoDB.contains(it->addr())) {
            auto old_item = old_infoDB.value(it->addr());
            // receive increment between interval
            auto rxdiff = (item->rxBytes() > old_item->rxBytes()) ? (item->rxBytes() - old_item->rxBytes()) : 0;
            // transfer increment between interval
            auto txdiff = (item->txBytes() > old_item->txBytes()) ? (item->txBytes() - old_item->txBytes()) : 0;

            timeval cur_time = timevalList[kCurrentStat];
            timeval prev_time = timevalList[kLastStat];

            auto ltime = prev_time.tv_sec + prev_time.tv_usec * 1. / 1000000;
            auto rtime = cur_time.tv_sec + cur_time.tv_usec * 1. / 1000000;
            auto interval = (rtime > ltime) ? (rtime - ltime) : 1;
            qreal recv_bps = rxdiff / interval;   // Bps
            qreal sent_bps = txdiff / interval;
            item->set_recv_bps(recv_bps);
            item->set_sent_bps(sent_bps);
            // qCDebug(app) << "Updated rate for" << it->ifname() << ": recv=" << recv_bps << "B/s, sent=" << sent_bps << "B/s";
        }

        m_infoDB.insert(it->addr(), item);
    }
    qCDebug(app) << "Finished updating network interface info. Found" << m_infoDB.size() << "interfaces.";
}
void NetifInfoDB::update()
{
    qCDebug(app) << "Updating NetifInfoDB...";
    this->update_addr();
    this->update_netif_info();
    qCDebug(app) << "NetifInfoDB update finished.";
}

} // namespace system
} // namespace core
