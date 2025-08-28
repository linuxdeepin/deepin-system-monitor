// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process.h"
#include "ddlog.h"
#include "private/process_p.h"
#include "system/device_db.h"
#include "process/process_db.h"
#include "system/sys_info.h"
#include "system/cpu_set.h"
#include "system/netif_info_db.h"
#include "wm/wm_window_list.h"

#include <QMap>
#include <QList>
#include <QDebug>
#include <QApplication>
#include <QVariantMap>

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
using namespace DDLog;

namespace core {
namespace process {

QString getPriorityName(int prio)
{
    qCDebug(app) << "Getting priority name for value:" << prio;
    const static QMap<ProcessPriority, QString> priorityMap = {
        {kVeryHighPriority, QApplication::translate("Process.Priority", "Very high")},
        {kHighPriority, QApplication::translate("Process.Priority", "High")},
        {kNormalPriority, QApplication::translate("Process.Priority", "Normal")},
        {kLowPriority, QApplication::translate("Process.Priority", "Low")},
        {kVeryLowPriority, QApplication::translate("Process.Priority", "Very low")},
        {kCustomPriority, QApplication::translate("Process.Priority", "Custom")},
        {kInvalidPriority, QApplication::translate("Process.Priority", "Invalid")}
    };

    ProcessPriority p = kInvalidPriority;
    if (prio == kVeryHighPriority || prio == kHighPriority || prio == kNormalPriority || prio == kLowPriority || prio == kVeryLowPriority) {
        p = ProcessPriority(prio);
    } else if (prio >= kVeryHighPriorityMax && prio <= kVeryLowPriorityMin) {
        p = kCustomPriority;
    }

    qCDebug(app) << "Priority maps to:" << priorityMap[p];
    return priorityMap[p];
}

ProcessPriority getProcessPriorityStub(int prio)
{
    qCDebug(app) << "Getting process priority stub for value:" << prio;
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

Process::Process()
    : d(new ProcessPrivate())
{
    qCDebug(app) << "Process object created";
}
Process::Process(pid_t pid)
    : d(new ProcessPrivate())
{
    qCDebug(app) << "Process object created for pid" << pid;
    d->pid = pid;
}
Process::Process(const Process &other)
    : d(other.d)
{
    qCDebug(app) << "Process object copied from pid" << other.d->pid;
}
Process &Process::operator=(const Process &rhs)
{
    if (this == &rhs)
        return *this;
    // qCDebug(app) << "Process object assigned from pid" << rhs.d->pid;

    d = rhs.d;
    return *this;
}

Process::~Process()
{
    qCDebug(app) << "Process object destroyed for pid" << (d ? d->pid : -1);
}

time_t Process::startTime() const
{
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    time_t st = monitor->sysInfo()->btime().tv_sec + time_t(d->start_time / HZ);
    qCDebug(app) << "Start time for pid" << d->pid << "is" << st;
    return st;
}

timeval Process::procuptime() const
{
    qCDebug(app) << "Process uptime for pid" << d->pid << "is" << d->uptime.tv_sec << "s";
    return d->uptime;
}

void Process::readProcessVariableInfo()
{
    qCDebug(app) << "Reading variable info for pid" << d->pid;
    d->valid = true;

    bool ok = true;
    ok = ok && readStat();
    readSchedStat();
    ok = ok && readStatm();

    readIO();
    readSockInodes();

    d->proc_name.refreashProcessName(this);
    d->uptime = SysInfo::instance()->uptime();

    calculateProcessMetrics();

    d->valid = d->valid && ok;
    qCDebug(app) << "Finished reading variable info for pid" << d->pid << "valid:" << d->valid;
}

void Process::readProcessSimpleInfo(bool skipStatReading)
{
    qCDebug(app) << "Reading simple info for pid" << d->pid;
    d->valid = true;
    bool ok = true;
    readEnviron();

    // 在DKapture模式下跳过stat读取，因为DKapture已提供这些数据
    if (!skipStatReading) {
        ok = ok && readStat();
        ok = ok && readStatus();  // status - 包含uid等信息，DKapture模式下由后端提供
    }

    ok = ok && readCmdline(); // cmdline - DKapture无法提供，两种模式都需要

    d->usrerName = SysInfo::userName(d->uid);
    d->proc_name.refreashProcessName(this);
    d->proc_icon.refreashProcessIcon(this);

    d->apptype = kNoFilter;
    const QVariant &euid = ProcessDB::instance()->processEuid();
    WMWindowList *wmwindowList = ProcessDB::instance()->windowList();

    if (euid == d->uid && (wmwindowList->isGuiApp(d->pid)
                           || wmwindowList->isTrayApp(d->pid)
                           || wmwindowList->isDesktopEntryApp(d->pid))) {
        qCDebug(app) << "Process" << d->pid << "is a GUI/Tray/Desktop app";
        d->apptype = kFilterApps;
    } else if (euid == d->uid) {
        qCDebug(app) << "Process" << d->pid << "is a current user app";
        d->apptype = kFilterCurrentUser;
    }

    d->valid = d->valid && ok;
    qCDebug(app) << "Finished reading simple info for pid" << d->pid << "valid:" << d->valid;
}

void Process::readProcessInfo()
{
    qCDebug(app) << "Reading full process info for pid" << d->pid;
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

    d->usrerName = SysInfo::userName(d->uid);
    d->proc_name.refreashProcessName(this);
    d->proc_icon.refreashProcessIcon(this);
    d->uptime = SysInfo::instance()->uptime();

    CPUSet *cpuset = DeviceDB::instance()->cpuSet();
    ProcessSet *procset =  ProcessDB::instance()->processSet();

    auto recentProcptr = procset->getRecentProcStage(d->pid);
    auto validrecentPtr = recentProcptr.lock();
    qreal timedelta = d->stime + d->utime;
    if (validrecentPtr) {
        qCDebug(app) << "Found recent process stage for pid" << d->pid;
        timedelta = timedelta - validrecentPtr->ptime;
        struct DiskIO io = {validrecentPtr->read_bytes, validrecentPtr->write_bytes, validrecentPtr->cancelled_write_bytes};
        d->diskIOSample->addSample(new DISKIOSampleFrame(validrecentPtr->uptime, io));

        d->networkIOSample->addSample(new IOSampleFrame(validrecentPtr->uptime, {0, 0}));
    }
    d->cpuUsageSample->addSample(new CPUUsageSampleFrame(qMax(0., timedelta) / cpuset->getUsageTotalDelta() * 100));

    struct DiskIO io = {d->read_bytes, d->write_bytes, d->cancelled_write_bytes};
    d->diskIOSample->addSample(new DISKIOSampleFrame(d->uptime, io));

    auto pair = d->diskIOSample->recentSamplePair();
    struct IOPS iops = DISKIOSampleFrame::diskiops(pair.first, pair.second);
    d->diskIOSpeedSample->addSample(new IOPSSampleFrame(iops));

    d->apptype = kNoFilter;
    const QVariant &euid = ProcessDB::instance()->processEuid();
    WMWindowList *wmwindowList = ProcessDB::instance()->windowList();

    if (euid == d->uid && (wmwindowList->isGuiApp(d->pid)
                           || wmwindowList->isTrayApp(d->pid)
                           || wmwindowList->isDesktopEntryApp(d->pid))) {
        qCDebug(app) << "Process" << d->pid << "is a GUI/Tray/Desktop app";
        d->apptype = kFilterApps;
    } else if (euid == d->uid) {
        qCDebug(app) << "Process" << d->pid << "is a current user app";
        d->apptype = kFilterCurrentUser;
    }

    qulonglong sum_recv = 0;
    qulonglong sum_send = 0;

    for (int i = 0; i < d->sockInodes.size(); ++i) {
        SockIOStat sockIOStat;
        bool result = NetifMonitor::instance()->getSockIOStatByInode(d->sockInodes[i], sockIOStat);
        if (result) {
            sum_recv += sockIOStat->rx_bytes;
            sum_send += sockIOStat->tx_bytes;
        }
    }
    d->networkIOSample->addSample(new IOSampleFrame(d->uptime, {sum_recv, sum_send}));

    auto netpair = d->networkIOSample->recentSamplePair();
    struct IOPS netiops = IOSampleFrame::iops(netpair.first, netpair.second);
    d->networkBandwidthSample->addSample(new IOPSSampleFrame(netiops));

    d->valid = d->valid && ok;
    qCDebug(app) << "Finished reading full info for pid" << d->pid << "valid:" << d->valid;
}

// read /proc/[pid]/stat
bool Process::readStat()
{
    qCDebug(app) << "Reading stat for pid" << d->pid;
    bool ok {true};
    char path[256];
    QByteArray buf;
    int fd, rc;
    ssize_t sz;
    char *pos, *begin;

    buf.reserve(1025);

    errno = 0;
    sprintf(path, PROC_STAT_PATH, d->pid);
    if(access(path, R_OK) != 0) {
        qCWarning(app) << "Cannot access stat file for process" << d->pid;
        return !ok;
    }
        
    // open /proc/[pid]/stat
    if ((fd = open(path, O_RDONLY)) < 0) {
        qCWarning(app) << "Failed to open stat file for process" << d->pid << "Error:" << strerror(errno);
        print_errno(errno, QString("open %1 failed").arg(path));
        return !ok;
    }

    // read data
    sz = read(fd, buf.data(), 1024);
    close(fd);
    if (sz < 0) {
        qCWarning(app) << "Failed to read stat file for process" << d->pid << "Error:" << strerror(errno);
        print_errno(errno, QString("read %1 failed").arg(path));
        return !ok;
    }
    buf.data()[sz] = '\0';

    // get process name between (...)
    begin = strchr(buf.data(), '(');
    begin += 1;

    pos = strrchr(buf.data(), ')');
    if (!pos) {
        qCWarning(app) << "Invalid stat file format for process" << d->pid;
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
        qCWarning(app) << "Failed to parse stat file for process" << d->pid << "Parsed fields:" << rc;
        return !ok;
    }
    // have guest & cguest time
    if (rc < 17) {
        d->guest_time = d->cguest_time = 0;
    }

    qCDebug(app) << "Successfully read stat for pid" << d->pid;
    return ok;
}

// read /proc/[pid]/cmdline
bool Process::readCmdline()
{
    qCDebug(app) << "Reading cmdline for pid" << d->pid;
    bool ok = true;
    char path[128] {};
    const size_t bsiz = 4096;
    QByteArray cmd;
    cmd.reserve(bsiz);
    size_t nb;
    char *begin, *cur, *end;

    sprintf(path, PROC_CMDLINE_PATH, d->pid);
    if(access(path, R_OK) != 0) {
        qCWarning(app) << "Cannot access cmdline file for process" << d->pid;
        return !ok;
    }

    errno = 0;
    // open /proc/[pid]/cmdline
    uFile fp(fopen(path, "r"));
    if (!fp) {
        qCWarning(app) << "Failed to open cmdline file for process" << d->pid << "Error:" << strerror(errno);
        print_errno(errno, QString("open %1 failed").arg(path));
        return !ok;
    }

    nb = fread(cmd.data(), 1, bsiz - 1, fp.get());
    if (ferror(fp.get())) {
        qCWarning(app) << "Failed to read cmdline file for process" << d->pid << "Error:" << strerror(errno);
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

    qCDebug(app) << "Successfully read cmdline for pid" << d->pid;
    return ok;
}

// read /proc/[pid]/environ
void Process::readEnviron()
{
    qCDebug(app) << "Reading environ for pid" << d->pid;
    const size_t sz = 1024;
    char path[128] {};
    ssize_t nb;
    QByteArray sbuf {};
    char buf[sz + 1] {};
    int fd;

    sprintf(path, PROC_ENVIRON_PATH, d->pid);
    if(access(path, R_OK) != 0) {
        // qCWarning(app) << "Cannot access environment file for process" << d->pid;
        return;
    }

    errno = 0;
    // open /proc/[pid]/environ
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        qCWarning(app) << "Failed to open environment file for process" << d->pid << "Error:" << strerror(errno);
        print_errno(errno, QString("open %1 failed").arg(path));
        return;
    }

    while ((nb = read(fd, buf, sz))) {
        buf[nb] = '\0';
        sbuf.append(buf, int(nb));
    }
    close(fd);

    if (nb == 0 && errno != 0) {
        qCWarning(app) << "Failed to read environment file for process" << d->pid << "Error:" << strerror(errno);
        print_errno(errno, QString("read %1 failed").arg(path));
        return;
    }

    if (sbuf.size() > 0) {
        qCDebug(app) << "Processing environment variables for pid" << d->pid;
        auto elist = sbuf.split('\0');
        for (auto it : elist) {
            // it: name=value pair
            auto kvp = it.split('=');
            if (kvp.size() == 2) {
                d->environ[kvp[0]] = kvp[1];
            }
        }
    }
    qCDebug(app) << "Finished reading environ for pid" << d->pid;
}

// read /proc/[pid]/schedstat
void Process::readSchedStat()
{
    qCDebug(app) << "Reading schedstat for pid" << d->pid;
    const size_t bsiz = 1024;
    QByteArray buf;
    char path[128];
    int fd, rc;
    ssize_t n;
    unsigned long long wtime = 0;

    buf.reserve(bsiz);
    sprintf(path, PROC_SCHEDSTAT_PATH, d->pid);
    if(access(path, R_OK) != 0) {
        qCWarning(app) << "Cannot access schedstat file for process" << d->pid;
        return;
    }

    errno = 0;
    // open /proc/[pid]/schedstat
    if ((fd = open(path, O_RDONLY)) < 0) {
        qCWarning(app) << "Failed to open schedstat file for process" << d->pid << "Error:" << strerror(errno);
        print_errno(errno, QString("open %1 failed").arg(path));
        return;
    }

    n = read(fd, buf.data(), bsiz - 1);
    close(fd);

    if (n < 0) {
        qCWarning(app) << "Failed to read schedstat file for process" << d->pid << "Error:" << strerror(errno);
        print_errno(errno, QString("read %1 failed").arg(path));
        return;
    }

    buf.data()[n] = '\0';
    rc = sscanf(buf.data(), "%*u %llu %*d", &wtime);
    if (rc == 1) {
        d->wtime = wtime * HZ / 1000000000;
        qCDebug(app) << "Successfully parsed schedstat for pid" << d->pid;
    } else {
        qCWarning(app) << "Failed to parse schedstat file for process" << d->pid;
    }
    qCDebug(app) << "Finished reading schedstat for pid" << d->pid;
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
    if(access(path, R_OK) != 0) {
        qCWarning(app) << "Cannot access status file for process" << d->pid;
        return !ok;
    }

    errno = 0;
    uFile fp(fopen(path, "r"));
    // open /proc/[pid]/status
    if (!fp) {
        qCWarning(app) << "Failed to open status file for process" << d->pid << "Error:" << strerror(errno);
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
    }

    if (ferror(fp.get())) {
        qCWarning(app) << "Failed to read status file for process" << d->pid << "Error:" << strerror(errno);
        print_errno(errno, QString("read %1 failed").arg(path));
        return !ok;
    }

    qCDebug(app) << "Successfully read status for pid" << d->pid;
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

    sprintf(path, PROC_STATM_PATH, d->pid);
    if(access(path, R_OK) != 0) {
        qCWarning(app) << "Cannot access statm file for process" << d->pid;
        return !ok;
    }

    errno = 0;
    // open /proc/[pid]/statm
    if ((fd = open(path, O_RDONLY)) < 0) {
        qCWarning(app) << "Failed to open statm file for process" << d->pid << "Error:" << strerror(errno);
        print_errno(errno, QString("open %1 failed").arg(path));
        return !ok;
    }

    nr = read(fd, buf, bsiz);
    close(fd);
    if (nr < 0) {
        qCWarning(app) << "Failed to read statm file for process" << d->pid << "Error:" << strerror(errno);
        print_errno(errno, QString("read %1 failed").arg(path));
        return !ok;
    }

    buf[nr] = '\0';
    // get resident set size & resident shared size in pages
    nr = sscanf(buf, "%llu %llu %llu", &d->vmsize, &d->rss, &d->shm);
    if (nr != 3) {
        d->vmsize = 0;
        d->rss = 0;
        d->shm = 0;
        qCWarning(app) << "Failed to parse statm file for process" << d->pid;
        print_errno(errno, QString("read %1 failed").arg(path));
    } else {
        // convert to kB
        d->vmsize <<= kb_shift;
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
    if(access(path, R_OK) != 0) {
        // qCWarning(app) << "Cannot access IO file for process" << d->pid;
        return;
    }

    errno = 0;
    // open /proc/[pid]/io
    uFile fp(fopen(path, "r"));
    if (!fp) {
        qCWarning(app) << "Failed to open IO file for process" << d->pid << "Error:" << strerror(errno);
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
    }
    if (ferror(fp.get())) {
        qCWarning(app) << "Failed to read IO file for process" << d->pid << "Error:" << strerror(errno);
        print_errno(errno, QString("read %1 failed").arg(path));
    }

    qCDebug(app) << "Finished reading IO for pid" << d->pid;
}

// read /proc/[pid]/fd
void Process::readSockInodes()
{
    struct dirent *dp;
    char path[128], fdp[256 + 32];
    struct stat sbuf;

    sprintf(path, PROC_FD_PATH, d->pid);
    if(access(path, R_OK) != 0) {
        // qCWarning(app) << "Cannot access fd directory for process" << d->pid;
        return;
    }

    errno = 0;
    // open /proc/[pid]/fd dir
    uDir dir(opendir(path));
    if (!dir) {
        qCWarning(app) << "Failed to open fd directory for process" << d->pid << "Error:" << strerror(errno);
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
                    // not append repeat data, may memory leak.
                    if (!d->sockInodes.contains(sbuf.st_ino)) {
                        d->sockInodes << sbuf.st_ino;
                    }
                }
            } // ::if(stat)
        } // ::if(isdigit)
    } // ::while(readdir)
    // if (errno) {
    //     print_errno(errno, QString("read %1 failed").arg(path));
    // }
}

bool Process::isValid() const
{
    return d && d->isValid();
}

pid_t Process::ppid() const
{
    return d->ppid;
}

pid_t Process::pid() const
{
    return d->pid;
}

qulonglong Process::utime() const
{
    return d->utime;
}

qulonglong Process::stime() const
{
    return d->stime;
}

QString Process::name() const
{
    return d->name;
}

void Process::setName(const QString &name)
{
    // qCDebug(app) << "Set name for pid" << d->pid << "to" << name;
    d->name = name;
}

QString Process::displayName() const
{
    return d->proc_name.displayName();
}

void Process::calculateProcessMetrics()
{
    CPUSet *cpuset = DeviceDB::instance()->cpuSet();
    ProcessSet *procset =  ProcessDB::instance()->processSet();

    auto recentProcptr = procset->getRecentProcStage(d->pid);
    auto validrecentPtr = recentProcptr.lock();
    qreal timedelta = d->stime + d->utime;
    if (validrecentPtr) {
        qCDebug(app) << "Found recent process stage for pid" << d->pid;
        qreal previousTime = validrecentPtr->ptime;
        timedelta = timedelta - previousTime;
        
        // Check for abnormal time delta that might indicate data corruption
        // If current time is much smaller than previous time, it suggests the current data was corrected
        if (timedelta < -1000) { // If delta is very negative (more than 1000 jiffies)
            qCInfo(app) << "Detected corrected CPU time for PID" << d->pid 
                       << "- current:" << (d->stime + d->utime) << "previous:" << previousTime
                       << "delta:" << timedelta << ". Using current time as baseline.";
            // Use current time as the baseline, effectively treating this as a new process
            timedelta = d->stime + d->utime;
        }
        
        struct DiskIO io = {validrecentPtr->read_bytes, validrecentPtr->write_bytes, validrecentPtr->cancelled_write_bytes};
        d->diskIOSample->addSample(new DISKIOSampleFrame(validrecentPtr->uptime, io));

        d->networkIOSample->addSample(new IOSampleFrame(validrecentPtr->uptime, {0, 0}));
    }
    d->cpuUsageSample->addSample(new CPUUsageSampleFrame(qMax(0., timedelta) / cpuset->getUsageTotalDelta() * 100));

    struct DiskIO io = {d->read_bytes, d->write_bytes, d->cancelled_write_bytes};
    d->diskIOSample->addSample(new DISKIOSampleFrame(d->uptime, io));

    auto pair = d->diskIOSample->recentSamplePair();
    struct IOPS iops = DISKIOSampleFrame::diskiops(pair.first, pair.second);
    d->diskIOSpeedSample->addSample(new IOPSSampleFrame(iops));

    qulonglong sum_recv = 0;
    qulonglong sum_send = 0;

    for (int i = 0; i < d->sockInodes.size(); ++i) {
        SockIOStat sockIOStat;
        bool result = NetifMonitor::instance()->getSockIOStatByInode(d->sockInodes[i], sockIOStat);
        if (result) {
            sum_recv += sockIOStat->rx_bytes;
            sum_send += sockIOStat->tx_bytes;
        }
    }
    d->networkIOSample->addSample(new IOSampleFrame(d->uptime, {sum_recv, sum_send}));

    auto netpair = d->networkIOSample->recentSamplePair();
    struct IOPS netiops = IOSampleFrame::iops(netpair.first, netpair.second);
    d->networkBandwidthSample->addSample(new IOPSSampleFrame(netiops));
}

QIcon Process::icon() const
{
    return d->proc_icon.icon();
}

qreal Process::cpu() const
{
    auto *sample = d->cpuUsageSample->recentSample();
    if (sample)
        return sample->data;
    else
        return {};
}

void Process::setCpu(qreal cpu)
{
    d->cpuUsageSample->addSample(new CPUUsageSampleFrame(cpu));
}

qulonglong Process::memory() const
{
    return d->rss - d->shm;
}

qulonglong Process::vtrmemory() const
{
    return d->vmsize;
}

qulonglong Process::sharememory() const
{
    return d->shm;
}

int Process::priority() const
{
    return d->nice;
}

void Process::setPriority(int prio)
{
    d->nice = prio;
}

char Process::state() const
{
    return d->state;
}

void Process::setState(char state)
{
    d->state = state;
}

QByteArrayList Process::cmdline() const
{
    return d->cmdline;
}

QString Process::cmdlineString() const
{
    return QUrl::fromPercentEncoding(d->cmdline.join(' '));
}

QHash<QString, QString> Process::environ() const
{
    return d->environ;
}

uid_t Process::uid() const
{
    return d->uid;
}

QString Process::userName() const
{
    return d->usrerName;
}

gid_t Process::gid() const
{
    return d->gid;
}

QString Process::groupName() const
{
    return SysInfo::groupName(d->gid);
}

qreal Process::readBps() const
{
    auto *sample = d->diskIOSpeedSample->recentSample();
    if (sample)
        return sample->data.inBps;
    else
        return {};
}

qreal Process::writeBps() const
{
    auto *sample = d->diskIOSpeedSample->recentSample();
    if (sample)
        return sample->data.outBps;
    else
        return {};
}

qulonglong Process::readBytes() const
{
    return d->read_bytes;
}

qulonglong Process::writeBytes() const
{
    return d->write_bytes;
}

qulonglong Process::cancelledWriteBytes() const
{
    return d->cancelled_write_bytes;
}

qreal Process::recvBps() const
{
    auto *sample = d->networkBandwidthSample->recentSample();
    if (sample)
        return sample->data.inBps;
    else
        return 0;
}

qreal Process::sentBps() const
{
    auto *sample = d->networkBandwidthSample->recentSample();
    if (sample)
        return sample->data.outBps;
    else
        return 0;
}

void Process::setNetIoBps(qreal recvBps, qreal sendBps)
{
    struct IOPS netIo = {recvBps, sendBps};
    d->networkBandwidthSample->addSample(new IOPSSampleFrame(netIo));
}

qulonglong Process::recvBytes() const
{
    auto *sample = d->networkIOSample->recentSample();
    if (sample)
        return sample->data.inBytes;
    else
        return 0;
}

qulonglong Process::sentBytes() const
{
    auto *sample = d->networkIOSample->recentSample();
    if (sample)
        return sample->data.outBytes;
    else
        return 0;
}

int Process::appType() const
{
    return d->apptype;
}

void Process::setAppType(int type)
{
    d->apptype = type;
}

// DKapture update methods removed - now handled by system service

void Process::setUptime(const timeval &uptime)
{
    d->uptime = uptime;
}

void Process::refreashProcessName()
{
    d->proc_name.refreashProcessName(this);
}

void Process::refreashProcessIcon()
{
    d->proc_icon.refreashProcessIcon(this);
}

void Process::setUserName(const QString &userName)
{
    d->usrerName = userName;
}

void Process::applyDKaptureData(const QVariantMap &data)
{
    // 设置基本状态信息
    if (data.contains("state")) {
        setState(data["state"].toString().at(0).toLatin1());
    }
    if (data.contains("priority")) {
        setPriority(data["priority"].toInt());
    }
    if (data.contains("comm")) {
        setName(data["comm"].toString());
    }

    // 设置 CPU 时间相关数据
    // 后端已经处理了DKapture累积数据的增量计算，这里接收的是增量值
    if (data.contains("utime")) {
        d->utime = data["utime"].toULongLong();
    }
    if (data.contains("stime")) {
        d->stime = data["stime"].toULongLong();
    }
    if (data.contains("cutime")) {
        d->cutime = data["cutime"].toULongLong();
    }
    if (data.contains("cstime")) {
        d->cstime = data["cstime"].toULongLong();
    }

    // 设置内存数据（DKapture返回的是原始格式，需要按照传统方式转换）
    // DKapture STAT中的rss和vsize是字节数（与/proc/[pid]/stat一致）
    if (data.contains("rss")) {
        d->rss = data["rss"].toULongLong() >> 10; // 字节转KB
    }
    if (data.contains("vsize")) {
        d->vmsize = data["vsize"].toULongLong() >> 10; // 字节转KB  
    }

    // DKapture STATM中的数据是页数（与/proc/[pid]/statm一致）
    if (data.contains("memory_resident")) {
        qulonglong residentPages = data["memory_resident"].toULongLong();
        d->rss = residentPages << kb_shift; // 页数转KB，与readStatm()一致
    }
    if (data.contains("memory_size")) {
        qulonglong memorySizePages = data["memory_size"].toULongLong();
        d->vmsize = memorySizePages << kb_shift; // 页数转KB，与readStatm()一致
    }
    if (data.contains("memory_shared")) {
        qulonglong sharedPages = data["memory_shared"].toULongLong();
        d->shm = sharedPages << kb_shift; // 页数转KB，与readStatm()一致
    }
    int pid = 0;
    if (data.contains("pid")) {
        pid = data["pid"].toInt();
    }

    // 设置其他基本信息
    if (data.contains("ppid")) {
        d->ppid = data["ppid"].toInt();
    }
    if (data.contains("num_threads")) {
        d->nthreads = data["num_threads"].toInt();
    }
    if (data.contains("nice")) {
        d->nice = data["nice"].toInt();
    }
    if (data.contains("start_time")) {
        d->start_time = data["start_time"].toULongLong();
    }

    // 设置 I/O 相关数据
    if (data.contains("read_bytes")) {
        d->read_bytes = data["read_bytes"].toULongLong();
    }
    if (data.contains("write_bytes")) {
        d->write_bytes = data["write_bytes"].toULongLong();
    }
    if (data.contains("cancelled_write_bytes")) {
        d->cancelled_write_bytes = data["cancelled_write_bytes"].toULongLong();
    }

    // 设置 STATUS 相关数据 - DKapture提供的UID/GID等信息
    if (data.contains("uid")) {
        d->uid = data["uid"].toUInt();
    }
    if (data.contains("gid")) {
        d->gid = data["gid"].toUInt();
    }
    if (data.contains("euid")) {
        d->euid = data["euid"].toUInt();
    }
    if (data.contains("egid")) {
        d->egid = data["egid"].toUInt();
    }

    // 设置 SCHEDSTAT 相关数据 - DKapture提供的调度统计信息
    // 只处理wtime字段，保持与原有实现的兼容性
    if (data.contains("rq_wait_time")) {
        // DKapture提供纳秒单位数据，需要转换为时钟滴答数（与传统方式一致）
        qulonglong rq_wait_time_ns = data["rq_wait_time"].toULongLong();
        d->wtime = rq_wait_time_ns * HZ / 1000000000;  // 纳秒转时钟滴答数
        qCDebug(app) << "✅ Applied SCHEDSTAT data for PID" << d->pid 
                    << "- rq_wait_time_ns:" << rq_wait_time_ns << "-> wtime:" << d->wtime;
    }

    // 网络流量数据改为使用传统方式获取 - 避免DKapture系统级vs用户态数据差异
    // 后端已禁用网络数据发送，前端使用传统socket inode方式获取网络流量
    // if (data.contains("network_rx_bytes")) {
    //     network_rx = data["network_rx_bytes"].toULongLong();
    // }
    // if (data.contains("network_tx_bytes")) {
    //     network_tx = data["network_tx_bytes"].toULongLong();
    // }
    
    // 标记进程为有效，但需要检查关键数据读取是否成功
    d->valid = true;
    bool ok = true;

    // 读取关键信息并检查成功性
    ok = ok && readCmdline();    // cmdline是必需的，失败则进程无效
    readEnviron();             // environ失败可以容忍
    readSockInodes();          // sockInodes失败可以容忍

    // 只有关键操作都成功才保持进程有效
    d->valid = d->valid && ok;
    
    d->usrerName = SysInfo::userName(d->uid);
    d->proc_name.refreashProcessName(this);
    d->proc_icon.refreashProcessIcon(this);
    d->uptime = SysInfo::instance()->uptime();

    // 更新应用类型，供过滤使用
    d->apptype = kNoFilter;
    const QVariant &euid = ProcessDB::instance()->processEuid();
    WMWindowList *wmwindowList = ProcessDB::instance()->windowList();

    if (euid == d->uid && (wmwindowList->isGuiApp(d->pid)
                           || wmwindowList->isTrayApp(d->pid)
                           || wmwindowList->isDesktopEntryApp(d->pid))) {
        qCDebug(app) << "Process" << d->pid << "is a GUI/Tray/Desktop app";
        d->apptype = kFilterApps;
    } else if (euid == d->uid) {
        qCDebug(app) << "Process" << d->pid << "is a current user app";
        d->apptype = kFilterCurrentUser;
    }

    calculateProcessMetrics();

    // qCInfo(app) << "Applied DKapture data to process" << pid() 
    //             << "- rss:" << (d->rss / 1024) << "KB"
    //             << "- vmsize:" << (d->vmsize / 1024) << "KB"
    //             << "- cpu_time:" << (d->utime + d->stime);
}

} // namespace process
} // namespace core
