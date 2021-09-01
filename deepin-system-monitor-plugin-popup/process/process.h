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
#ifndef PROCESS_H
#define PROCESS_H

#include "system/sys_info.h"

#include <QByteArray>
#include <QString>
#include <QIcon>
#include <QSharedDataPointer>
#include <QUrl>

#include <sys/types.h>

using namespace core::system;

namespace core {
namespace process {

enum ProcessPriority {
    kInvalidPriority = INT_MAX,
    kVeryHighPriority = -20, // default veryhigh priority
    kHighPriority = -10, // default high priority
    kNormalPriority = 0, // normal priority
    kLowPriority = 10, // default low priority
    kVeryLowPriority = 19, // default verylow priority
    kCustomPriority
};

static const int kVeryHighPriorityMax = -20;
static const int kVeryHighPriorityMin = -11;
static const int kHighPriorityMax = -10;
static const int kHighPriorityMin = -1;
static const int kLowPriorityMax = 1;
static const int kLowPriorityMin = 10;
static const int kVeryLowPriorityMax = 11;
static const int kVeryLowPriorityMin = 19;

QString getPriorityName(int prio);
ProcessPriority getProcessPriorityStub(int prio);

/**
 * @brief The Process class
 */
class ProcessPrivate;
class Process
{
public:
    explicit Process();
    explicit Process(pid_t pid);
    Process(const Process &other);
    Process &operator=(const Process &rhs);
    ~Process();

    bool isValid() const;

    pid_t pid() const;
    pid_t ppid() const;

    int appType() const;
    void setAppType(int type);

    qulonglong utime() const;
    qulonglong stime() const;

    QString name() const;
    void setName(const QString &name);
    QString displayName() const;

    QIcon icon() const;

    qreal cpu() const;
    void setCpu(qreal cpu);

    qulonglong memory() const;
    qulonglong vtrmemory() const;
    qulonglong sharememory() const;

    int priority() const;
    void setPriority(int priority);

    char state() const;
    void setState(char state);

    QByteArrayList cmdline() const;
    QString cmdlineString() const;

    QHash<QString, QString> environ() const;

    time_t startTime() const;
    timeval procuptime() const;

    uid_t uid() const;
    QString userName() const;

    gid_t gid() const;
    QString groupName() const;

    qreal readBps() const;
    qreal writeBps() const;

    qulonglong readBytes() const;
    qulonglong writeBytes() const;
    qulonglong cancelledWriteBytes() const;

    qreal recvBps() const;
    qreal sentBps() const;
    void setNetIoBps(qreal recvBps, qreal sendBps);

    qulonglong recvBytes() const;
    qulonglong sentBytes() const;

    void readProcessInfo();

private:
    /**
     * @brief Read /proc/[pid]/stat
     * @return true: success; false: failure
     */
    bool readStat();
    /**
     * @brief Read /proc/[pid]/cmdline
     * @return true: success; false: failure
     */
    bool readCmdline();
    /**
     * @brief Read /proc/[pid]/environ
     */
    void readEnviron();
    /**
     * @brief Read /proc/[pid]/schedstat
     */
    void readSchedStat();
    /**
     * @brief Read /proc/[pid]/status
     * @return true: success; false: failure
     */
    bool readStatus();
    /**
     * @brief Read /proc/[pid]/statm
     * @return true: success; false: failure
     */
    bool readStatm();
    /**
     * @brief Read /proc/[pid]/io
     * @return true: success; false: failure
     */
    void readIO();
    /**
     * @brief Read /proc/[pid]/fd
     * @return true: success; false: failure
     */
    void readSockInodes();

private:
    QSharedDataPointer<ProcessPrivate> d;
};

} // namespace process
} // namespace core

#endif // PROCESS_H
