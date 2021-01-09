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
#include "process_set.h"

#include "common/common.h"

#include <QDebug>

#include <errno.h>

#define PROC_PATH "/proc"

using namespace common::error;

namespace core {
namespace process {

ProcessSet::ProcessSet()
    : m_set {}
{
}

ProcessSet::ProcessSet(const ProcessSet &other)
    : m_set(other.m_set)
{
}

bool ProcessSet::containsById(pid_t pid)
{
    return m_set.contains(pid);
}

void ProcessSet::refresh()
{
    scanProcess();
}

void ProcessSet::scanProcess()
{
    for (auto iter = m_set.begin(); iter != m_set.end(); iter++) {
        std::shared_ptr<RecentProcStage> procstage = std::make_shared<RecentProcStage>();
        procstage->ptime = iter->utime() + iter->stime();
        procstage->read_bytes = iter->readBytes();
        procstage->write_bytes = iter->writeBytes();
        procstage->cancelled_write_bytes = iter->cancelledWriteBytes();
        procstage->uptime = iter->procuptime();
        m_recentProcStage[iter->pid()] = procstage;
    }

    m_set.clear();

    Iterator iter;
    while (iter.hasNext()) {
        Process proc = iter.next();
        if (!proc.isValid())
            continue;

        m_set.insert(proc.pid(), proc);
    }

    m_recentProcStage.clear();
}

ProcessSet::Iterator::Iterator()
{
    errno = 0;
    auto *dp = opendir(PROC_PATH);
    if (!dp) {
        print_errno(errno, "open /proc failed");
        return;
    }
    m_dir.reset(dp);

    advance();
}

bool ProcessSet::Iterator::hasNext()
{
    return m_dirent && isdigit(m_dirent->d_name[0]);
}

Process ProcessSet::Iterator::next()
{
    if (m_dirent && isdigit(m_dirent->d_name[0])) {
        auto pid = pid_t(atoi(m_dirent->d_name));
        Process proc(pid);
        proc.readProcessInfo();

        advance();

        if (proc.isValid())
            return proc;
    }

    return Process();
}

void ProcessSet::Iterator::advance()
{
    while ((m_dirent = readdir(m_dir.get()))) {
        if (isdigit(m_dirent->d_name[0]))
            break;
    }
    if (!m_dirent && errno) {
        print_errno(errno, "read /proc failed");
    }
}

std::weak_ptr<RecentProcStage> ProcessSet::getRecentProcStage(pid_t pid) const
{
    return m_recentProcStage[pid];
}

const Process ProcessSet::getProcessById(pid_t pid) const
{
    return m_set[pid];
}

QList<pid_t> ProcessSet::getPIDList() const
{
    return m_set.keys();
}

void ProcessSet::removeProcess(pid_t pid)
{
    m_set.remove(pid);
}

void ProcessSet::updateProcessState(pid_t pid, char state)
{
    if (m_set.contains(pid))
        m_set[pid].setState(state);
}

void ProcessSet::updateProcessPriority(pid_t pid, int priority)
{
    if (m_set.contains(pid))
        m_set[pid].setPriority(priority);
}

} // namespace process
} // namespace core
