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

class NetifInfoDB
{
public:
    explicit NetifInfoDB();
    virtual ~NetifInfoDB() = default;

    QList<INetAddr> addrList(const QByteArray &ifname);
    QMultiMap<QByteArray, INetAddr> addrmap();
    QList<NetifInfo> infoDB();
    void update();
    inline bool getSockIOStatByInode(ino_t ino, SockIOStat &stat);

private:
    std::unique_ptr<Netlink> m_netlink;

    mutable QReadWriteLock m_rwlock;
    QMultiMap<QByteArray, INetAddr> m_addrDB;
    QList<NetifInfo> m_infoDB;
    // socket inode to io stat mapping
    QMap<ino_t, SockIOStat> m_sockIOStatMap     {};
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

inline QList<NetifInfo> NetifInfoDB::infoDB()
{
    QReadLocker lock(&m_rwlock);
    return m_infoDB;
}

} // namespace system
} // namespace core

#endif // NETIF_INFO_DB_H
