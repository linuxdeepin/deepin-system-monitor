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
#ifndef NETIF_INFO_CACHE_H
#define NETIF_INFO_CACHE_H

#include <QThread>

#include <memory>
#include <unordered_map>

class NetifInfoCacheUpdater;
class QDBusInterface;
struct netif_info_t;
class NetifInfoCache : public QObject
{
    Q_OBJECT
public:
    explicit NetifInfoCache(QObject *parent = nullptr);

signals:
    void linkStateChanged();

private slots:
    void updateDeviceState(const QString &device, bool enabled);

private:
    void backupLinkState();

private:
    QThread m_thread;
    NetifInfoCacheUpdater *m_cacheUpdater;

    using NetifInfo = std::shared_ptr<struct netif_info_t>;
    using NetifInfoMap = std::unordered_map<int, NetifInfo>;
    NetifInfoMap m_cache;

    // TODO: address cache

    struct link_state_t {
        int ifindex;
        unsigned int carrier_changes;
        unsigned int flags;
    };

    using LinkState = std::unique_ptr<struct link_state_t>;
    using LinkStateMap = std::unordered_map<int, LinkState>;
    LinkStateMap m_linkStateMap;

    QDBusInterface *m_busIf;
};

#endif // NETIF_INFO_CACHE_H
