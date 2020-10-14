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

#ifndef PROCESS_STAT_H
#define PROCESS_STAT_H

#include <QString>
#include <QSharedPointer>
#include <QHash>

#include <sys/types.h>

/**
 * @brief Process stat struct
 */
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
    unsigned long long read_bytes; // disk read bytes
    unsigned long long write_bytes; // disk write bytes
    unsigned long long cancelled_write_bytes; // cancelled write bytes

    QString                 proc_name;              // exec name
    QByteArrayList          cmdline;                // process cmdline
    QHash<QString, QString> environ; // environment cache

    QList<ino_t>            sockInodes;             // socket inodes opened by this process
};

// meta context
struct stat_context {
    unsigned int    kb_shift;
    unsigned long   hz;
};

using ProcStat = QSharedPointer<struct proc_stat>;

using ProcIterateCallback = void (*)(ProcStat &stat, void *context);

/**
 * @brief Process stat class
 */
class ProcessStat
{
public:
    /**
     * @brief Read process stats
     * @param callback User callback function
     * @param context Stat context
     * @return true: success; false: failure
     */
    static bool readProcStats(ProcIterateCallback callback, void *context);
    /**
     * @brief Get process stat context
     * @param ctx Stat context
     */
    static void getStatContext(struct stat_context &ctx);

private:
    /**
     * @brief Read /proc/[pid]/stat
     * @param ps Stat info
     * @return true: success; false: failure
     */
    static inline bool readStat(ProcStat &ps);
    /**
     * @brief Read /proc/[pid]/cmdline
     * @param ps Stat info
     * @return true: success; false: failure
     */
    static inline bool readCmdline(ProcStat &ps);
    /**
     * @brief Read /proc/[pid]/environ
     * @param ps Stat info
     */
    static inline void readEnviron(ProcStat &ps);
    /**
     * @brief Read /proc/[pid]/schedstat
     * @param ps Stat info
     * @param ctx Stat context
     */
    static inline void readSchedStat(ProcStat &ps, struct stat_context &ctx);
    /**
     * @brief Read /proc/[pid]/status
     * @param ps Stat info
     * @return true: success; false: failure
     */
    static inline bool readStatus(ProcStat &ps);
    /**
     * @brief Read /proc/[pid]/statm
     * @param ps Stat info
     * @param ctx Stat context
     * @return true: success; false: failure
     */
    static inline bool readStatm(ProcStat &ps, struct stat_context &ctx);
    /**
     * @brief Read /proc/[pid]/io
     * @param ps Stat info
     * @return true: success; false: failure
     */
    static inline bool readIO(ProcStat &ps);
    /**
     * @brief Read /proc/[pid]/fd
     * @param ps Stat info
     * @return true: success; false: failure
     */
    static inline bool readSockInodes(ProcStat &ps);

private:
    // default constructor
    ProcessStat() = default;
};

#endif // PROCESS_STAT_H
