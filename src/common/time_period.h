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
#ifndef TIME_PERIOD_H
#define TIME_PERIOD_H

#include <QtGlobal>
#include <QMetaType>

#include <sys/time.h>

namespace common {
namespace core {

class TimePeriod
{
public:
    enum time_period_t {
        kNoPeriod = 0,
        k1Min = 60 * 1000000,
        k5Min = k1Min * 5,
        k15Min = k1Min * 15
    };

    explicit TimePeriod();
    explicit TimePeriod(TimePeriod::time_period_t period, const struct timeval &interval);
    TimePeriod(const TimePeriod &rhs);
    TimePeriod &operator=(const TimePeriod &rhs) = default;
    ~TimePeriod() = default;

    static void registerMetaType();

    TimePeriod::time_period_t period() const;
    void setPeriod(TimePeriod::time_period_t period);
    const struct timeval &interval() const;
    void setInterval(const struct timeval &interval);
    size_t ticks() const;

    bool operator==(const TimePeriod &rhs) const;
    bool operator!=(const TimePeriod &rhs) const;

private:
    TimePeriod::time_period_t m_period;
    struct timeval m_interval;
};

inline TimePeriod::time_period_t TimePeriod::period() const
{
    return m_period;
}

inline void TimePeriod::setPeriod(TimePeriod::time_period_t period)
{
    m_period = period;
}

inline const timeval &TimePeriod::interval() const
{
    return m_interval;
}

inline void TimePeriod::setInterval(const timeval &interval)
{
    m_interval = interval;
}

} // namespace core
} // namespace common

Q_DECLARE_METATYPE(common::core::TimePeriod)

#endif // TIME_PERIOD_H
