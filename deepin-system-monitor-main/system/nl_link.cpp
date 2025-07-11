// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nl_link.h"
#include "ddlog.h"

using namespace DDLog;
namespace core {
namespace system {

NLLink::NLLink(struct rtnl_link *link, bool hold)
    : m_link(link)
    , m_hold(hold)
{
    qCDebug(app) << "NLLink created, hold=" << hold;
    Q_ASSERT(m_link != nullptr);
}

NLLink::~NLLink()
{
    qCDebug(app) << "NLLink destroyed, hold=" << m_hold;
    if (m_hold && m_link) {
        rtnl_link_put(m_link);
        qCDebug(app) << "rtnl_link_put called";
    }
}

} // namespace system
} // namespace core
