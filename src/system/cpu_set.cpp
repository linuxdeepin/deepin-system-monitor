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
#include "cpu_set.h"

#include "common/common.h"
#include "common/thread_manager.h"
#include "system_monitor_thread.h"
#include "system_monitor.h"

#include <QMap>
#include <QByteArray>

#include <ctype.h>
#include <errno.h>
#include <sched.h>

#define PROC_PATH_STAT "/proc/stat"
#define PROC_PATH_CPUINFO "/proc/cpuinfo"

using namespace common::error;
using namespace common::alloc;

namespace core {
namespace system {

/*
* borrowed & modified from LINUX bitmap.c
*/

/*
 * Region 9-38:4/10 describes the following bitmap structure:
 * 0       9  12    18          38
 * .........****......****......****......
 *      ^  ^     ^           ^
 *      start  off   group_len         end
 */
struct region {
    unsigned int start;
    unsigned int off;
    unsigned int group_len;
    unsigned int end;
};

static bool cpuset_set_region(const struct region *r,
                              cpu_set_t *set, size_t size)
{
    unsigned int start;

    if (r->end >= size)
        return false;

    for (start = r->start; start <= r->end; start += r->group_len) {
        auto len = std::min(r->end - start + 1, r->off);
        for (size_t pos = 0; pos < len; pos++)
            CPU_SET_S(start + pos, size, set);
    }

    return true;
}

static bool cpuset_check_region(const struct region *r)
{
    if (r->start > r->end || r->group_len == 0 || r->off > r->group_len)
        return false;

    return true;
}

static const char *cpuset_getnum(const char *str, unsigned int *num)
{
    unsigned long long n;
    char *ptr;

    errno = 0;
    n = strtoull(str, &ptr, 10);
    if (errno == ERANGE || n != uint(n))
        return nullptr;

    *num = uint(n);
    return ptr;
}

static inline bool end_of_str(char c)
{
    return c == '\0' || c == '\n';
}

static inline bool __end_of_region(char c)
{
    return isspace(c) || c == ',';
}

static inline bool end_of_region(char c)
{
    return __end_of_region(c) || end_of_str(c);
}

/*
 * The format allows commas and whitespaces at the beginning
 * of the region.
 */
static const char *cpuset_find_region(const char *str)
{
    while (__end_of_region(*str))
        str++;

    return end_of_str(*str) ? nullptr : str;
}

static const char *cpuset_parse_region(const char *str, struct region *r)
{
    str = cpuset_getnum(str, &r->start);
    if (!str)
        return nullptr;

    if (end_of_region(*str))
        goto no_end;

    if (*str != '-')
        return nullptr;

    str = cpuset_getnum(str + 1, &r->end);
    if (!str)
        return nullptr;

    if (end_of_region(*str))
        goto no_pattern;

    if (*str != ':')
        return nullptr;

    str = cpuset_getnum(str + 1, &r->off);
    if (!str)
        return nullptr;

    if (*str != '/')
        return nullptr;

    return cpuset_getnum(str + 1, &r->group_len);

no_end:
    r->end = r->start;
no_pattern:
    r->off = r->end + 1;
    r->group_len = r->end + 1;

    return end_of_str(*str) ? nullptr : str;
}

/**
 * cpuset_parselist - convert list format ASCII string to cpuset
 * @buf: read user string from this buffer; must be terminated
 *    with a \0 or \n.
 * @maskp: write resulting mask here
 * @nmaskbits: number of bits in mask to be written
 *
 * Input format is a comma-separated list of decimal numbers and
 * ranges.  Consecutively set bits are shown as two hyphen-separated
 * decimal numbers, the smallest and largest bit numbers set in
 * the range.
 * Optionally each range can be postfixed to denote that only parts of it
 * should be set. The range will divided to groups of specific size.
 * From each group will be used only defined amount of bits.
 * Syntax: range:used_size/group_size
 * Example: 0-1023:2/256 ==> 0,1,256,257,512,513,768,769
 *
 * Returns: 0 on success, -errno on invalid input strings. Error values:
 *
 *   - ``-EINVAL``: wrong region format
 *   - ``-EINVAL``: invalid character in string
 *   - ``-ERANGE``: bit number specified too large for mask
 *   - ``-EOVERFLOW``: integer overflow in the input parameters
 */
bool cpuset_parse_list(const char *buf, cpu_set_t *set, size_t size)
{
    struct region r;
    bool ok;

    CPU_ZERO_S(size, set);

    while (buf) {
        buf = cpuset_find_region(buf);
        if (!buf)
            return false;

        buf = cpuset_parse_region(buf, &r);
        if (!buf)
            return false;

        ok = cpuset_check_region(&r);
        if (!ok)
            return false;

        ok = cpuset_set_region(&r, set, size);
        if (!ok)
            return false;
    }

    return true;
}

CPUSet::CPUSet()
    : d(new CPUSetPrivate())
{
}
CPUSet::CPUSet(const CPUSet &other)
    : d(other.d)
{
}
CPUSet &CPUSet::operator=(const CPUSet &rhs)
{
    if (this == &rhs)
        return *this;

    d = rhs.d;
    return *this;
}
CPUSet::~CPUSet()
{
}

void CPUSet::update()
{
    read_stats();
    read_overall_info();
}

void CPUSet::read_stats()
{
    FILE *fp;
    uFile fPtr;
    QByteArray line(BUFSIZ, '\0');
    int ncpu = 0;
    int nr;

    if (!(fp = fopen(PROC_PATH_STAT, "r"))) {
        print_errno(errno, QString("open %1 failed").arg(PROC_PATH_STAT));
        return;
    } // ::if(fopen)
    fPtr.reset(fp);

    while (fgets(line.data(), BUFSIZ, fp)) {
        if (!strncmp(line.data(), "cpu ", 4)) {
            if (!d->m_stat) {
                d->m_stat = std::make_shared<struct cpu_stat_t>();
                *d->m_stat = {};
            }

            if (d->m_stat) {
                // all cpu stat in jiffies
                nr = sscanf(line.data() + 5, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                            &d->m_stat->user,
                            &d->m_stat->nice,
                            &d->m_stat->sys,
                            &d->m_stat->idle,
                            &d->m_stat->iowait,
                            &d->m_stat->hardirq,
                            &d->m_stat->softirq,
                            &d->m_stat->steal,
                            &d->m_stat->guest,
                            &d->m_stat->guest_nice);

                if (nr == 10) {
                    // usage calc
                    QByteArray cpu {"cpu"};
                    d->m_stat->cpu = cpu;

                    if (!d->m_usage) {
                        d->m_usage = std::make_shared<struct cpu_usage_t>();
                        *d->m_usage = {};
                    }

                    if (d->m_usage) {
                        d->m_usage->cpu = cpu;
                        d->m_usage->total = d->m_stat->user + d->m_stat->nice + d->m_stat->sys + d->m_stat->idle + d->m_stat->iowait + d->m_stat->hardirq + d->m_stat->softirq + d->m_stat->steal;
                        d->m_usage->idle = d->m_stat->idle + d->m_stat->iowait;
                    } // ::if(usage)
                } else
                    print_errno(errno, QString("read %1 failed, cpu").arg(PROC_PATH_STAT));
            } // ::if(m_stat)
        } else if (!strncmp(line.data(), "cpu", 3)) {
            // per cpu stat in jiffies
            auto stat = std::make_shared<struct cpu_stat_t>();
            if (stat) {
                *stat = {};

                nr = sscanf(line.data() + 3, "%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
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

                if (nr == 11) {
                    QByteArray cpu {"cpu"};
                    cpu.append(QByteArray::number(ncpu));
                    stat->cpu = cpu;

                    // usage calc
                    auto usage = std::make_shared<struct cpu_usage_t>();
                    if (usage) {
                        *usage = {};
                        usage->cpu = cpu;
                        usage->total = stat->user + stat->nice + stat->sys + stat->idle + stat->iowait + stat->hardirq + stat->softirq + stat->steal;
                        usage->idle = stat->idle + stat->iowait;

                        d->m_statDB[stat->cpu] = stat;
                        d->m_usageDB[usage->cpu] = usage;
                    } //::if(usage)
                } else
                    print_errno(errno, QString("read %1 failed, cpu%2").arg(PROC_PATH_STAT).arg(ncpu));
            } // ::if(stat)
        } else if (!strncmp(line.data(), "btime", 5)) {
            // read boot time in seconds since epoch
            struct timeval btime {
            };
            long nsec {};
            int nm = sscanf(line.data() + 5, "%ld", &nsec);
            if (nm == 1) {
                btime.tv_sec = nsec;
                btime.tv_usec = 0;

                // set sysinfo btime
                auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
                monitor->sysInfo()->set_btime(btime);
            } // ::if(nm == 1)
        } // ::if(btime)
    } // ::while(fgets)

    if (ferror(fp))
        print_errno(errno, QString("read %1 failed").arg(PROC_PATH_STAT));
}

void CPUSet::read_overall_info()
{
    FILE *fp;
    uFile fPtr;
    QByteArray line(BUFSIZ, '\0');

    if (!(fp = fopen(PROC_PATH_CPUINFO, "r"))) {
        print_errno(errno, QString("open %1 failed").arg(PROC_PATH_CPUINFO));
        return;
    } // ::if(fopen)

    fPtr.reset(fp);

    while (fgets(line.data(), BUFSIZ, fp)) {
        if (line.startsWith("vendor")
                || line.startsWith("vendor_id")
                || line.startsWith("CPU implementer")) {
            d->m_vendor = line.mid(line.indexOf(':')).trimmed();

        } else if (line.startsWith("model")
                   || line.startsWith("CPU part")
                   || line.startsWith("cpu model")) {
            // model id
        } else if (line.startsWith("model name")) {
            d->m_model = line.mid(line.indexOf(':')).trimmed();

        } else if (line.startsWith("flags")
                   || line.startsWith("features")
                   || line.startsWith("Features")
                   || line.startsWith("ASEs implemented")
                   || line.startsWith("type")) {
            QByteArray flags = line.mid(line.indexOf(':'));
            if (flags.contains(" vmx "))
                d->m_virtualization = "VT-x";

            else if (flags.contains(" svm "))
                d->m_virtualization = "AMD-V";

        } // ::if
    } // ::while(fgets)

    if (ferror(fp)) {
        print_errno(errno, QString("read %1 failed").arg(PROC_PATH_CPUINFO));
        return;
    }

    // read kernel_max
    // read online/possible/present
    // foreach(pos)->present
    CPUInfo info;
}

} // namespace system
} // namespace core
