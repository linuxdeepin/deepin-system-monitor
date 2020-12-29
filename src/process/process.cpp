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

#include "process.h"

#include "system/system_monitor.h"
#include "common/thread_manager.h"
#include "common/base_thread.h"
#include "system/system_monitor_thread.h"

#include <QMap>
#include <QList>
#include <QDebug>

#include <memory>

#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define PROC_PATH "/proc"
#define PROC_STAT_PATH "/proc/%u/stat"
#define PROC_STATUS_PATH "/proc/%u/status"
#define PROC_STATM_PATH "/proc/%u/statm"
#define PROC_CMDLINE_PATH "/proc/%u/cmdline"
#define PROC_ENVIRON_PATH "/proc/%u/environ"
#define PROC_IO_PATH "/proc/%u/io"
#define PROC_FD_PATH "/proc/%u/fd"
#define PROC_FD_NAME_PATH "/proc/%u/fd/%s"
#define PROC_SCHEDSTAT_PATH "/proc/%u/schedstat"

using namespace common::alloc;
using namespace common::init;
using namespace common::core;
using namespace common::error;
using namespace core::system;

namespace core {
namespace process {

Process::Process()
    : d(new ProcessPrivate())
{
}
Process::Process(pid_t pid)
    : d(new ProcessPrivate())
{
    d->pid = pid;
    readProcessInfo();
}
Process::Process(const Process &other)
    : d(other.d)
{
}
Process &Process::operator=(const Process &rhs)
{
    if (this == &rhs)
        return *this;

    d = rhs.d;
    return *this;
}
Process::~Process()
{
}

time_t Process::startTime() const
{
    auto *thread = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread);
    if (thread) {
        auto *monitor = thread->threadJobInstance<SystemMonitor>();
        if (monitor) {
            auto sysInfo = monitor->sysInfo();
            return sysInfo.btime().tv_sec + time_t(d->start_time / HZ);
        }
    }

    return 0;
}

void Process::readProcessInfo()
{
    d->valid = true;

    bool ok = true;

    ok = ok && readStat();
    ok = ok && readCmdline();
    readEnviron();
    readSchedStat();
    ok = ok && readStatus();
    ok = ok && readStatm();
    readIO();
    readSockInodes();

    d->valid = d->valid && ok;
}

// read /proc/[pid]/stat
bool Process::readStat()
{
    bool ok {true};
    char path[256];
    QByteArray buf;
    int fd, rc;
    ssize_t sz;
    char *pos, *begin;
    uFD ufd;

    buf.reserve(1025);

    errno = 0;
    sprintf(path, PROC_STAT_PATH, d->pid);
    // open /proc/[pid]/stat
    if ((fd = open(path, O_RDONLY)) < 0) {
        print_errno(errno, QString("open %1 failed").arg(path));
        return !ok;
    }
    ufd.reset(new int {fd});

    // read data
    sz = read(fd, buf.data(), 1024);
    if (sz < 0) {
        print_errno(errno, QString("read %1 failed").arg(path));
        return !ok;
    }
    buf.data()[sz] = '\0';

    // get process name between (...)
    begin = strchr(buf.data(), '(');
    begin += 1;

    pos = strrchr(buf.data(), ')');
    if (!pos) {
        return !ok;
    }

    *pos = '\0';
    // process name (may be truncated by kernel if it's too long)
    d->name = QByteArray(begin);

    pos += 2;

    //****************3**4**5***********************************14***15**
    rc = sscanf(pos, "%c %d %d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu"
                     //*16***17******19*20******22************************************
                     " %lld %lld %*d %d %u %*u %llu %*u %*u %*u %*u %*u %*u %*u %*u"
                     //********************************39*40*41******43***44**********
                     " %*u %*u %*u %*u %*u %*u %*u %*u %u %u %u %*u %llu %lld\n",
                &d->state, // 3
                &d->ppid, // 4
                &d->pgid, // 5
                &d->utime, // 14
                &d->stime, // 15
                &d->cutime, // 16
                &d->cstime, // 17
                &d->nice, // 19
                &d->nthreads, // 20
                &d->start_time, // 22
                &d->processor, // 39
                &d->rt_prio, // 40
                &d->policy, // 41
                &d->guest_time, // 43
                &d->cguest_time); // 44
    if (rc < 15) {
        return !ok;
    }
    // have guest & cguest time
    if (rc < 17) {
        d->guest_time = d->cguest_time = 0;
    }

    return ok;
}

// read /proc/[pid]/cmdline
bool Process::readCmdline()
{
    bool ok = true;
    char path[128] {};
    const size_t bsiz = 4096;
    QByteArray cmd;
    cmd.reserve(bsiz);
    size_t nb;
    char *begin, *cur, *end;

    sprintf(path, PROC_CMDLINE_PATH, d->pid);

    errno = 0;
    // open /proc/[pid]/cmdline
    uFile fp(fopen(path, "r"));
    if (!fp) {
        print_errno(errno, QString("open %1 failed").arg(path));
        return !ok;
    }

    nb = fread(cmd.data(), 1, bsiz - 1, fp.get());
    if (ferror(fp.get())) {
        print_errno(errno, QString("read %1 failed").arg(path));
        return !ok;
    }

    cmd.data()[nb] = '\0';

    begin = cur = cmd.data();
    end = cmd.data() + nb;
    while (cur < end) {
        // cmdline may sperarted by null character
        if (*cur == '\0') {
            QByteArray buf(begin);
            d->cmdline << buf;
            begin = cur + 1;
        }
        ++cur;
    }
    if (begin < end) {
        QByteArray buf(begin);
        d->cmdline << buf;
    }

    return ok;
}

// read /proc/[pid]/environ
void Process::readEnviron()
{
    const size_t sz = 1024;
    char path[128] {};
    ssize_t nb;
    QByteArray sbuf {};
    char buf[sz + 1] {};
    int fd;
    uFD ufd;

    sprintf(path, PROC_ENVIRON_PATH, d->pid);

    errno = 0;
    // open /proc/[pid]/environ
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        print_errno(errno, QString("open %1 failed").arg(path));
        return;
    }
    ufd.reset(new int {fd});

    while ((nb = read(fd, buf, sz))) {
        buf[nb] = '\0';
        sbuf.append(buf, int(nb));
    }
    if (nb == 0 && errno != 0) {
        print_errno(errno, QString("read %1 failed").arg(path));
        return;
    }

    if (sbuf.size() > 0) {
        auto elist = sbuf.split('\0');
        for (auto it : elist) {
            // it: name=value pair
            auto kvp = it.split('=');
            if (kvp.size() == 2) {
                d->environ[kvp[0]] = kvp[1];
            }
        }
    } // ::if(sbuf)
}

// read /proc/[pid]/schedstat
void Process::readSchedStat()
{
    const size_t bsiz = 1024;
    QByteArray buf;
    char path[128];
    int fd, rc;
    ssize_t n;
    unsigned long long wtime = 0;
    uFD ufd;

    buf.reserve(bsiz);
    sprintf(path, PROC_SCHEDSTAT_PATH, d->pid);

    errno = 0;
    // open /proc/[pid]/schedstat
    if ((fd = open(path, O_RDONLY)) < 0) {
        print_errno(errno, QString("open %1 failed").arg(path));
        return;
    }
    ufd.reset(new int {fd});

    n = read(fd, buf.data(), bsiz - 1);
    if (n < 0) {
        print_errno(errno, QString("read %1 failed").arg(path));
        return;
    }

    buf.data()[n] = '\0';
    rc = sscanf(buf.data(), "%*u %llu %*d", &wtime);
    if (rc == 1) {
        d->wtime = wtime * HZ / 1000000000;
    }
}

// read /proc/[pid]/status
bool Process::readStatus()
{
    bool ok {true};
    const size_t bsiz = 256;
    QByteArray buf;
    char path[128];

    buf.reserve(bsiz);
    sprintf(path, PROC_STATUS_PATH, d->pid);

    errno = 0;
    uFile fp(fopen(path, "r"));
    // open /proc/[pid]/status
    if (!fp) {
        print_errno(errno, QString("open %1 failed").arg(path));
        return !ok;
    }

    // scan each line
    while (fgets(buf.data(), bsiz, fp.get())) {
        if (!strncmp(buf.data(), "Umask:", 6)) {
            sscanf(buf.data() + 7, "%u", &d->mask);
        } else if (!strncmp(buf.data(), "State:", 6)) {
            sscanf(buf.data() + 7, "%c %*s", &d->state);
        } else if (!strncmp(buf.data(), "Uid:", 4)) {
            sscanf(buf.data() + 5, "%u %u %u %u",
                   &d->uid,
                   &d->euid,
                   &d->suid,
                   &d->fuid);
        } else if (!strncmp(buf.data(), "Gid:", 4)) {
            sscanf(buf.data() + 5, "%u %u %u %u",
                   &d->gid,
                   &d->egid,
                   &d->sgid,
                   &d->fgid);
        }
    } // ::while(fgets)

    if (ferror(fp.get())) {
        print_errno(errno, QString("read %1 failed").arg(path));
        return !ok;
    }

    return ok;
}

// read /proc/[pid]/statm
bool Process::readStatm()
{
    bool ok {true};
    int fd;
    const size_t bsiz = 1024;
    char path[128] {}, buf[bsiz + 1] {};
    ssize_t nr;
    uFD ufd;

    sprintf(path, PROC_STATM_PATH, d->pid);

    errno = 0;
    // open /proc/[pid]/statm
    if ((fd = open(path, O_RDONLY)) < 0) {
        print_errno(errno, QString("open %1 failed").arg(path));
        return !ok;
    }
    ufd.reset(new int {fd});

    nr = read(fd, buf, bsiz);
    if (nr < 0) {
        print_errno(errno, QString("read %1 failed").arg(path));
        return !ok;
    }

    buf[nr] = '\0';
    // get resident set size & resident shared size in pages
    nr = sscanf(buf, "%*u %llu %llu", &d->rss, &d->shm);
    if (nr != 2) {
        d->rss = 0;
        d->shm = 0;
        print_errno(errno, QString("read %1 failed").arg(path));
    } else {
        // convert to kB
        d->rss <<= kb_shift;
        d->shm <<= kb_shift;
    }
    return ok;
}

// read /proc/[pid]/io
void Process::readIO()
{
    const size_t bsiz = 128;
    char path[128], buf[bsiz];

    sprintf(path, PROC_IO_PATH, d->pid);

    errno = 0;
    // open /proc/[pid]/io
    uFile fp(fopen(path, "r"));
    if (!fp) {
        print_errno(errno, QString("open %1 failed").arg(path));
        return;
    }

    // scan each line
    while (fgets(buf, bsiz, fp.get())) {
        if (!strncmp(buf, "read_bytes", 10)) {
            sscanf(buf + 12, "%llu", &d->read_bytes);
        } else if (!strncmp(buf, "write_bytes", 11)) {
            sscanf(buf + 13, "%llu", &d->write_bytes);
        } else if (!strncmp(buf, "cancelled_write_bytes", 21)) {
            sscanf(buf + 23, "%llu", &d->cancelled_write_bytes);
        }
    } // ::while(fgets)
    if (ferror(fp.get())) {
        print_errno(errno, QString("read %1 failed").arg(path));
    }
}

// read /proc/[pid]/fd
void Process::readSockInodes()
{
    struct dirent *dp;
    char path[128], fdp[256 + 32];
    struct stat sbuf;

    sprintf(path, PROC_FD_PATH, d->pid);

    errno = 0;
    // open /proc/[pid]/fd dir
    uDir dir(opendir(path));
    if (!dir) {
        print_errno(errno, QString("open %1 failed").arg(path));
        return;
    }

    // enumerate each entry
    while ((dp = readdir(dir.get()))) {
        // only if entry name starts with a digit
        if (isdigit(dp->d_name[0])) {
            // open /proc/[pid]/fd/[fd]
            sprintf(fdp, PROC_FD_NAME_PATH, d->pid, dp->d_name);
            memset(&sbuf, 0, sizeof(struct stat));
            if (!stat(fdp, &sbuf)) {
                // get inode if it's a socket descriptor
                if (S_ISSOCK(sbuf.st_mode)) {
                    d->sockInodes << sbuf.st_ino;
                }
            } // ::if(stat)
        } // ::if(isdigit)
    } // ::while(readdir)
    if (errno) {
        print_errno(errno, QString("read %1 failed").arg(path));
    }
}

QString getPriorityName(int prio)
{
    const static QMap<ProcessPriority, QString> priorityMap = {
        {kVeryHighPriority, QApplication::translate("Process.Priority", "Very high")},
        {kHighPriority, QApplication::translate("Process.Priority", "High")},
        {kNormalPriority, QApplication::translate("Process.Priority", "Normal")},
        {kLowPriority, QApplication::translate("Process.Priority", "Low")},
        {kVeryLowPriority, QApplication::translate("Process.Priority", "Very low")},
        {kCustomPriority, QApplication::translate("Process.Priority", "Custom")},
        {kInvalidPriority, QApplication::translate("Process.Priority", "Invalid")}};

    ProcessPriority p = kInvalidPriority;
    if (prio == kVeryHighPriority || prio == kHighPriority || prio == kNormalPriority || prio == kLowPriority || prio == kVeryLowPriority) {
        p = ProcessPriority(prio);
    } else if (prio >= kVeryHighPriorityMax && prio <= kVeryLowPriorityMin) {
        p = kCustomPriority;
    }

    return priorityMap[p];
}

ProcessPriority getProcessPriorityStub(int prio)
{
    if (prio == 0) {
        return kNormalPriority;
    } else if (prio == kVeryHighPriority || prio == kHighPriority || prio == kLowPriority || prio == kVeryLowPriority) {
        return ProcessPriority(prio);
    } else if (prio <= kVeryLowPriorityMin && prio >= kVeryHighPriorityMax) {
        return kCustomPriority;
    } else {
        return kInvalidPriority;
    }
}

} // namespace process
} // namespace core
