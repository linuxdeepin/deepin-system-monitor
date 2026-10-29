// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netlink.h"
#include "ddlog.h"
#include "nl_addr.h"
#include "nl_link.h"

#include <QDebug>

#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <netlink/cache.h>
using namespace DDLog;
namespace core {
namespace system {

Netlink::Netlink()
{
    qCDebug(app) << "Creating Netlink object";
    int rc = 0;
    m_sock = nl_socket_alloc();
    if (!m_sock) {
        qCWarning(app) << "Failed to allocate netlink socket";
        return;
    }
    qCDebug(app) << "Netlink socket allocated";

    rc = nl_connect(m_sock, NETLINK_ROUTE);
    if (rc) {
        qCWarning(app) << "Failed to connect netlink socket:" << nl_geterror(rc);
        nl_socket_free(m_sock);
        m_sock = nullptr;
        return;
    }
    qCDebug(app) << "Netlink socket connected";

    rc = rtnl_link_alloc_cache(m_sock, AF_UNSPEC, &m_linkCache);
    if (rc) {
        qCWarning(app) << "Failed to allocate link cache:" << nl_geterror(rc);
        nl_socket_free(m_sock);
        m_sock = nullptr;
        return;
    }
    qCDebug(app) << "Netlink link cache allocated";

    rc = rtnl_addr_alloc_cache(m_sock, &m_addrCache);
    if (rc) {
        qCWarning(app) << "Failed to allocate address cache:" << nl_geterror(rc);
        nl_cache_free(m_linkCache);
        m_linkCache = nullptr;
        nl_socket_free(m_sock);
        m_sock = nullptr;
    }
    qCDebug(app) << "Netlink address cache allocated";
}

Netlink::~Netlink()
{
    qCDebug(app) << "Destroying Netlink object";
    if (m_linkCache)
        nl_cache_free(m_linkCache);
    if (m_addrCache)
        nl_cache_free(m_addrCache);
    if (m_sock)
        nl_socket_free(m_sock);
    qCDebug(app) << "Netlink resources freed";
}

LinkIterator Netlink::linkIterator()
{
    qCDebug(app) << "Creating link iterator";
    LinkIterator it(m_sock, m_linkCache);
    return it;
}

AddrIterator Netlink::addrIterator()
{
    qCDebug(app) << "Creating address iterator";
    AddrIterator it(m_sock, m_addrCache);
    return it;
}

}   // namespace system
}   // namespace core
