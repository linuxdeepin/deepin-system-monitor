// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process.h"
#include "ddlog.h"
#include "process/private/process_p.h"
#include "system/device_db.h"
#include "process/process_db.h"
#include "system/sys_info.h"
#include "system/cpu_set.h"
//#include "system/netif_info_db.h"
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

Process::Process()
    : d(new ProcessPrivate())
{

}

Process::Process(pid_t pid)
    : d(new ProcessPrivate())
{
    d->pid = pid;
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
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    return monitor->sysInfo()->btime().tv_sec + time_t(d->start_time / HZ);
}

timeval Process::procuptime() const
{
    return d->uptime;
}

void Process::readProcessVariableInfo()
{
    readProcessInfo();
}

void Process::readProcessSimpleInfo(bool skipStatReading)
{
    d->valid = true;
    bool ok = true;

    ok = ok && readCmdline();      // 两种模式都需要cmdline

    // 根据skipStatReading决定是否跳过stat、statm和status读取
    if (!skipStatReading) {
        ok = ok && readStat();     // 传统模式读取stat
        readSchedStat();           // 传统模式读取schedstat
        ok = ok && readStatm();    // 传统模式读取statm
        ok = ok && readStatus();   // 传统模式读取status
    }

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
        d->apptype = kFilterApps;
    } else if (euid == d->uid) {
        d->apptype = kFilterCurrentUser;
    }

    d->valid = d->valid && ok;
}

void Process::readProcessInfo()
{
    // 传统模式：调用完整的readProcessSimpleInfo，不跳过stat读取
    readProcessSimpleInfo(false);
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

    buf.reserve(1025);

    errno = 0;
    sprintf(path, PROC_STAT_PATH, d->pid);
    // open /proc/[pid]/stat
    if ((fd = open(path, O_RDONLY)) < 0) {
        qCWarning(app) << "Failed to open" << path << ":" << strerror(errno);
        return !ok;
    }

    // read data
    sz = read(fd, buf.data(), 1024);
    close(fd);
    if (sz < 0) {
        qCWarning(app) << "Failed to read" << path << ":" << strerror(errno);
        return !ok;
    }
    buf.data()[sz] = '\0';

    // get process name between (...)
    begin = strchr(buf.data(), '(');
    begin += 1;

    pos = strrchr(buf.data(), ')');
    if (!pos) {
        qCWarning(app) << "Failed to find process name in stat file";
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
        qCWarning(app) << "Failed to parse stat file, got" << rc << "fields";
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
        qCWarning(app) << "Failed to open" << path << ":" << strerror(errno);
        return !ok;
    }

    nb = fread(cmd.data(), 1, bsiz - 1, fp.get());
    if (ferror(fp.get())) {
        qCWarning(app) << "Failed to read" << path << ":" << strerror(errno);
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

    sprintf(path, PROC_ENVIRON_PATH, d->pid);

    errno = 0;
    // open /proc/[pid]/environ
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        qCWarning(app) << "Failed to open" << path << ":" << strerror(errno);
        return;
    }

    while ((nb = read(fd, buf, sz))) {
        buf[nb] = '\0';
        sbuf.append(buf, int(nb));
    }
    close(fd);

    if (nb == 0 && errno != 0) {
        qCWarning(app) << "Failed to read" << path << ":" << strerror(errno);
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

    buf.reserve(bsiz);
    sprintf(path, PROC_SCHEDSTAT_PATH, d->pid);

    errno = 0;
    // open /proc/[pid]/schedstat
    if ((fd = open(path, O_RDONLY)) < 0) {
        qCWarning(app) << "Failed to open" << path << ":" << strerror(errno);
        return;
    }

    n = read(fd, buf.data(), bsiz - 1);
    close(fd);

    if (n < 0) {
        qCWarning(app) << "Failed to read" << path << ":" << strerror(errno);
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
        qCWarning(app) << "Failed to open" << path << ":" << strerror(errno);
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
        qCWarning(app) << "Failed to read" << path << ":" << strerror(errno);
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

    sprintf(path, PROC_STATM_PATH, d->pid);

    errno = 0;
    // open /proc/[pid]/statm
    if ((fd = open(path, O_RDONLY)) < 0) {
        qCWarning(app) << "Failed to open" << path << ":" << strerror(errno);
        return !ok;
    }

    nr = read(fd, buf, bsiz);
    close(fd);
    if (nr < 0) {
        qCWarning(app) << "Failed to read" << path << ":" << strerror(errno);
        return !ok;
    }

    buf[nr] = '\0';
    // get resident set size & resident shared size in pages
    nr = sscanf(buf, "%llu %llu %llu", &d->vmsize, &d->rss, &d->shm);
    if (nr != 3) {
        d->vmsize = 0;
        d->rss = 0;
        d->shm = 0;
        qCWarning(app) << "Failed to parse statm file for PID" << pid();
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

    errno = 0;
    // open /proc/[pid]/io
    uFile fp(fopen(path, "r"));
    if (!fp) {
        qCWarning(app) << "Failed to open" << path << ":" << strerror(errno);
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
        qCWarning(app) << "Failed to read" << path << ":" << strerror(errno);
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
        qCWarning(app) << "Failed to open" << path << ":" << strerror(errno);
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
                    if (!d->sockInodes.contains(sbuf.st_ino)) {
                        d->sockInodes << sbuf.st_ino;
                    }
                }
            } // ::if(stat)
        } // ::if(isdigit)
    } // ::while(readdir)
    if (errno) {
        qCWarning(app) << "Failed to read" << path << ":" << strerror(errno);
    }
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
    d->name = name;
}

QString Process::displayName() const
{
    return d->proc_name.displayName();
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

void Process::applyDKaptureData(const QVariantMap &pidData)
{
    // 设置基本状态信息
    if (pidData.contains("state")) {
        setState(pidData["state"].toString().at(0).toLatin1());
    }
    if (pidData.contains("priority")) {
        setPriority(pidData["priority"].toInt());
    }
    if (pidData.contains("comm")) {
        setName(pidData["comm"].toString());
    }

    // 设置 CPU 时间相关数据
    if (pidData.contains("utime")) {
        d->utime = pidData["utime"].toULongLong();
    }
    if (pidData.contains("stime")) {
        d->stime = pidData["stime"].toULongLong();
    }
    if (pidData.contains("cutime")) {
        d->cutime = pidData["cutime"].toULongLong();
    }
    if (pidData.contains("cstime")) {
        d->cstime = pidData["cstime"].toULongLong();
    }

    // 设置内存相关数据（使用与main程序相同的转换逻辑）
    // DKapture STAT中的rss和vsize是字节数（与/proc/[pid]/stat一致）
    if (pidData.contains("rss")) {
        d->rss = pidData["rss"].toULongLong() >> 10; // 字节转KB
    }
    if (pidData.contains("vsize")) {
        d->vmsize = pidData["vsize"].toULongLong() >> 10; // 字节转KB  
    }

    // DKapture STATM中的数据是页数（与/proc/[pid]/statm一致）
    if (pidData.contains("memory_resident")) {
        qulonglong residentPages = pidData["memory_resident"].toULongLong();
        d->rss = residentPages << kb_shift; // 页数转KB，与readStatm()一致
    }
    if (pidData.contains("memory_size")) {
        qulonglong memorySizePages = pidData["memory_size"].toULongLong();
        d->vmsize = memorySizePages << kb_shift; // 页数转KB，与readStatm()一致
    }
    if (pidData.contains("memory_shared")) {
        qulonglong sharedPages = pidData["memory_shared"].toULongLong();
        d->shm = sharedPages << kb_shift; // 页数转KB，与readStatm()一致
    }

    // 设置其他基本信息
    if (pidData.contains("ppid")) {
        d->ppid = pidData["ppid"].toInt();
    }
    if (pidData.contains("num_threads")) {
        d->nthreads = pidData["num_threads"].toInt();
    }
    if (pidData.contains("nice")) {
        d->nice = pidData["nice"].toInt();
    }
    if (pidData.contains("start_time")) {
        d->start_time = pidData["start_time"].toULongLong();
    }

    // 设置 STATUS 相关数据 - DKapture提供的UID/GID等信息
    if (pidData.contains("uid")) {
        d->uid = pidData["uid"].toUInt();
    }
    if (pidData.contains("gid")) {
        d->gid = pidData["gid"].toUInt();
    }
    if (pidData.contains("euid")) {
        d->euid = pidData["euid"].toUInt();
    }
    if (pidData.contains("egid")) {
        d->egid = pidData["egid"].toUInt();
    }
    
    // 设置 I/O 相关数据
    if (pidData.contains("read_bytes")) {
        d->read_bytes = pidData["read_bytes"].toULongLong();
    }
    if (pidData.contains("write_bytes")) {
        d->write_bytes = pidData["write_bytes"].toULongLong();
    }
    if (pidData.contains("cancelled_write_bytes")) {
        d->cancelled_write_bytes = pidData["cancelled_write_bytes"].toULongLong();
    }

    // 设置 SCHEDSTAT 相关数据 - DKapture提供的调度统计信息
    // 只处理wtime字段，保持与原有实现的兼容性
    if (pidData.contains("rq_wait_time")) {
        // DKapture提供纳秒单位数据，需要转换为时钟滴答数（与传统方式一致）
        qulonglong rq_wait_time_ns = pidData["rq_wait_time"].toULongLong();
        d->wtime = rq_wait_time_ns * HZ / 1000000000;  // 纳秒转时钟滴答数
        qCDebug(app) << "✅ Applied SCHEDSTAT data for PID" << d->pid 
                    << "- rq_wait_time_ns:" << rq_wait_time_ns << "-> wtime:" << d->wtime;
    }
    
    // 标记进程为有效，但需要检查关键数据读取是否成功
    d->valid = true;
    bool ok = true;
    
    // 读取关键信息并检查成功性
    ok = ok && readCmdline();    // cmdline是必需的，失败则进程无效
    readEnviron();             // environ失败可以容忍
    // readSockInodes();        // plugin-popup 不需要网络流量计算
    
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
    // plugin-popup 是轻量级组件，网络数据由calculateProcessMetrics统一处理
    // 避免重复网络计算

    auto netpair = d->networkIOSample->recentSamplePair();
    struct IOPS netiops = IOSampleFrame::iops(netpair.first, netpair.second);
    d->networkBandwidthSample->addSample(new IOPSSampleFrame(netiops));
    
    // qCInfo(app) << "Applied DKapture data to process" << pid() 
    //             << "- rss:" << (d->rss / 1024) << "KB"
    //             << "- vmsize:" << (d->vmsize / 1024) << "KB"
    //             << "- cpu_time:" << (d->utime + d->stime);
}

} // namespace process
} // namespace core
