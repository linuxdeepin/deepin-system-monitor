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

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <pwd.h>
#include <arpa/inet.h>

#include <QMap>
#include <QScopedArrayPointer>
#include <QDebug>
#include <QProcess>

#include "system_stat.h"
#include "common/hash.h"

#define PROC_PATH_UPTIME    "/proc/uptime"
#define PROC_PATH_STAT      "/proc/stat"
#define PROC_PATH_CPU       PROC_PATH_STAT
#define PROC_PATH_MEM       "/proc/meminfo"
#define PROC_PATH_DISK      "/proc/diskstats"
#define PROC_PATH_NET       "/proc/net/dev"
#define SYSFS_PATH_BLOCK    "/sys/block"

#define PROC_PATH_SOCK_TCP  "/proc/net/tcp"
#define PROC_PATH_SOCK_TCP6 "/proc/net/tcp6"
#define PROC_PATH_SOCK_UDP  "/proc/net/udp"
#define PROC_PATH_SOCK_UDP6 "/proc/net/udp6"

#define MAX_NAME_LEN 128

auto print_err = [](decltype(errno) e, const QString &msg)
{
    qDebug() << QString("Error: [%1] %2, ").arg(e).arg(strerror(e)) << msg;
};

bool SystemStat::readUpTime(qulonglong &uptime)
{
    bool b = false;
    FILE *fp;
    char buf[128];
    int rc;
    unsigned long up_nsec, up_ncent;

    errno = 0;
    if ((fp = fopen(PROC_PATH_UPTIME, "r")) == nullptr) {
        print_err(errno, QString("open %1 failed").arg(PROC_PATH_UPTIME));
        return b;
    }

    if (fgets(buf, sizeof(buf), fp)) {
        rc = sscanf(buf, "%lu.%lu", &up_nsec, &up_ncent);
        if (rc == 2) {
            uptime = up_nsec * 100 + up_nsec;
            b = true;
        }
    }
    fclose(fp);
    if (!b) {
        print_err(errno, QString("read %1 failed").arg(PROC_PATH_UPTIME));
    }

    return b;
}

bool SystemStat::readBootTime(time_t &btime)
{
    bool b = false;
    FILE *fp;
    char buf[128];

    btime = 0;

    errno = 0;
    if (!(fp = fopen(PROC_PATH_STAT, "r"))) {
        print_err(errno, QString("open %1 failed").arg(PROC_PATH_STAT));
        return b;
    }

    while (fgets(buf, 128, fp)) {
        if (sscanf(buf, "btime %lu", &btime) == 1) {
            b = true;
            break;
        }
    }
    if (ferror(fp)) {
        print_err(errno, QString("read %1 failed").arg(PROC_PATH_STAT));
    }
    fclose(fp);

    return b;
}

bool SystemStat::readCPUStats(CPUStat &cpuStat, CPUStatMap &cpuStatMap)
{
    bool b = true;
    FILE *fp;
    const size_t bsiz = 8192;
    QScopedArrayPointer<char> line(new char[bsiz] {});
    int ncpu = 0;
    int rc;

    if (!cpuStat) {
        cpuStat = CPUStat(new cpu_stat {});
    }
    cpuStatMap.clear();

    if ((fp = fopen(PROC_PATH_CPU, "r")) == nullptr) {
        print_err(errno, QString("open %1 failed").arg(PROC_PATH_CPU));
        return false;
    }

    while (fgets(line.data(), bsiz, fp)) {
        // ignore errors while processing each line
        if (!strncmp(line.data(), "cpu ", 4)) {
            // all cpu stat in jiffies
            memset(cpuStat.data(), 0, sizeof(struct cpu_stat));
            rc = sscanf(line.data() + 5, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                        &cpuStat->user,
                        &cpuStat->nice,
                        &cpuStat->sys,
                        &cpuStat->idle,
                        &cpuStat->iowait,
                        &cpuStat->hardirq,
                        &cpuStat->softirq,
                        &cpuStat->steal,
                        &cpuStat->guest,
                        &cpuStat->guest_nice);
            b = (rc == 10);
        } else if (!strncmp(line.data(), "cpu", 3)) {
            // per cpu stat in jiffies
            auto stat = CPUStat(new cpu_stat {});
            rc = sscanf(line.data() + 3, "%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                        &ncpu,
                        &stat->user,
                        &stat->nice,
                        &stat->sys,
                        &stat->idle,
                        &stat->iowait,
                        &stat->hardirq,
                        &stat->softirq,
                        &stat->steal,
                        &stat->guest,
                        &stat->guest_nice);
            cpuStatMap[ncpu] = stat;
            b = (rc = 11);
        } else {
            break;
        }
    }

    b = !ferror(fp) && b;
    fclose(fp);
    if (!b) {
        print_err(errno, QString("read %1 failed").arg(PROC_PATH_CPU));
    }

    return b;
}

bool SystemStat::readMemStats(MemStat &memStat)
{
    FILE *fp;
    const size_t bsiz = 512;
    QScopedArrayPointer<char> line(new char[bsiz] {});
    int rc;
    bool b = true;

    if ((fp = fopen(PROC_PATH_MEM, "r")) == nullptr) {
        print_err(errno, QString("open %1 failed").arg(PROC_PATH_MEM));
        return false;
    }

    if (!memStat) {
        memStat = MemStat(new mem_stat {});
    }
    memset(memStat.data(), 0, sizeof(struct mem_stat));

    QProcess process;
    process.start("swapon -s");
    process.waitForFinished(100);
    QString swapInfo = process.readAllStandardOutput();
    QStringList swapInfoList = swapInfo.split("\n", QString::SkipEmptyParts);
    bool isCmdAvailable = false;
    if (swapInfoList.size() >= 2) {
        QStringList list {};
        foreach (QString line, swapInfoList) {
            if (line.contains("swap", Qt::CaseInsensitive)) {
                list = line.split("\t", QString::SkipEmptyParts);
                if (list.size() >= 4) {
                    QString swapTotal = list[2];
                    QString swapUsed = list[3];
                    memStat->swap_total_kb = swapTotal.toULongLong();
                    memStat->swap_free_kb = memStat->swap_total_kb - swapUsed.toULongLong();
                    isCmdAvailable = true;
                }
                break;
            }
        }
    }

    while (fgets(line.data(), bsiz, fp)) {
        if (!strncmp(line.data(), "MemTotal:", 9)) {
            rc = sscanf(line.data() + 9, "%llu", &memStat->mem_total_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "MemFree:", 8)) {
            rc = sscanf(line.data() + 8, "%llu", &memStat->mem_free_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "MemAvailable:", 13)) {
            rc = sscanf(line.data() + 13, "%llu", &memStat->mem_avail_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "Buffers:", 8)) {
            rc = sscanf(line.data() + 8, "%llu", &memStat->buffers_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "Cached:", 7)) {
            rc = sscanf(line.data() + 7, "%llu", &memStat->cached_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "SwapCached:", 11)) {
            rc = sscanf(line.data() + 11, "%llu", &memStat->swap_cached_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "Active:", 7)) {
            rc = sscanf(line.data() + 7, "%llu", &memStat->active_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "Inactive:", 9)) {
            rc = sscanf(line.data() + 9, "%llu", &memStat->inactive_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "SwapTotal:", 10)) {
            if (isCmdAvailable)
                continue;
            rc = sscanf(line.data() + 10, "%llu", &memStat->swap_total_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "SwapFree:", 9)) {
            if (isCmdAvailable)
                continue;
            rc = sscanf(line.data() + 9, "%llu", &memStat->swap_free_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "Dirty:", 6)) {
            rc = sscanf(line.data() + 6, "%llu", &memStat->dirty_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "AnonPages:", 10)) {
            rc = sscanf(line.data() + 10, "%llu", &memStat->anon_pages_kb);
            b = (rc == 1);
        } else if (!strncmp(line.data(), "Slab:", 5)) {
            rc = sscanf(line.data() + 5, "%llu", &memStat->slab_kb);
            b = (rc == 1);
        }
    }

    b = !ferror(fp) && b;
    fclose(fp);
    if (!b) {
        print_err(errno, QString("read %1 failed").arg(PROC_PATH_MEM));
    }

    return b;
}

bool SystemStat::readDiskIOStats(DiskIOStat &statSum, DiskIOStatMap &statIOMap)
{
    bool b = false;
    FILE *fp;
    const size_t bsiz = 2048;
    QScopedArrayPointer<char> line(new char[bsiz] {});
    unsigned int major, minor;
    char dev_name[MAX_NAME_LEN];
    int rc;

    // ignore any partition stats here, ref: sysstat#common.c#is_device
    auto is_block_dev = [ = ](char *dev_name) -> bool {
        char syspath[PATH_MAX];
        char *slash;

        // replace any '/' characters in device names if any
        while ((slash = strchr(dev_name, '/')))
        {
            *slash = '!';
        }

        snprintf(syspath, sizeof(syspath), "%s/%s", SYSFS_PATH_BLOCK, dev_name);
        return (!access(syspath, F_OK));
    };

    if ((fp = fopen(PROC_PATH_DISK, "r")) == nullptr) {
        print_err(errno, QString("open %1 failed").arg(PROC_PATH_DISK));
        return b;
    }

    if (!statSum) {
        statSum = DiskIOStat(new disk_io_stat {});
    }
    memset(statSum.data(), 0, sizeof(struct disk_io_stat));
    statIOMap.clear();

    while (fgets(line.data(), bsiz, fp)) {
        auto stat = DiskIOStat(new disk_io_stat {});
        //************************1**2**3****4********5********6*******7********************8********9**
        rc = sscanf(line.data(), "%u %u %s %llu %*u %llu %*u %llu %*u %llu %*u %*u %*u %*u %llu %*u %llu",
                    &major,
                    &minor,
                    dev_name,
                    &stat->read_ios,
                    &stat->read_sectors,
                    &stat->write_ios,
                    &stat->write_sectors,
                    &stat->discard_ios,
                    &stat->discard_sectors);
        // discard io stats might not be available
        if (rc >= 7) {
            if (is_block_dev(dev_name)) {
                // per block dev stats
                statIOMap[dev_name] = stat;

                // total io
                statSum->total_ios += stat->read_ios + stat->write_ios + stat->discard_ios;
                statSum->read_ios += stat->read_ios;
                statSum->read_sectors += stat->read_sectors;
                statSum->write_ios += stat->write_ios;
                statSum->write_sectors += stat->write_sectors;
                statSum->discard_ios += stat->discard_ios;
                statSum->discard_sectors += stat->discard_sectors;

                b = true;
            }
        }
    }
    b = !ferror(fp) && b;
    fclose(fp);
    if (!b) {
        print_err(errno, QString("read %1 failed").arg(PROC_PATH_DISK));
    }

    return b;
}

bool SystemStat::readNetIfStats(NetIFStat &statSum, NetIFStatMap &statNetIfMap)
{
    bool b = false;
    FILE *fp;
    const size_t bsiz = 256;
    QScopedArrayPointer<char> line(new char[bsiz] {});
    char *pos, *start;
    int rc;

    if ((fp = fopen(PROC_PATH_NET, "r")) == nullptr) {
        print_err(errno, QString("open %1 failed").arg(PROC_PATH_NET));
        return b;
    }

    if (!statSum) {
        statSum = NetIFStat(new netif_stat {});
    }
    statNetIfMap.clear();
    memset(statSum.data(), 0, sizeof(struct netif_stat));
    strncpy(statSum->iface, "(sum)", 6);

    while (fgets(line.data(), bsiz, fp)) {
        start = line.data();
        pos = strchr(line.data(), ':');
        if (!pos)
            continue;

        *pos++ = '\0';

        auto stat = NetIFStat(new netif_stat {});
        rc = sscanf(start, "%s", stat->iface);
        if (rc != 1)
            continue;

        //******************1****2********************3****4****5****6************************7**
        rc = sscanf(pos, "%llu %llu %*u %*u %*u %*u %llu %llu %llu %llu %*u %*u %*u %*u %*u %llu",
                    &stat->rx_bytes,
                    &stat->rx_packets,
                    &stat->rx_compressed,
                    &stat->multicast,
                    &stat->tx_bytes,
                    &stat->tx_packets,
                    &stat->tx_compressed);
        if (rc != 7)
            continue;

        statNetIfMap[stat->iface] = stat;

        statSum->rx_bytes += stat->rx_bytes;
        statSum->rx_packets += stat->rx_packets;
        statSum->rx_compressed += stat->rx_compressed;
        statSum->multicast += stat->multicast;
        statSum->tx_bytes += stat->tx_bytes;
        statSum->tx_packets += stat->tx_packets;
        statSum->tx_compressed += stat->tx_compressed;

        b = true;
    }
    b = !ferror(fp) && b;
    fclose(fp);
    if (!b) {
        print_err(errno, QString("read %1 failed").arg(PROC_PATH_NET));
    }

    return b;
}

bool SystemStat::readSockStat(SockStatMap &statMap)
{
    bool ok {true};

    auto parseSocks = [](int family, int proto, const char *proc, SockStatMap & statMap) -> bool {
        bool ok {true};
        FILE *fp {};
        const size_t BLEN = 4096;
        QByteArray buffer {BLEN, 0};
        int nr {};
        ino_t ino {};
        char s_addr[128] {}, d_addr[128] {};
        uint64_t hash {};
        QByteArray fmtbuf {};
        uint64_t cchash[2] {};
        QString patternA {}, patternB {};

        errno = 0;
        if (!(fp = fopen(proc, "r")))
        {
            print_err(errno, QString("open %1 failed").arg(PROC_PATH_SOCK_TCP));
            return !ok;
        }

        while (fgets(buffer.data(), BLEN, fp))
        {
            auto stat = QSharedPointer<struct sock_stat_t>::create();

            //*****************************************************************
            nr = sscanf(buffer.data(), "%*s %64[0-9A-Fa-f]:%x %64[0-9A-Fa-f]:%x %*x %*s %*s %*s %u %*u %ld",
                        s_addr,
                        &stat->s_port,
                        d_addr,
                        &stat->d_port,
                        &stat->uid,
                        &ino);

            // ignore first line
            if (nr == 0)
                continue;

            // socket still in waiting state
            if (ino == 0) {
                continue;
            }

            stat->ino = ino;
            stat->sa_family = family;
            stat->proto = proto;

            // saddr & daddr
            if (family == AF_INET6) {
                sscanf(s_addr, "%08x%08x%08x%08x",
                       &stat->s_addr.in6.s6_addr32[0],
                        &stat->s_addr.in6.s6_addr32[1],
                        &stat->s_addr.in6.s6_addr32[2],
                        &stat->s_addr.in6.s6_addr32[3]);
                sscanf(d_addr, "%08x%08x%08x%08x",
                       &stat->d_addr.in6.s6_addr32[0],
                        &stat->d_addr.in6.s6_addr32[1],
                        &stat->d_addr.in6.s6_addr32[2],
                        &stat->d_addr.in6.s6_addr32[3]);
                // convert ipv4 mapped ipv6 address to ipv4
                if (stat->s_addr.in6.s6_addr32[0] == 0x0 &&
                        stat->s_addr.in6.s6_addr32[1] == 0x0 &&
                        stat->s_addr.in6.s6_addr32[2] == 0xffff0000) {
                    stat->sa_family = AF_INET;
                    stat->s_addr.in4.s_addr = stat->s_addr.in6.s6_addr32[3];
                    stat->d_addr.in4.s_addr = stat->d_addr.in6.s6_addr32[3];
                }
            } else {
                sscanf(s_addr, "%x", &stat->s_addr.in4.s_addr);
                sscanf(d_addr, "%x", &stat->d_addr.in4.s_addr);
            }

            if (stat->sa_family == AF_INET) {
                char saddr_str[INET_ADDRSTRLEN + 1] {}, daddr_str[INET_ADDRSTRLEN + 1] {};
                inet_ntop(AF_INET, &stat->s_addr.in4, saddr_str, INET_ADDRSTRLEN);
                inet_ntop(AF_INET, &stat->d_addr.in4, daddr_str, INET_ADDRSTRLEN);

                patternA = QString("%1:%2-%3:%4").arg(saddr_str).arg(stat->s_port).arg(daddr_str).arg(stat->d_port);
                if (proto == IPPROTO_TCP) {
                    patternB = QString("%1:%2-%3:%4").arg(daddr_str).arg(stat->d_port).arg(saddr_str).arg(stat->s_port);
                }

            } else if (stat->sa_family == AF_INET6) {
                char saddr6_str[INET6_ADDRSTRLEN + 1] {}, daddr6_str[INET6_ADDRSTRLEN + 1] {};
                inet_ntop(AF_INET6, &stat->s_addr.in6, saddr6_str, INET6_ADDRSTRLEN);
                inet_ntop(AF_INET6, &stat->d_addr.in6, daddr6_str, INET6_ADDRSTRLEN);

                patternA = QString("%1:%2-%3:%4").arg(saddr6_str).arg(stat->s_port).arg(daddr6_str).arg(stat->d_port);
                if (proto == IPPROTO_TCP) {
                    patternB = QString("%1:%2-%3:%4").arg(daddr6_str).arg(stat->d_port).arg(saddr6_str).arg(stat->s_port);
                }

            } else {
                // unexpected here
            }

            fmtbuf = patternA.toLocal8Bit();
            utils::hash(fmtbuf.constData(), fmtbuf.length(), utils::global_seed, cchash);
            hash = cchash[0];
            statMap.insert(hash, stat);

            // if it's TCP, we need add reverse mapping due to its bidirectional piping feature,
            // otherwise we wont be able to get the inode
            if (proto == IPPROTO_TCP) {
                fmtbuf = patternB.toLocal8Bit();
                utils::hash(fmtbuf.constData(), fmtbuf.length(), utils::global_seed, cchash);
                hash = cchash[0];
                statMap.insert(hash, stat);
            }
        }
        if (ferror(fp))
        {
            ok = !ok;
            print_err(errno, QString("read %1 failed").arg(proc));
        }
        fclose(fp);

        return ok;
    };

    statMap.clear();

    ok = parseSocks(AF_INET, IPPROTO_TCP, PROC_PATH_SOCK_TCP, statMap);
    ok = parseSocks(AF_INET, IPPROTO_UDP, PROC_PATH_SOCK_UDP, statMap) && ok;
    ok = parseSocks(AF_INET6, IPPROTO_TCP, PROC_PATH_SOCK_TCP6, statMap) && ok;
    ok = parseSocks(AF_INET6, IPPROTO_UDP, PROC_PATH_SOCK_UDP6, statMap) && ok;

    return ok;
}

bool SystemStat::readNetIfAddrs(NetIFAddrsMap &addrsMap)
{
    struct ifaddrs *addr_hdr, *addr_p;

    errno = 0;
    if (getifaddrs(&addr_hdr) == -1) {
        qWarning() << QString("getifaddrs failed: [%1] %2").arg(errno).arg(strerror(errno));
        return false;
    }

    for (addr_p = addr_hdr; addr_p != nullptr; addr_p = addr_p->ifa_next) {
        if (!addr_p->ifa_addr)
            continue;

        auto netifAddr = QSharedPointer<struct net_ifaddr_t>::create();
        netifAddr->family = addr_p->ifa_addr->sa_family;
        if (netifAddr->family == AF_INET) {
            netifAddr->addr.in4 = reinterpret_cast<struct sockaddr_in *>(addr_p->ifa_addr)->sin_addr;
        } else if (netifAddr->family == AF_INET6) {
            netifAddr->addr.in6 = reinterpret_cast<struct sockaddr_in6 *>(addr_p->ifa_addr)->sin6_addr;
        } else {
            continue;
        }

        strncpy(netifAddr->iface, addr_p->ifa_name, strlen(addr_p->ifa_name) + 1);
        addrsMap.insert(netifAddr->iface, netifAddr);
    }
    freeifaddrs(addr_hdr);

    if (addrsMap.size() > 0)
        return true;
    else
        return false;
}

QString SystemStat::getCurrentRealUserName()
{
    static QString name {};

    if (name.isEmpty()) {
        name = SystemStat::getUserName(getuid());
    }
    return name;
}

QString SystemStat::getCurrentRealGroupName()
{
    static QString name {};

    if (name.isEmpty()) {
        name = SystemStat::getGroupName(getgid());
    }
    return name;
}

QString SystemStat::getCurrentEffectiveUserName()
{
    static QString name {};

    if (name.isEmpty()) {
        name = SystemStat::getUserName(geteuid());
    }
    return name;
}

QString SystemStat::getCurrentEffectiveGroupName()
{
    static QString name {};

    if (name.isEmpty()) {
        name = SystemStat::getGroupName(getegid());
    }
    return name;
}

QString SystemStat::getUserName(uid_t uid)
{
    struct passwd *pwd = getpwuid(uid);
    if (pwd) {
        return { pwd->pw_name };
    } else {
        return { getenv("USER") };
    }
}

QString SystemStat::getGroupName(gid_t gid)
{
    struct passwd *pwd = getpwuid(gid);
    if (pwd) {
        return { pwd->pw_name };
    } else {
        return { getenv("GROUP") };
    }
}
