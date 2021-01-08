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
#ifndef SAMPLE_H
#define SAMPLE_H

#include "time_period.h"
#include "common/common.h"

#include <memory>
#include <list>

#include <sys/time.h>

using namespace common::core;
using namespace common::time;

struct IO {
    qulonglong inBytes;
    qulonglong outBytes;

    struct IO &operator-=(const struct IO &rhs)
    {
        inBytes -= rhs.inBytes;
        outBytes -= rhs.outBytes;
        return *this;
    }
    struct IO &operator+=(const struct IO &rhs)
    {
        inBytes += rhs.inBytes;
        outBytes += rhs.outBytes;
        return *this;
    }
    const struct IO operator-(const struct IO &other) const {
        return IO(*this) -= other;
    }
    const struct IO operator+(const struct IO &other) const {
        return IO(*this) += other;
    }
};

struct IOPS {
    qreal inBps;
    qreal outBps;
};

template<typename T>
class SampleFrame
{
public:
    SampleFrame()
        : data()
    {
    }
    SampleFrame(const T &d)
        : data(d)
    {
    }
    SampleFrame(const SampleFrame<T> &other)
        : data(other.data)
    {
    }

    T data;
};

template<>
class SampleFrame<IO>
{
public:
    SampleFrame()
        : ts()
        , data()
    {
    }
    SampleFrame(const struct timeval &tv, const struct IO &d)
        : ts(tv)
        , data(d)
    {
    }
    SampleFrame(const SampleFrame<IO> &other)
        : ts(other.ts)
        , data(other.data)
    {
    }

    SampleFrame<IO> &operator+=(const SampleFrame<IO> &rhs)
    {
        ts += rhs.ts;
        data += rhs.data;
        return *this;
    }
    SampleFrame<IO> &operator-=(const SampleFrame<IO> &rhs)
    {
        ts -= rhs.ts;
        data -= rhs.data;
        return *this;
    }
    const SampleFrame<IO> operator+(const SampleFrame<IO> &rhs) const
    {
        return SampleFrame<IO>(*this) += rhs;
    }
    const SampleFrame<IO> operator-(const SampleFrame<IO> &rhs) const
    {
        return SampleFrame<IO>(*this) -= rhs;
    }

    static IOPS iops(const SampleFrame<IO> *lhs, const SampleFrame<IO> *rhs)
    {
        if (!lhs)
            return {.0, .0};

        auto tmdiff = lhs->ts - rhs->ts;
        auto ratio = tmdiff.tv_sec + tmdiff.tv_usec * 1. / 1000000;
        auto inBps = (lhs->data.inBytes - (rhs ? rhs->data.inBytes : 0)) / ratio;
        auto outBps = (lhs->data.outBytes - (rhs ? rhs->data.outBytes : 0)) / ratio;

        return {inBps, outBps};
    }

    struct timeval ts;
    struct IO data;
};

template<>
class SampleFrame<qulonglong>
{
public:
    SampleFrame()
        : data()
    {
    }
    SampleFrame(const struct timeval &tv, qulonglong &d)
        : ts(tv)
        , data(d)
    {
    }
    SampleFrame(const SampleFrame<qulonglong> &other)
        : ts(other.ts)
        , data(other.data)
    {
    }

    SampleFrame<qulonglong> &operator+=(const SampleFrame<qulonglong> &rhs)
    {
        ts += rhs.ts;
        data += rhs.data;
        return *this;
    }
    SampleFrame<qulonglong> &operator-=(const SampleFrame<qulonglong> &rhs)
    {
        ts -= rhs.ts;
        data -= rhs.data;
        return *this;
    }
    const SampleFrame<qulonglong> operator+(const SampleFrame<qulonglong> &rhs) const
    {
        return SampleFrame(*this) += rhs;
    }
    const SampleFrame<qulonglong> operator-(const SampleFrame<qulonglong> &rhs) const
    {
        return SampleFrame(*this) -= rhs;
    }

    struct timeval ts;
    qulonglong data;
};

template<typename T>
class Sample
{
public:
    explicit Sample()
        : m_samples {}
        , m_period {}
        , m_maxSamples {}
    {
        m_maxSamples = m_period.ticks();
    }
    explicit Sample(const TimePeriod &period)
        : m_samples {}
        , m_period {period}
        , m_maxSamples {}
    {
        m_maxSamples = m_period.ticks();
    }
    Sample(const Sample &other)
        : m_period(other.m_period)
        , m_maxSamples(other.m_maxSamples)
    {
        for (auto &item : other.m_samples) {
            m_samples.push_back(std::make_shared<SampleFrame<T>>(*item));
        }
        m_maxSamples = m_period.ticks();
    }

    inline void addSample(SampleFrame<T> *frame)
    {
        if (frame) {
            if (m_samples.size() >= m_maxSamples)
                m_samples.pop_front();

            std::shared_ptr<SampleFrame<T>> framePtr;
            framePtr.reset(frame);
            m_samples.push_back(framePtr);
        }
    }

    inline const TimePeriod &timePeriod() const
    {
        return m_period;
    }

    inline const SampleFrame<T> *recentSample() const
    {
        if (m_samples.size() > 0)
            return m_samples.last().get();
        else
            return nullptr;
    }

    inline void updateTimePeriod(const TimePeriod &newPeriod)
    {
        auto ticks = newPeriod.ticks();
        auto old = m_period.ticks();

        if (timercmp(&newPeriod.interval(), &m_period.interval(), !=)) {
            m_samples.clear();
        } else if (ticks < old) {
            while (m_samples.size() > ticks) {
                m_samples.pop_front();
            }
        }
        m_maxSamples = ticks;
        m_period = newPeriod;
    }

    inline const QPair<const SampleFrame<T> *, const SampleFrame<T> *> recentSamplePair() const
    {
        QPair<const SampleFrame<T> *, const SampleFrame<T> *> pair {};
        if (m_samples.size() > 0)
            pair.first = m_samples[0].get();
        if (m_samples.size() > 1)
            pair.second = m_samples[1].get();
        return pair;
    }

    inline int count() const
    {
        return m_samples.size();
    }

    inline const SampleFrame<T> *sample(int index) const
    {
        if (index >= 0 && index < m_samples.size()) {
            return m_samples[index].get();
        }

        return nullptr;
    }

private:
    // TODO: replace with circular buffer impl. (ref: boost/circular_buffer)
    QList<std::shared_ptr<SampleFrame<T>>> m_samples;
    TimePeriod m_period;
    size_t m_maxSamples;
};

using IOSampleFrame = SampleFrame<IO>;
using IOPSSampleFrame = SampleFrame<IOPS>;
using IOSample = Sample<IO>;
using IOPSSample = Sample<IOPS>;

#endif // SAMPLE_H
