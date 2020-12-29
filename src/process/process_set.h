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

enum FilterType { kNoFilter,
                  kFilterApps,
                  kFilterCurrentUser };

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

    void refresh();

private:
    void scanProcess();

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

    friend class Iterator;
};

inline const Process ProcessSet::getProcessById(pid_t pid) const
{
    return m_set[pid];
}

inline QList<pid_t> ProcessSet::getPIDList() const
{
    return m_set.keys();
}

inline void ProcessSet::removeProcess(pid_t pid)
{
    m_set.remove(pid);
}

inline void ProcessSet::updateProcessState(pid_t pid, char state)
{
    if (m_set.contains(pid))
        m_set[pid].setState(state);
}

inline void ProcessSet::updateProcessPriority(pid_t pid, int priority)
{
    if (m_set.contains(pid))
        m_set[pid].setPriority(priority);
}

} // namespace process
} // namespace core

#endif // PROCESS_SET_H
