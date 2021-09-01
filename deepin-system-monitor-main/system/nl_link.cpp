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
#include "nl_link.h"

namespace core {
namespace system {

NLLink::NLLink(struct rtnl_link *link, bool hold)
    : m_link(link)
    , m_hold(hold)
{
    Q_ASSERT(m_link != nullptr);
}

NLLink::~NLLink()
{
    if (m_hold && m_link)
        rtnl_link_put(m_link);
}

} // namespace system
} // namespace core
