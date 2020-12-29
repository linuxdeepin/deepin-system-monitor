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

#include <memory>

namespace core {
namespace system {

NetifInfoDB::NetifInfoDB()
    : m_netlink(new Netlink())
    , m_addrDB {}
    , m_infoDB {}
{
}

void NetifInfoDB::update()
{
    // iterator

    QWriteLocker lock(&m_rwlock);
    // d->infoMap =
    // d->addrMap =
}

} // namespace system
} // namespace core
