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
#ifndef PROCESS_STAT_H
#define PROCESS_STAT_H

#include <sys/types.h>

#include <QString>
#include <QSharedPointer>
#include <QHash>

// from /proc presudo filesystem
struct proc_stat {
    char                    state;                  // process state

    char                    __unused__1[3];         // ##padding##

    pid_t                   pid;                    // pid
    pid_t                   ppid;                   // parent process
    pid_t                   pgid;                   // process group id
    unsigned int            mask;                   // mask
    uid_t                   uid;                    // real uid
    gid_t                   gid;                    // real gid
    uid_t                   euid;                   // effective uid
    gid_t                   egid;                   // effective gid
    uid_t                   suid;                   // saved uid
    gid_t                   sgid;                   // saved gid
    uid_t                   fuid;                   // filesystem uid
    gid_t                   fgid;                   // filesystem gid
    unsigned int            processor;              // cpu number
    unsigned int            rt_prio;                // real time priority
    unsigned int            policy;                 // scheduling policy
    unsigned int            nthreads;               // number of threads
    int                     nice;                   // process nice

    char                    __unused__2[4];         // ##padding##

    unsigned long long      utime;                  // user time
    unsigned long long      stime;                  // kernel time
    long long               cutime;                 // user time on waiting children
    long long               cstime;                 // kernel time on waiting children
    unsigned long long      start_time;             // start time since system boot in clock ticks
    unsigned long long      vmsize;                 // vm size in kB
    unsigned long long      rss;                    // resident set size in kB
    unsigned long long      shm;                    // resident shared size in kB
    unsigned long long      guest_time;             // guest time (virtual cpu time for guest os)
    long long               cguest_time;            // children guest time in clock ticks

    unsigned long long      wtime;                  // time spent waiting on a runqueue

    // io
    unsigned long long      read_bytes;             //
    unsigned long long      write_bytes;            //
    unsigned long long      cancelled_write_bytes;  //

    QString                 proc_name;              // exec name
    QByteArrayList          cmdline;                // process cmdline
    QHash<QString, QString> environ;

    QList<ino_t>            sockInodes;             // socket inodes opened by this process
};

struct stat_context {
    unsigned int    kb_shift;
    unsigned long   hz;
};

using ProcStat = QSharedPointer<struct proc_stat>;

using ProcIterateCallback = void (*)(ProcStat &stat, void *context);

class ProcessStat
{
public:
    static bool readProcStats(ProcIterateCallback callback, void *context);
    static void getStatContext(struct stat_context &ctx);

private:
    // read /proc/[pid]/stat
    static inline bool readStat(ProcStat &ps);
    // read /proc/[pid]/cmdline
    static inline bool readCmdline(ProcStat &ps);
    // read /proc/[pid]/environ
    static inline void readEnviron(ProcStat &ps);
    // read /proc/[pid]/schedstat
    static inline bool readSchedStat(ProcStat &ps, struct stat_context &ctx);
    // read /proc/[pid]/status
    static inline bool readStatus(ProcStat &ps);
    // read /proc/[pid]/statm
    static inline bool readStatm(ProcStat &ps, struct stat_context &ctx);
    // read /proc/[pid]/io
    static inline bool readIO(ProcStat &ps);
    // read /proc/[pid]/fd
    static inline bool readSockInodes(ProcStat &ps);

private:
    ProcessStat() = default;
};

#endif // PROCESS_STAT_H
