// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cpu.h"
#include "cpu_set.h"
#include "private/cpu_p.h"
#include "common/common.h"
#include "ddlog.h"

#define PATH_TPL_CPU_CACHE "/sys/devices/system/cpu/%d/cache"
#define PATH_TPL_CPU_CACHE_ID "/sys/devices/system/cpu/%d/cache/index%d/id"
#define PATH_TPL_CPU_CACHE_TYPE "/sys/devices/system/cpu/%d/cache/index%d/type"
#define PATH_TPL_CPU_CACHE_SIZE "/sys/devices/system/cpu/%d/cache/index%d/size"
#define PATH_TPL_CPU_CACHE_LEVEL "/sys/devices/system/cpu/%d/cache/index%d/level"
#define PATH_TPL_CPU_CACHE_SHR_CPU_LIST "/sys/devices/system/cpu/%d/cache/index%d/shared_cpu_list"

#define PATH_TPL_CPU_FREQ_MAX "/sys/devices/system/cpu/%d/cpufreq/cpuinfo_max_freq"
#define PATH_TPL_CPU_FREQ_MIN "/sys/devices/system/cpu/%d/cpufreq/cpuinfo_min_freq"
#define PATH_TPL_CPU_FREQ_CUR "/sys/devices/system/cpu/%d/cpufreq/scaling_cur_freq"

#define PATH_TPL_CPU_TOPO_CORE_ID "/sys/devices/system/cpu/%d/topology/core_id"
#define PATH_TPL_CPU_TOPO_CORE_SIBLINGS "/sys/devices/system/cpu/%d/topology/core_siblings_list"
#define PATH_TPL_CPU_TOPO_PHY_PKG_ID "/sys/devices/system/cpu/%d/topology/physical_package_id"
#define PATH_TPL_CPU_TOPO_THR_SIBLINGS "/sys/devices/system/cpu/%d/topology/thread_siblings_list"

using namespace DDLog;

namespace core {
namespace system {

CPUInfo::CPUInfo()
    : d(new CPUInfoPrivate())
{
    qCDebug(app) << "CPUInfo default constructor";
}
CPUInfo::CPUInfo(int index)
    : d(new CPUInfoPrivate())
{
    qCDebug(app) << "CPUInfo constructor for index" << index;
    d->index = index;
}
CPUInfo::CPUInfo(const CPUInfo &other)
    : d(other.d)
{
    qCDebug(app) << "CPUInfo copy constructor for index" << other.d->index;
}
CPUInfo &CPUInfo::operator=(const CPUInfo &rhs)
{
    qCDebug(app) << "CPUInfo assignment operator for index" << rhs.d->index;
    if (this == &rhs)
        return *this;

    d = rhs.d;
    return *this;
}
CPUInfo::~CPUInfo()
{
    // qCDebug(app) << "CPUInfo destructor for index" << d->index;
}

int CPUInfo::logicalIndex() const
{
    qCDebug(app) << "Getting CPU logical index:" << d->index;
    return d->index;
}

QByteArray CPUInfo::logicalName() const
{
    qCDebug(app) << "Getting CPU logical name:" << d->cpu;
    return d->cpu;
}

QString CPUInfo::coreID() const
{
    qCDebug(app) << "Getting CPU core ID:" << d->m_core_id;
    return QString::number(d->m_core_id);
}

QString CPUInfo::modelName() const
{
    qCDebug(app) << "Getting CPU model name:" << d->m_model_name;
    return d->m_model_name;
}

QString CPUInfo::vendorId() const
{
    qCDebug(app) << "Getting CPU vendor ID:" << d->m_vendor_id;
    return d->m_vendor_id;
}

QString CPUInfo::cpuFreq() const
{
    qCDebug(app) << "Getting CPU frequency:" << d->m_cpu_freq;
    if( d->m_cpu_freq == "-") {
        qCDebug(app) << "CPU frequency is not available";
        return "-";
    }
    return d->m_cpu_freq + "MHz";
}

QString CPUInfo::cacheSize() const
{
    qCDebug(app) << "Getting CPU cache size:" << d->m_cache_size;
    return d->m_cache_size;
}

QString CPUInfo::l1iCache() const
{
    qCDebug(app) << "Getting L1 instruction cache";
    for (auto &ce : d->cache) {
        if (ce->level == 1 && ce->type == "Instruction") {
            qCDebug(app) << "Found L1i cache:" << ce->size;
            return common::format::formatUnit_memory_disk({ce->size}, common::format::B, 0);
        }
    }

    qCDebug(app) << "L1i cache not found";
    return "";
}

QString CPUInfo::l1dCache() const
{
    qCDebug(app) << "Getting L1 data cache";
    for (auto &ce : d->cache) {
        if (ce->level == 1 && ce->type == "Data") {
            qCDebug(app) << "Found L1d cache:" << ce->size;
            return common::format::formatUnit_memory_disk({ce->size}, common::format::B, 0);
        }
    }

    qCDebug(app) << "L1d cache not found";
    return "";
}

QString CPUInfo::l2Cache() const
{
    qCDebug(app) << "Getting L2 cache";
    for (auto &ce : d->cache) {
        if (ce->level == 2) {
            qCDebug(app) << "Found L2 cache:" << ce->size;
            return common::format::formatUnit_memory_disk({ce->size}, common::format::B, 0);
        }
    }

    qCDebug(app) << "L2 cache not found";
    return "";
}

QString CPUInfo::l3Cache() const
{
    qCDebug(app) << "Getting L3 cache";
    for (auto &ce : d->cache) {
        if (ce->level == 3) {
            qCDebug(app) << "Found L3 cache:" << ce->size;
            return common::format::formatUnit_memory_disk({ce->size}, common::format::B, 0);
        }
    }

    qCDebug(app) << "L3 cache not found";
    return "";
}

void CPUInfo::setIndex(int index)
{
    qCDebug(app) << "Setting CPU index to" << index;
    d->index = index;
}

void CPUInfo::setCoreId(int coreId)
{
    qCDebug(app) << "Setting CPU core ID to" << coreId;
    d->m_core_id = coreId;
}

void CPUInfo::setModelName(QString modelName)
{
    qCDebug(app) << "Setting CPU model name to" << modelName;
    d->m_model_name = modelName;
}

void CPUInfo::setVendorId(QString vendorId)
{
    qCDebug(app) << "Setting CPU vendor ID to" << vendorId;
    d->m_vendor_id = vendorId;
}

void CPUInfo::setCpuFreq(QString cpuFreq)
{
    qCDebug(app) << "Setting CPU frequency to" << cpuFreq;
    d->m_cpu_freq = cpuFreq;
}

void CPUInfo::setCacheSize(QString cacheSize)
{
    qCDebug(app) << "Setting CPU cache size to" << cacheSize;
    d->m_cache_size = cacheSize;
}

} // namespace system
} // namespace core
