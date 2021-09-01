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
#ifndef PROCESS_SET_H
#define PROCESS_SET_H

#include "process.h"
#include "common/common.h"

#include <QMap>

#include <dirent.h>

using namespace common::alloc;

namespace core {
namespace process {

enum FilterType { kFilterApps,
                  kFilterCurrentUser,
                  kNoFilter
                };

struct RecentProcStage {
    qulonglong ptime = 0;
    qulonglong read_bytes = 0; // disk read bytes
    qulonglong write_bytes = 0; // disk write bytes
    qulonglong cancelled_write_bytes = 0;
    timeval uptime = {0, 0};
};

class ProcessSet
{
public:
    explicit ProcessSet();
    ProcessSet(const ProcessSet &other);
    ~ProcessSet() = default;

    const Process getProcessById(pid_t pid) const;
    QList<pid_t> getPIDList() const;
    void removeProcess(pid_t pid);
    void updateProcessState(pid_t pid, char state);
    void updateProcessPriority(pid_t pid, int priority);
    std::weak_ptr<RecentProcStage> getRecentProcStage(pid_t pid) const;

    void refresh();

private:
    void scanProcess();
    void mergeSubProcNetIO(pid_t ppid, qreal &recvBps, qreal &sendBps);
    void mergeSubProcCpu(pid_t ppid, qreal &cpu);

    class Iterator
    {
    public:
        Iterator();

        bool hasNext();
        Process next();

    private:
        void advance();

        uDir m_dir {};
        struct dirent *m_dirent {};
    };

private:
    QMap<pid_t, Process> m_set;
    QMap<pid_t, std::shared_ptr<RecentProcStage>> m_recentProcStage {};

    QMap<pid_t, pid_t> m_pidCtoPMapping {}; // child to parent pid mapping
    QMultiMap<pid_t, pid_t> m_pidPtoCMapping {}; // parent to child pid mapping

    friend class Iterator;
};

} // namespace process
} // namespace core

#endif // PROCESS_SET_H
