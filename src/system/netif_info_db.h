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
#ifndef NETIF_INFO_DB_H
#define NETIF_INFO_DB_H

#include "netif.h"
#include "netlink.h"

#include <QMultiMap>
#include <QMap>
#include <QReadWriteLock>
#include "netif_monitor.h"
#include <memory>

class QReadWriteLock;

namespace core {
namespace system {

struct netif_stat {
    unsigned long long rx_bytes; // received bytes
    unsigned long long tx_bytes; // transmitted bytes
    char iface[IF_NAMESIZE]; // interface name
};

class NetifInfoDB
{
      enum StatIndex { kLastStat = 0, kCurrentStat = 1, kStatCount = kCurrentStat + 1 };

public:
    explicit NetifInfoDB();
    virtual ~NetifInfoDB() = default;

    QList<INetAddr> addrList(const QByteArray &ifname);
    QMultiMap<QByteArray, INetAddr> addrmap();
    QMap<QByteArray, NetifInfo> infoDB();
    void update();
    bool getSockIOStatByInode(ino_t ino, SockIOStat &stat);

protected:
    void update_addr();
    void update_netif_info();

private:
    std::unique_ptr<Netlink> m_netlink;

    mutable QReadWriteLock m_rwlock;
    QMultiMap<QByteArray, INetAddr> m_addrDB;
    QMap<QByteArray, NetifInfo> m_infoDB;
    // socket inode to io stat mapping
    QMap<ino_t, SockIOStat> m_sockIOStatMap     {};


    timeval timevalList[kStatCount] = {timeval{0, 0}, timeval{0, 0}};
    QSharedPointer<struct netif_stat> m_netStat[kStatCount] {{}, {}};
};

inline QList<INetAddr> NetifInfoDB::addrList(const QByteArray &ifname)
{
    QReadLocker lock(&m_rwlock);
    return m_addrDB.values(ifname);
}

inline QMultiMap<QByteArray, INetAddr> NetifInfoDB::addrmap()
{
    QReadLocker lock(&m_rwlock);
    return m_addrDB;
}

inline QMap<QByteArray, NetifInfo> NetifInfoDB::infoDB()
{
    QReadLocker lock(&m_rwlock);
    return m_infoDB;
}

} // namespace system
} // namespace core

#endif // NETIF_INFO_DB_H
