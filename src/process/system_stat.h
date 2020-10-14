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
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SYSTEM_STAT_H
#define SYSTEM_STAT_H

#include <QSharedPointer>
#include <QMultiMap>

#include <sys/types.h>
#include <netinet/in.h>

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

// from /proc/stat, ref: sysstat#rd_stats.h#stats_cpu
struct cpu_stat {
    unsigned long long user; // user time
    unsigned long long nice; // user time with low priority
    unsigned long long sys; // system time
    unsigned long long idle; // idle time
    unsigned long long iowait; // io wait time
    unsigned long long hardirq; // interrupt time
    unsigned long long softirq; // soft interrupt time
    unsigned long long steal; // stolen time
    unsigned long long guest; // guest time
    unsigned long long guest_nice; // guest time (niced)
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
    unsigned long long rx_packets; // received packets
    unsigned long long tx_packets; // transmitted packets
    unsigned long long rx_bytes; // received bytes
    unsigned long long tx_bytes; // transmitted bytes
    unsigned long long rx_compressed; // number of compressed packets received
    unsigned long long tx_compressed; // number of compressed packets transmitted
    unsigned long long multicast; // number of multicast frames transmitted or received
    char iface[IF_NAMESIZE]; // interface name
};

// from /proc/net/tcp & /proc/net/udp & /proc/net/tcp6 & /proc/net/udp6
struct sock_stat_t {
    ino_t   ino;              // socket inode
    int     sa_family;        // AF_INET & AF_INET6
    int     proto;            // IPPROTO_TCP & IPPROTO_UDP
    union {
        in_addr in4;
        in6_addr in6;
    }       s_addr;           // source address
    uint    s_port;           // source port
    union {
        in_addr in4;
        in6_addr in6;
    }       d_addr;           // remote address
    uint    d_port;           // remote port
    uid_t   uid;              // socket uid
};

struct net_ifaddr_t {
    char iface[IF_NAMESIZE]; // interface name
    int family; // address family
    // flags
    union {
        in_addr in4;
        in6_addr in6;
    } addr; // address
    // netmask
    // broadcast/p2p
};

using CPUStat       = QSharedPointer<struct cpu_stat>;
using CPUStatMap    = QMap<int, QSharedPointer<struct cpu_stat>>;
using MemStat       = QSharedPointer<struct mem_stat>;
using DiskIOStat    = QSharedPointer<struct disk_io_stat>;
using DiskIOStatMap = QMap<QString, QSharedPointer<struct disk_io_stat>>;
using NetIFStat     = QSharedPointer<struct netif_stat>;
using NetIFStatMap  = QMap<QString, QSharedPointer<struct netif_stat>>;
using SockStat      = QSharedPointer<struct sock_stat_t>;
using SockStatMap   = QMultiMap<uint64_t, SockStat>; // [hash, SockStat]
using NetIFAddr     = QSharedPointer<struct net_ifaddr_t>;
using NetIFAddrsMap = QMultiMap<QString, NetIFAddr>;

class SystemStat
{
public:
    /**
     * @brief Read system uptime since startup
     * @param uptime Uptime in seconds
     * @return true: success; false: failure
     */
    static bool readUpTime(qulonglong &uptime);
    /**
     * @brief Read bootup time
     * @param btime Bootup time
     * @return true: success; false: failure
     */
    static bool readBootTime(time_t &btime);
    /**
     * @brief Read /proc/stat
     * @param statSum Total cpu usage stat
     * @param statSMPMap Logical cpu usage stat
     * @return true: success; false: failure
     */
    static bool readCPUStats(CPUStat &statSum, CPUStatMap &statSMPMap);
    /**
     * @brief Read /proc/meminfo
     * @param stat Memory stat
     * @return true: success; false: failure
     */
    static bool readMemStats(MemStat &stat);
    /**
     * @brief Read /proc/diskstats
     * @param statSum Total block device usage stats
     * @param statIOMap Usage stats of each block device
     * @return true: success; false: failure
     */
    static bool readDiskIOStats(DiskIOStat &statSum, DiskIOStatMap &statIOMap);
    /**
     * @brief Read /proc/net/dev
     * @param statSum Total network interface bandwidth usage stats
     * @param statNetIfMap Bandwidth usage stats of each interface
     * @return true: success; false: failure
     */
    static bool readNetIfStats(NetIFStat &statSum, NetIFStatMap &statNetIfMap);
    /**
     * @brief Read /proc/net/tcp, /proc/net/udp, /proc/net/tcp6, /proc/net/udp6
     * @param statMap TCP & UDP stat table
     * @return true: success; false: failure
     */
    static bool readSockStat(SockStatMap &statMap);
    /**
     * @brief Get all interfaces which have addresses set
     * @param addrsMap Network interface address map
     * @return true: success; false: failure
     */
    static bool readNetIfAddrs(NetIFAddrsMap &addrsMap);

    /**
     * @brief Get current real user name
     * @return Current real user name
     */
    static QString getCurrentRealUserName();
    /**
     * @brief Get current real group name
     * @return Current real group name
     */
    static QString getCurrentRealGroupName();

    /**
     * @brief Get current effective user name
     * @return Current effective user name
     */
    static QString getCurrentEffectiveUserName();
    /**
     * @brief Get current effective group name
     * @return Current effective group name
     */
    static QString getCurrentEffectiveGroupName();

    /**
     * @brief Get user name by uid
     * @param uid User Id
     * @return User name
     */
    static QString getUserName(uid_t uid);
    /**
     * @brief Get group name by gid
     * @param gid Group Id
     * @return Group name
     */
    static QString getGroupName(gid_t gid);
private:
    /**
     * @brief Default constructor
     */
    SystemStat() = default;
};

#endif // SYSTEM_STAT_H
