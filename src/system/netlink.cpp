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
#include "netlink.h"

#include "nl_addr.h"
#include "nl_link.h"

#include <QDebug>

#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <netlink/cache.h>

namespace core {
namespace system {

Netlink::Netlink()
{
    int rc = 0;
    m_sock = nl_socket_alloc();
    if (!m_sock) {
        qWarning() << "Error: nl_socket_alloc failed";
        return;
    }

    rc = nl_connect(m_sock, NETLINK_ROUTE);
    if (rc) {
        qWarning() << "Error: nl_connect failed";
        return;
    }

    rc = rtnl_link_alloc_cache(m_sock, AF_UNSPEC, &m_linkCache);
    if (rc) {
        qWarning() << "Error: rtnl_link_alloc_cache failed";
        return;
    }

    rc = rtnl_addr_alloc_cache(m_sock, &m_addrCache);
    if (rc) {
        qWarning() << "Error: rtnl_addr_alloc_cache failed";
    }
}

Netlink::~Netlink()
{
    nl_cache_free(m_linkCache);
    nl_cache_free(m_addrCache);
    nl_socket_free(m_sock);
}

LinkIterator Netlink::linkIterator()
{
    LinkIterator it(m_sock, m_linkCache);
    return it;
}

AddrIterator Netlink::addrIterator()
{
    AddrIterator it(m_sock, m_addrCache);
    return it;
}

} // namespace system
} // namespace core
