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
#include "netif_info_cache.h"

// Qt
#include <QTimer>
#include <QMutex>
#include <QDebug>

// netlink
#include <netlink/socket.h>
#include <netlink/addr.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>

// stdc++
#include <memory>
#include <map>

#include "netif.h"

struct NLSocketFree {
    void operator()(struct nl_sock *sock)
    {
        nl_socket_free(sock); // socket closed automatically when free
    }
};
using NLSock = std::unique_ptr<struct nl_sock, NLSocketFree>;

struct NLCacheFree {
    void operator()(struct nl_cache *cache)
    {
        nl_cache_free(cache);
    }
};
using NLCache = std::unique_ptr<struct nl_cache, NLCacheFree>;

void nl_addr_iter_callback(struct nl_object *obj, void *cookie)
{
    auto *addr = reinterpret_cast<struct rtnl_addr *>(obj);
    auto *addrMap = reinterpret_cast<NetifInfoMap *>(cookie);
    Q_ASSERT(addr != nullptr);
    Q_ASSERT(addrMap != nullptr);

    // TODO
}

void nl_link_iter_callback(struct nl_object *obj, void *cookie)
{
    auto *link = reinterpret_cast<struct rtnl_link *>(obj);
    auto *infoMap = reinterpret_cast<NetifInfoMap *>(cookie);
    Q_ASSERT(link != nullptr);
    Q_ASSERT(infoMap != nullptr);

    NetifInfo info(new netif_info_t());

    auto ifindex = rtnl_link_get_ifindex(link);
    info->ifindex = ifindex;

    auto name = rtnl_link_get_name(link);
    strncpy(info->name, name, IFNAMSIZ);

    //    auto carrier = rtnl_link_get_carrier(link);

    uint32_t carrier_changes = 0;
    rtnl_link_get_carrier_changes(link, &carrier_changes);
    info->carrier_changes = carrier_changes;

    uint arp_type = rtnl_link_get_arptype(link);
    info->type = arp_type;

    char hwaddr[32];
    struct nl_addr *addr = rtnl_link_get_addr(link);
    nl_addr2str(addr, hwaddr, 32);
    strncpy(info->hwaddr, hwaddr, 32);

    // TODO: other fields
    infoMap->emplace(std::make_pair(info->ifindex, info));
}

static NetifInfoMap buildNetifInfoCache(struct nl_sock *sock)
{
    NetifInfoMap cache;

    // build link cache
    struct nl_cache *link_cache;
    if (rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache) < 0) {
        qCritical("netlink link cache build failed");
        return {};
    }
    NLCache p_link_cache;
    p_link_cache.reset(link_cache);
    nl_cache_foreach(link_cache, nl_link_iter_callback, &cache);

    // build address cache
    struct nl_cache *addr_cache;
    if (rtnl_addr_alloc_cache(sock, &addr_cache) < 0) {
        qCritical("netlink address cache build failed");
        return {};
    }
    NLCache p_addr_cache;
    p_addr_cache.reset(addr_cache);
    nl_cache_foreach(addr_cache, nl_addr_iter_callback, &cache);

    // TBD:

    return cache;
}

class NetifInfoCacheUpdater : public QObject
{
    Q_OBJECT
public:
    explicit NetifInfoCacheUpdater(QObject *parent = nullptr)
        : QObject(parent)
        , m_timer(new QTimer(this))
        , m_sock(nl_socket_alloc())
        , m_interval(2000)
    {
        connect(m_timer, &QTimer::timeout, this, &NetifInfoCacheUpdater::updateCache);

        nl_connect(m_sock.get(), NETLINK_ROUTE);
    }

    void setUpdateInterval(int interval)
    {
        m_interval = interval;
        m_timer->setInterval(m_interval);
    }

public slots:
    void cron()
    {
        m_timer->setInterval(m_interval);
        m_timer->start();
    }

private slots:
    void updateCache()
    {
        // netlink socket
        auto *sock = m_sock.get();
        Q_ASSERT(sock != nullptr);

        //
        auto cache = buildNetifInfoCache(sock);

        m_lock.lock();
        m_cache = cache;
        m_lock.unlock();
        emit cacheUpdated();
    }

signals:
    void cacheUpdated();

private:
    friend class NetifInfoCache;

    QTimer *m_timer;
    NLSock m_sock;

    QMutex m_lock;
    NetifInfoMap m_cache;

    int m_interval;
    int __unused__; // padding
};

NetifInfoCache::NetifInfoCache(QObject *parent)
    : QObject(parent)
    , m_cacheUpdater(new NetifInfoCacheUpdater())
{
    m_cacheUpdater->moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, m_cacheUpdater, &NetifInfoCacheUpdater::cron);
    connect(&m_thread, &QThread::finished, m_cacheUpdater, &QObject::deleteLater);
    connect(m_cacheUpdater, &NetifInfoCacheUpdater::cacheUpdated, this, [&]() {
        backupLinkState();
        m_cacheUpdater->m_lock.lock();
        m_cache = m_cacheUpdater->m_cache;
        m_cacheUpdater->m_lock.unlock();

        // temperory solution to fix pcap api malfunction when link state changes

        // compare
        if (m_linkStateMap.size() > 0) {
            int nchanges_prev = 0, nchanges_cur = 0;
            for (auto &iter : m_cache) {
                nchanges_cur += iter.second->carrier_changes;

                auto filter = m_linkStateMap.find(iter.first);
                if (filter != m_linkStateMap.end()) {
                    nchanges_prev += filter->second->carrier_changes;
                    // TODO: and flags
                } else {
                    // link added maybe...
                    emit linkStateChanged();
                    break;
                }
            }
            if (nchanges_cur != nchanges_prev) {
                emit linkStateChanged();
            }
            for (auto &iter : m_linkStateMap) {
                auto filter = m_cache.find(iter.first);
                if (filter == m_cache.end()) {
                    // link removed maybe...
                    emit linkStateChanged();
                    break;
                }
            }
        }
    });
    m_thread.start();
}

void NetifInfoCache::backupLinkState()
{
    if (m_cache.size() > 0) {
        m_linkStateMap.clear();

        for (auto &iter : m_cache) {
            LinkState state(new struct link_state_t());
            state->ifindex = iter.second->ifindex;
            state->carrier_changes = iter.second->carrier_changes;
            state->flags = iter.second->flags;
            m_linkStateMap[state->ifindex] = std::move(state);
        }
    }
}

#include "netif_info_cache.moc"
