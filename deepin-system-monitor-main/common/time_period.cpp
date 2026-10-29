// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "time_period.h"
#include "ddlog.h"

namespace common {
namespace core {

using namespace DDLog;

TimePeriod::TimePeriod()
    : m_period(kNoPeriod)
    , m_interval({2, 0})
{
    qCDebug(app) << "TimePeriod default constructor";
}

TimePeriod::TimePeriod(TimePeriod::time_period_t period, const timeval &interval)
    : m_period(period)
    , m_interval(interval)
{
    qCDebug(app) << "TimePeriod overloaded constructor";
}

TimePeriod::TimePeriod(const TimePeriod &rhs)
    : m_period(rhs.m_period)
    , m_interval(rhs.m_interval)
{
    qCDebug(app) << "TimePeriod copy constructor";
}

bool TimePeriod::operator==(const TimePeriod &rhs) const
{
    // qCDebug(app) << "TimePeriod operator==";
    return m_period == rhs.m_period && timercmp(&m_interval, &rhs.m_interval, ==);
}

bool TimePeriod::operator!=(const TimePeriod &rhs) const
{
    // qCDebug(app) << "TimePeriod operator!=";
    return !operator==(rhs);
}

void TimePeriod::registerMetaType()
{
    // qCDebug(app) << "TimePeriod registerMetaType";
    qRegisterMetaType<TimePeriod>("TimePeriod");
}

size_t TimePeriod::ticks() const
{
    qCDebug(app) << "TimePeriod ticks";
    if (m_interval.tv_sec == 0 && m_interval.tv_usec == 0) {
        qCWarning(app) << "interval is 0";
        return 0;
    }

    if (m_period == kNoPeriod) {
        qCDebug(app) << "m_period is kNoPeriod";
        return 2;
    }

    auto v = m_period / (m_interval.tv_sec * 1000000 + m_interval.tv_usec);
    size_t res = v < 1 ? 1 : size_t(v);
    qCDebug(app) << "ticks result:" << res;
    return res;
}

} // namespace core
} // namespace common
