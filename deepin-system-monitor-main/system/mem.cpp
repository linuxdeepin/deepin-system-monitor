// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mem.h"
#include "private/mem_p.h"
#include "common/common.h"
#include "ddlog.h"

#include <stdio.h>

#define PROC_PATH_MEM "/proc/meminfo"

using namespace common::error;
using namespace common::alloc;
using namespace DDLog;

namespace core {
namespace system {

MemInfo::MemInfo()
    : d(new MemInfoPrivate())
{
    qCDebug(app) << "MemInfo constructor";
}

MemInfo::MemInfo(const MemInfo &other)
    : d(other.d)
{
    qCDebug(app) << "MemInfo copy constructor";
}

MemInfo &MemInfo::operator=(const MemInfo &rhs)
{
    qCDebug(app) << "MemInfo assignment operator";
    if (this == &rhs)
        return *this;

    d = rhs.d;
    return *this;
}

MemInfo::~MemInfo()
{
    // qCDebug(app) << "MemInfo destructor";
}

qulonglong MemInfo::memTotal() const
{
    return d->mem_total_kb;
}

qulonglong MemInfo::memAvailable() const
{
    return d->mem_avail_kb;
}

qulonglong MemInfo::buffers() const
{
    return d->buffers_kb;
}

qulonglong MemInfo::cached() const
{
    return d->cached_kb;
}

qulonglong MemInfo::active() const
{
    return d->active_kb;
}

qulonglong MemInfo::inactive() const
{
    return d->inactive_kb;
}

qulonglong MemInfo::swapTotal() const
{
    return d->swap_total_kb;
}

qulonglong MemInfo::swapFree() const
{
    return d->swap_free_kb;
}

qulonglong MemInfo::swapCached() const
{
    return d->swap_cached_kb;
}

qulonglong MemInfo::shmem() const
{
    return d->shmem_kb;
}

qulonglong MemInfo::slab() const
{
    return d->slab_kb;
}

qulonglong MemInfo::dirty() const
{
    return d->dirty_kb;
}

qulonglong MemInfo::mapped() const
{
    return d->mapped_kb;
}

void MemInfo::readMemInfo()
{
    qCDebug(app) << "Reading memory info from" << PROC_PATH_MEM;
    FILE *fp;
    uFile ufp;
    const size_t BUFLEN = 512;
    QByteArray line(BUFLEN, '\0');

    if ((fp = fopen(PROC_PATH_MEM, "r"))) {
        ufp.reset(fp);

        int nr = 0;
        while (fgets(line.data(), BUFLEN, fp)) {
            if (!strncmp(line.data(), "MemTotal:", 9)) {
                nr = sscanf(line.data() + 9, "%llu", &d->mem_total_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse MemTotal from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "MemFree:", 8)) {
                nr = sscanf(line.data() + 8, "%llu", &d->mem_free_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse MemFree from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "MemAvailable:", 13)) {
                nr = sscanf(line.data() + 13, "%llu", &d->mem_avail_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse MemAvailable from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "Buffers:", 8)) {
                nr = sscanf(line.data() + 8, "%llu", &d->buffers_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse Buffers from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "Cached:", 7)) {
                nr = sscanf(line.data() + 7, "%llu", &d->cached_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse Cached from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "SwapCached:", 11)) {
                nr = sscanf(line.data() + 11, "%llu", &d->swap_cached_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse SwapCached from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "Active:", 7)) {
                nr = sscanf(line.data() + 7, "%llu", &d->active_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse Active from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "Inactive:", 9)) {
                nr = sscanf(line.data() + 9, "%llu", &d->inactive_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse Inactive from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "SwapTotal:", 10)) {
                nr = sscanf(line.data() + 10, "%llu", &d->swap_total_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse SwapTotal from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "SwapFree:", 9)) {
                nr = sscanf(line.data() + 9, "%llu", &d->swap_free_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse SwapFree from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "Dirty:", 6)) {
                nr = sscanf(line.data() + 6, "%llu", &d->dirty_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse Dirty from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "Shmem:", 6)) {
                nr = sscanf(line.data() + 6, "%llu", &d->shmem_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse Shmem from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "Slab:", 5)) {
                nr = sscanf(line.data() + 5, "%llu", &d->slab_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse Slab from" << PROC_PATH_MEM;

            } else if (!strncmp(line.data(), "Mapped:", 7)) {
                nr = sscanf(line.data() + 7, "%llu", &d->mapped_kb);

                if (nr != 1)
                    qCWarning(app) << "Failed to parse Mapped from" << PROC_PATH_MEM;
            }
        } // ::while(fgets)

        if (ferror(fp)) {
            qCWarning(app) << "Error reading" << PROC_PATH_MEM << ":" << strerror(errno);
        }
        qCDebug(app) << "Finished reading memory info.";
        return;
    } // ::if(fopen)

    qCWarning(app) << "Failed to open" << PROC_PATH_MEM << ":" << strerror(errno);
}

} // namespace system
} // namespace core
