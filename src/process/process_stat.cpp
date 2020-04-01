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
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <QMap>
#include <QList>
#include <QDebug>

#include "process_stat.h"

#define PROC_PATH               "/proc"
#define PROC_STAT_PATH          "/proc/%u/stat"
#define PROC_STATUS_PATH        "/proc/%u/status"
#define PROC_STATM_PATH         "/proc/%u/statm"
#define PROC_CMDLINE_PATH       "/proc/%u/cmdline"
#define PROC_ENVIRON_PATH       "/proc/%u/environ"
#define PROC_IO_PATH            "/proc/%u/io"
#define PROC_FD_PATH            "/proc/%u/fd"
#define PROC_FD_NAME_PATH       "/proc/%u/fd/%s"
#define PROC_SCHEDSTAT_PATH     "/proc/%u/schedstat"

auto print_err = [](decltype(errno) e, const QString &msg)
{
    qDebug() << QString("Error: [%1] %2, ").arg(e).arg(strerror(e)) << msg;
};

// ref: sysstat#common.c#get_kb_shift
static inline unsigned int get_kb_shift(void)
{
    int shift = 0;
    long size;

    /* One can also use getpagesize() to get the size of a page */
    if ((size = sysconf(_SC_PAGESIZE)) == -1) {
        perror("sysconf");
    }

    size >>= 10;    /* Assume that a page has a minimum size of 1 kB */

    while (size > 1) {
        shift++;
        size >>= 1;
    }

    return uint(shift);
}

static inline unsigned long get_HZ(void)
{
    long ticks;

    if ((ticks = sysconf(_SC_CLK_TCK)) == -1) {
        perror("sysconf");
    }

    return ulong(ticks);
}

bool ProcessStat::readProcStats(ProcIterateCallback pfnCallback, void *context)
{
    bool b = false;
    DIR *dp;
    struct dirent *dir;
    struct stat_context ctx {};

    ProcessStat::getStatContext(ctx);

    errno = 0;
    dp = opendir(PROC_PATH);
    if (!dp) {
        print_err(errno, "open /proc failed");
        return b;
    }

    while ((dir = readdir(dp))) {
        if (isdigit(dir->d_name[0])) {
            auto ps = ProcStat(new proc_stat {});
            auto pid = pid_t(atoi(dir->d_name));
            ps->pid = pid;

            b = readStat(ps);
            b = b && readCmdline(ps);
            readEnviron(ps);
            b = b && readSchedStat(ps, ctx);
            b = b && readStatus(ps);
            b = b && readStatm(ps, ctx);
            readIO(ps);
            readSockInodes(ps);

            if (b && pfnCallback) {
                pfnCallback(ps, context);
            }
        }
    }
    if (errno) {
        print_err(errno, "read /proc failed");
        closedir(dp);
        return b;
    }
    closedir(dp);

    return b;
}

void ProcessStat::getStatContext(stat_context &ctx)
{
    ctx.kb_shift = get_kb_shift();
    ctx.hz = get_HZ();
}

bool ProcessStat::readStat(ProcStat &ps)
{
    bool b = false;
    char path[256];
    QScopedArrayPointer<char> buf(new char[1025] {});
    int fd, rc;
    ssize_t sz;
    char *pos, *begin;

    errno = 0;
    sprintf(path, PROC_STAT_PATH, ps->pid);
    if ((fd = open(path, O_RDONLY)) < 0) {
        print_err(errno, QString("open %1 failed").arg(path));
        return b;
    }

    sz = read(fd, buf.data(), 1024);
    if (sz < 0) {
        print_err(errno, QString("read %1 failed").arg(path));
        close(fd);
        return b;
    }
    buf.data()[sz] = '\0';
    close(fd);

    begin = strchr(buf.data(), '(');
    begin += 1;

    pos = strrchr(buf.data(), ')');
    if (!pos) {
        return b;
    }

    *pos = '\0';
    ps->proc_name = QString(begin);

    pos += 2;

    //****************3**4**5*******************************************14***15**
    rc = sscanf(pos, "%c %d %d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu"
                //*16***17******19*20******22************************************
                " %lld %lld %*d %d %u %*u %llu %*u %*u %*u %*u %*u %*u %*u %*u"
                //********************************39*40*41******43***44**********
                " %*u %*u %*u %*u %*u %*u %*u %*u %u %u %u %*u %llu %lld\n",
                &ps->state,           // 3
                &ps->ppid,            // 4
                &ps->pgid,            // 5
                &ps->utime,           // 14
                &ps->stime,           // 15
                &ps->cutime,          // 16
                &ps->cstime,          // 17
                &ps->nice,            // 19
                &ps->nthreads,        // 20
                &ps->start_time,      // 22
                &ps->processor,       // 39
                &ps->rt_prio,         // 40
                &ps->policy,          // 41
                &ps->guest_time,      // 43
                &ps->cguest_time);    // 44
    if (rc < 15) {
        return b;
    }
    if (rc < 17) {
        ps->guest_time = ps->cguest_time = 0;
    }

    b = true;

    return b;
}

bool ProcessStat::readCmdline(ProcStat &ps)
{
    bool b = false;
    FILE *fp;
    char path[128] {};
    const size_t bsiz = 4096;
    QScopedArrayPointer<char> cmd(new char[bsiz] {});
    size_t nb;
    char *begin, *cur, *end;

    sprintf(path, PROC_CMDLINE_PATH, ps->pid);

    errno = 0;
    if (!(fp = fopen(path, "r"))) {
        print_err(errno, QString("open %1 failed").arg(path));
        return b;
    }

    nb = fread(cmd.data(), 1, bsiz - 1, fp);
    if (ferror(fp)) {
        print_err(errno, QString("read %1 failed").arg(path));
        fclose(fp);
        return b;
    }
    fclose(fp);

    cmd.data()[nb] = '\0';

    begin = cur = cmd.data();
    end = cmd.data() + nb;
    while (cur < end) {
        if (*cur == '\0') {
            QByteArray buf(begin);
            ps->cmdline << buf;
            begin = cur + 1;
        }
        ++cur;
    }
    if (begin < end) {
        QByteArray buf(begin);
        ps->cmdline << buf;
    }
    b = true;

    return b;
}

void ProcessStat::readEnviron(ProcStat &ps)
{
    const size_t sz = 1024;
    char path[128] {};
    ssize_t nb;
    int fd;
    QByteArray sbuf {};
    char buf[sz + 1] {};

    sprintf(path, PROC_ENVIRON_PATH, ps->pid);

    errno = 0;
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        print_err(errno, QString("open %1 failed").arg(path));
        return;
    }

    while ((nb = read(fd, buf, sz))) {
        buf[nb] = '\0';
        sbuf.append(buf, int(nb));
    }
    if (nb == 0 && errno != 0) {
        print_err(errno, QString("read %1 failed").arg(path));
    }
    close(fd);

    if (sbuf.size() > 0) {
        auto elist = sbuf.split('\0');
        for (auto it : elist) {
            // it: name=value pair
            auto kvp = it.split('=');
            if (kvp.size() == 2) {
                ps->environ[kvp[0]] = kvp[1];
            }
        }
    }
}

bool ProcessStat::readSchedStat(ProcStat &ps, struct stat_context &ctx)
{
    bool b = false;
    const size_t bsiz = 1024;
    QScopedArrayPointer<char> buf(new char[bsiz] {});
    char path[128];
    int fd, rc;
    ssize_t n;
    unsigned long long wtime = 0;

    sprintf(path, PROC_SCHEDSTAT_PATH, ps->pid);

    errno = 0;
    if ((fd = open(path, O_RDONLY)) < 0) {
        print_err(errno, QString("open %1 failed").arg(path));
        return b;
    }

    n = read(fd, buf.data(), bsiz - 1);
    if (n < 0) {
        print_err(errno, QString("read %1 failed").arg(path));
        close(fd);
        return b;
    }
    close(fd);

    buf.data()[n] = '\0';
    rc = sscanf(buf.data(), "%*u %llu %*d", &wtime);
    if (rc == 1) {
        ps->wtime = wtime * ctx.hz / 1000000000;
        b = true;
    }

    return b;
}

bool ProcessStat::readStatus(ProcStat &ps)
{
    bool b = false;
    FILE *fp;
    const size_t bsiz = 256;
    QScopedArrayPointer<char> buf(new char[bsiz] {});
    char path[128];

    sprintf(path, PROC_STATUS_PATH, ps->pid);

    errno = 0;
    if (!(fp = fopen(path, "r)"))) {
        print_err(errno, QString("open %1 failed").arg(path));
        return b;
    }

    while (fgets(buf.data(), bsiz, fp)) {
        if (!strncmp(buf.data(), "Umask:", 6)) {
            sscanf(buf.data() + 7, "%u", &ps->mask);
        } else if (!strncmp(buf.data(), "State:", 6)) {
            sscanf(buf.data() + 7, "%c %*s", &ps->state);
        } else if (!strncmp(buf.data(), "Uid:", 4)) {
            sscanf(buf.data() + 5, "%u %u %u %u",
                   &ps->uid,
                   &ps->euid,
                   &ps->suid,
                   &ps->fuid);
        } else if (!strncmp(buf.data(), "Gid:", 4)) {
            sscanf(buf.data() + 5, "%u %u %u %u",
                   &ps->gid,
                   &ps->egid,
                   &ps->sgid,
                   &ps->fgid);
        }
    }
    if (ferror(fp)) {
        print_err(errno, QString("open %1 failed").arg(path));
    } else {
        b = true;
    }
    fclose(fp);

    return b;
}

bool ProcessStat::readStatm(ProcStat &ps, struct stat_context &ctx)
{
    bool b = false;
    int fd;
    const size_t bsiz = 1024;
    char path[128] {}, buf[bsiz + 1] {};
    ssize_t nr;

    sprintf(path, PROC_STATM_PATH, ps->pid);

    errno = 0;
    if ((fd = open(path, O_RDONLY)) < 0) {
        print_err(errno, QString("open %1 failed").arg(path));
        return b;
    }

    nr = read(fd, buf, bsiz);
    if (nr < 0) {
        print_err(errno, QString("read %1 failed").arg(path));
        close(fd);
        return b;
    }
    close(fd);

    buf[nr] = '\0';
    nr = sscanf(buf, "%*u %llu %llu", &ps->rss, &ps->shm);
    if (nr != 2) {
        ps->rss = 0;
        ps->shm = 0;
        print_err(errno, QString("read %1 failed").arg(path));
    } else {
        ps->rss <<= ctx.kb_shift;
        ps->shm <<= ctx.kb_shift;
        b = true;
    }
    return b;
}

bool ProcessStat::readIO(ProcStat &ps)
{
    bool b = false;
    FILE *fp;
    const size_t bsiz = 128;
    char path[128], buf[bsiz];

    sprintf(path, PROC_IO_PATH, ps->pid);

    errno = 0;
    if (!(fp = fopen(path, "r"))) {
        print_err(errno, QString("open %1 failed").arg(path));
        return b;
    }

    while (fgets(buf, bsiz, fp)) {
        if (!strncmp(buf, "read_bytes", 10)) {
            sscanf(buf + 12, "%llu", &ps->read_bytes);
        } else if (!strncmp(buf, "write_bytes", 11)) {
            sscanf(buf + 13, "%llu", &ps->write_bytes);
        } else if (!strncmp(buf, "cancelled_write_bytes", 21)) {
            sscanf(buf + 23, "%llu", &ps->cancelled_write_bytes);
        }
    }
    if (ferror(fp)) {
        print_err(errno, QString("read %1 failed").arg(path));
    } else {
        b = true;
    }
    fclose(fp);

    return b;
}

bool ProcessStat::readSockInodes(ProcStat &ps)
{
    bool b = false;
    DIR *dir;
    struct dirent *dp;
    char path[128], fdp[256 + 32];
    struct stat sbuf;

    sprintf(path, PROC_FD_PATH, ps->pid);

    errno = 0;
    if (!(dir = opendir(path))) {
        print_err(errno, QString("open %1 failed").arg(path));
        return b;
    }

    while ((dp = readdir(dir))) {
        if (isdigit(dp->d_name[0])) {
            sprintf(fdp, PROC_FD_NAME_PATH, ps->pid, dp->d_name);
            memset(&sbuf, 0, sizeof(struct stat));
            if (!stat(fdp, &sbuf)) {
                if (S_ISSOCK(sbuf.st_mode)) {
                    ps->sockInodes << sbuf.st_ino;
                }
            }
        }
    }
    if (errno) {
        print_err(errno, QString("read %1 failed").arg(path));
    } else {
        b = true;
    }
    closedir(dir);

    return b;
}
