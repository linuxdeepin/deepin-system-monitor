/*
 * Copyright (C) 2019 ~ 2019 Union Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@uniontech.com>
 *
 * Maintainer: zccrs <zhangjide@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SYSTEM_STAT_H
#define SYSTEM_STAT_H

#include <sys/types.h>
#include <net/if.h>
#include <time.h>

#include <QSharedPointer>

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

// from /proc/stat, ref: sysstat#rd_stats.h#stats_cpu
struct cpu_stat {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long sys;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long hardirq;
    unsigned long long softirq;
    unsigned long long steal;
    unsigned long long guest;
    unsigned long long guest_nice;
};

// from /proc/meminfo, ref: sysstat#rd_stats.h#stats_memory
struct mem_stat {
    unsigned long long mem_total_kb;        // MemTotal
    unsigned long long mem_free_kb;         // MemFree
    unsigned long long mem_avail_kb;        // MemAvailable
    unsigned long long buffers_kb;          // Buffers
    unsigned long long cached_kb;           // Cached
    unsigned long long swap_cached_kb;      // SwapCached
    unsigned long long active_kb;           // Active
    unsigned long long inactive_kb;         // Inactive
    unsigned long long slab_kb;             // Slab
    unsigned long long swap_total_kb;       // SwapTotal
    unsigned long long swap_free_kb;        // SwapFree
    unsigned long long dirty_kb;            // Dirty
    unsigned long long anon_pages_kb;       // AnonPages
};

// from /proc/diskstats, ref: sysstat#rd_stats.h#stats_io
struct disk_io_stat {
    unsigned long long total_ios;
    unsigned long long read_ios;            // # of reads completed
    unsigned long long read_sectors;        // # of sectors read
    unsigned long long write_ios;           // # of writes completed
    unsigned long long write_sectors;       // # of sectors written
    unsigned long long discard_ios;         // # of discards completed
    unsigned long long discard_sectors;     // # of sectors discarded
};

// from /proc/net/dev, ref: sysstat#rd_stats.h#stats_net_dev
struct netif_stat {
    unsigned long long rx_packets;
    unsigned long long tx_packets;
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    unsigned long long rx_compressed;
    unsigned long long tx_compressed;
    unsigned long long multicast;
    char iface[IF_NAMESIZE];
};

using CPUStat = QSharedPointer<struct cpu_stat>;
using CPUStatMap = QMap<int, QSharedPointer<struct cpu_stat>>;
using MemStat = QSharedPointer<struct mem_stat>;
using DiskIOStat = QSharedPointer<struct disk_io_stat>;
using DiskIOStatMap = QMap<QString, QSharedPointer<struct disk_io_stat>>;
using NetIFStat = QSharedPointer<struct netif_stat>;
using NetIFStatMap = QMap<QString, QSharedPointer<struct netif_stat>>;

class SystemStat
{
public:
    static bool readUpTime(qulonglong &uptime);
    static bool readBootTime(time_t &btime);
    static bool readCPUStats(CPUStat &statSum, CPUStatMap &statSMPMap);
    static bool readMemStats(MemStat &stat);
    static bool readDiskIOStats(DiskIOStat &statSum, DiskIOStatMap &statIOMap);
    static bool readNetIfStats(NetIFStat &statSum, NetIFStatMap &statNetIfMap);

    static QString getCurrentRealUserName();
    static QString getCurrentRealGroupName();

    static QString getCurrentEffectiveUserName();
    static QString getCurrentEffectiveGroupName();

    static QString getUserName(uid_t uid);
    static QString getGroupName(gid_t gid);
private:
    SystemStat() = default;
};

#endif // SYSTEM_STAT_H
