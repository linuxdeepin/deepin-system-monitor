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
#include "netif_info_db.h"

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
namespace core {
namespace system {

NetifInfoDB::NetifInfoDB()
    : m_netlink(new Netlink())
    , m_addrDB {}
    , m_infoDB {}
{
}

void NetifInfoDB::update_addr()
{
    // iterator
    // 查找所有的addr
    AddrIterator iter = m_netlink->addrIterator();
    m_addrDB.clear();
    while (iter.hasNext()) {
        auto it = iter.next();
        if (it->family() == AF_INET) {
            std::shared_ptr<struct inet_addr4_t> ip4net = std::make_shared<struct inet_addr4_t>();
            ip4net->family = it->family();
            ip4net->addr = it->local();
            ip4net->mask = it->multicast();
            ip4net->bcast = it->broadcast();
            m_addrDB.insert(it->local(), ip4net);
        } else if (it->family() == AF_INET6) {
            std::shared_ptr<struct inet_addr6_t> temp = std::make_shared<struct inet_addr6_t>();
            temp->family = it->family();
            temp->addr = it->local();
            temp->scope = it->scope();
            temp->prefixlen = it->prefixlen();
            m_addrDB.insert(it->local(), temp);
        }
    }

}
// 更新网络信息
void NetifInfoDB::update_netif_info()
{
    LinkIterator iter = m_netlink->linkIterator();
    QMap<QByteArray, NetifInfo> old_infoDB = m_infoDB;
    m_infoDB.clear();
    while (iter.hasNext()) {
        auto it = iter.next();

        if (it->ifname() == "lo") {
            continue;
        }
        NetifInfo  item;
        item.updateLinkInfo(it.get());
        item.updateAddrInfo(m_addrDB.values(it->addr()));
        // 更新速率

        if (old_infoDB.contains(it->addr())) {
            timevalList[kLastStat] = timevalList[kCurrentStat];
            timevalList[kCurrentStat] = SysInfo::instance()->uptime();

            auto old_item = old_infoDB.value(it->addr());
            // receive increment between interval
            auto rxdiff = (item.rxBytes() > old_item.rxBytes()) ? (item.rxBytes() - old_item.rxBytes()) : 0;
            // transfer increment between interval
            auto txdiff = (item.txBytes() > old_item.txBytes()) ? (item.txBytes() - old_item.txBytes()) : 0;
            timeval cur_time = timevalList[kCurrentStat];
            timeval prev_time = timevalList[kLastStat];

            auto ltime = prev_time.tv_sec + prev_time.tv_usec * 1. / 1000000;
            auto rtime = cur_time.tv_sec + cur_time.tv_usec * 1. / 1000000;
            auto interval = (rtime > ltime) ? (rtime - ltime) : 1;
            qreal recv_bps = rxdiff / interval;   // Bps
            qreal sent_bps = txdiff / interval;
            item.set_recv_bps(recv_bps);
            item.set_sent_bps(sent_bps);
        }


        m_infoDB.insert(it->addr(), item);
    }
}
void NetifInfoDB::update()
{
    QWriteLocker lock(&m_rwlock);
    this->update_addr();
    this->update_netif_info();

    // d->infoMap =
    // d->addrMap =
}

// 获取进程的上传和下载的流量
bool NetifInfoDB::getSockIOStatByInode(ino_t ino, SockIOStat &stat)
{
    NetifMonitorThread *thread = ThreadManager::instance()->thread<NetifMonitorThread>(BaseThread::kNetifMonitorThread);
    auto netifMonitor = thread->netifJobInstance();
    return netifMonitor->getSockIOStatByInode(ino, stat);

}

} // namespace system
} // namespace core
