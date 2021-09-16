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
#include "time_period.h"

namespace common {
namespace core {

TimePeriod::TimePeriod()
    : m_period(kNoPeriod)
    , m_interval({2, 0})
{
}

TimePeriod::TimePeriod(TimePeriod::time_period_t period, const timeval &interval)
    : m_period(period)
    , m_interval(interval)
{
}

TimePeriod::TimePeriod(const TimePeriod &rhs)
    : m_period(rhs.m_period)
    , m_interval(rhs.m_interval)
{
}

bool TimePeriod::operator==(const TimePeriod &rhs) const
{
    return m_period == rhs.m_period && timercmp(&m_interval, &rhs.m_interval, ==);
}

bool TimePeriod::operator!=(const TimePeriod &rhs) const
{
    return !operator==(rhs);
}

void TimePeriod::registerMetaType()
{
    qRegisterMetaType<TimePeriod>("TimePeriod");
}

size_t TimePeriod::ticks() const
{
    if (m_interval.tv_sec == 0 && m_interval.tv_usec == 0)
        return 0;

    if (m_period == kNoPeriod) {
        return 2;
    }

    auto v = m_period / (m_interval.tv_sec * 1000000 + m_interval.tv_usec);
    return v < 1 ? 1 : size_t(v);
}

} // namespace core
} // namespace common
