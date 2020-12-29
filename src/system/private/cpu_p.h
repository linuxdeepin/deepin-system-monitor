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
#ifndef CPU_P_H
#define CPU_P_H

#include <QSharedData>

#include <memory>

namespace core {
namespace system {

struct cpu_cache_t {
    QByteArray name;
    QByteArray type;
    qulonglong size;
    int level;
    int shared_cpu_map;
};
using CPUCache = std::unique_ptr<struct cpu_cache_t>;
using CPUCacheList = std::list<CPUCache>;

class CPUInfo;

class CPUInfoPrivate : public QSharedData
{
public:
    CPUInfoPrivate()
        : QSharedData()
        , index {0}
        , curfreq {0}
        , minfreq {0}
        , maxfreq {0}
        , package_id {0}
        , core_id {0}
        , book_id {0}
        , drawer_id {0}
        , cpu {}
        , cache {}
    {
    }

    CPUInfoPrivate(const CPUInfoPrivate &other)
        : QSharedData(other)
        , index {other.index}
        , curfreq {other.curfreq}
        , minfreq {other.minfreq}
        , maxfreq {other.maxfreq}
        , package_id {other.package_id}
        , core_id {other.core_id}
        , book_id {other.book_id}
        , drawer_id {other.drawer_id}
        , cpu {other.cpu}
        , cache {}
    {
        for (auto &citem : other.cache) {
            CPUCache cp(new cpu_cache_t());
            *cp = *citem;
            cache.push_back(std::move(cp));
        }
    }
    ~CPUInfoPrivate() {}

private:
    int index; // logical index
    quint32 curfreq; // current frequency
    quint32 minfreq; // minimum frequency
    quint32 maxfreq; // maximum frequency
    int package_id; // cpu package index
    int core_id; // cpu core index
    int book_id; // cpu book index
    int drawer_id; // cpu drawer index

    QByteArray cpu;
    CPUCacheList cache; // cpu cache list

    friend class CPUInfo;
};

} // namespace system
} // namespace core

#endif // CPU_P_H
