// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process_set.h"
#include "process/process_db.h"
#include "common/common.h"
#include "wm/wm_window_list.h"
// #include "settings.h"

#include <QDebug>
#include <QSet>

#include <errno.h>

#define PROC_PATH "/proc"

using namespace common::error;

namespace core {
namespace process {

ProcessSet::ProcessSet()
    : m_set {}
    , m_recentProcStage {}
    , m_pidCtoPMapping {}
    , m_pidPtoCMapping {}
{
}

ProcessSet::ProcessSet(const ProcessSet &other)
    : m_set(other.m_set)
    , m_recentProcStage(other.m_recentProcStage)
    , m_pidCtoPMapping(other.m_pidCtoPMapping)
    , m_pidPtoCMapping(other.m_pidPtoCMapping)
{
    m_prePid.clear();
    m_curPid.clear();
    m_pidMyApps.clear();
    m_simpleSet.clear();
    // m_settings = Settings::instance();
}

void ProcessSet::mergeSubProcResources(pid_t ppid, qreal &cpu,
                                        qreal &recvBps, qreal &sendBps) const
{
    QList<pid_t> pending {ppid};
    QSet<pid_t> visited;
    while (!pending.isEmpty()) {
        const pid_t pid = pending.takeLast();
        if (visited.contains(pid))
            continue;
        visited.insert(pid);

        const auto proc = m_set.constFind(pid);
        if (proc == m_set.cend())
            continue;
        cpu += proc->cpu();
        recvBps += proc->recvBps();
        sendBps += proc->sentBps();

        auto child = m_pidPtoCMapping.constFind(pid);
        while (child != m_pidPtoCMapping.cend() && child.key() == pid) {
            pending.append(child.value());
            ++child;
        }
    }
}

QMap<pid_t, QList<pid_t>> ProcessSet::collapseWineContainerGroups(WMWindowList *windowList,
                                                                  uid_t euid)
{
    using WineIdentity = QPair<QString, QString>;

    const auto wineIdentity = [](const Process &proc, WineIdentity &identity) {
        const QHash<QString, QString> environ = proc.environ();
        const QString prefix = environ.value("WINEPREFIX").trimmed();
        const QString package = environ.value("DEB_PACKAGE_NAME").trimmed();
        if (prefix.isEmpty() || package.isEmpty())
            return false;

        identity = qMakePair(prefix, package);
        return true;
    };
    const auto isWineContainerProcess = [](const Process &proc) {
        const QString name = proc.name();
        if (name == QStringLiteral("wineserver")
                || name.startsWith(QStringLiteral("deepin-wine"))
                || name.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive))
            return true;

        const QByteArrayList cmdline = proc.cmdline();
        return !cmdline.isEmpty()
                && QString::fromLocal8Bit(cmdline.first())
                           .contains(".exe", Qt::CaseInsensitive);
    };

    QMap<WineIdentity, QList<pid_t>> membersByContainer;
    for (auto it = m_set.cbegin(); it != m_set.cend(); ++it) {
        if (it.value().uid() != euid)
            continue;

        WineIdentity identity;
        if (wineIdentity(it.value(), identity) && isWineContainerProcess(it.value()))
            membersByContainer[identity].append(it.key());
    }

    QMap<pid_t, QList<pid_t>> groups;
    for (auto it = membersByContainer.cbegin(); it != membersByContainer.cend(); ++it) {
        // Appended in PID order while traversing m_set.
        const QList<pid_t> &members = it.value();

        QSet<pid_t> memberPids;
        QList<pid_t> appCandidates;
        int rootCount = 0;
        for (pid_t pid : members) {
            memberPids.insert(pid);
            if (m_set.constFind(pid)->appType() == kFilterApps)
                appCandidates.append(pid);
        }
        for (pid_t pid : members) {
            if (!memberPids.contains(m_set.constFind(pid)->ppid()))
                ++rootCount;
        }

        if (appCandidates.isEmpty() || (rootCount < 2 && appCandidates.size() < 2))
            continue;

        pid_t representativePid = -1;
        int representativeScore = -1;
        for (pid_t pid : appCandidates) {
            int score = 0;
            if (windowList->isGuiApp(pid))
                score = 3;
            else if (windowList->isDesktopEntryApp(pid))
                score = 2;
            else if (windowList->isTrayApp(pid))
                score = 1;

            if (score > representativeScore
                    || (score == representativeScore
                        && (representativePid < 0 || pid < representativePid))) {
                representativePid = pid;
                representativeScore = score;
            }
        }

        groups.insert(representativePid, members);
        // Reparent only the aggregation tree. Keep the real PPIDs for process
        // details and ancestor checks. Native descendants stay on their branches.
        for (pid_t pid : members) {
            m_pidPtoCMapping.remove(m_set.constFind(pid)->ppid(), pid);
            if (pid != representativePid)
                m_pidPtoCMapping.insert(representativePid, pid);
        }
        for (pid_t pid : appCandidates) {
            if (pid == representativePid)
                continue;

            Process &demoted = m_set.find(pid).value();
            demoted.detach();
            demoted.setAppType(kFilterCurrentUser);
            windowList->removeDesktopEntryApp(pid);
        }
    }

    return groups;
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
        procstage->recv_bytes = iter->recvBytes();
        procstage->sent_bytes = iter->sentBytes();
        procstage->uptime = iter->procuptime();
        m_recentProcStage[iter->pid()] = procstage;
    }
    m_curPid.clear();
    m_set.clear();
    m_pidPtoCMapping.clear();
    m_pidCtoPMapping.clear();
    WMWindowList *wmwindowList = ProcessDB::instance()->windowList();

    Iterator iter;
    QList<pid_t> appLst;
    while (iter.hasNext()) {
        Process proc = iter.next();

        if(!m_curPid.contains(proc.pid()))
            m_curPid.append(proc.pid());

    }

    if(m_prePid != m_curPid) {
        for (const pid_t &pid : m_prePid) {
            if(!m_curPid.contains(pid)){
                if(m_simpleSet.contains(pid))
                    m_simpleSet.remove(pid);
                //for each pid,only one process reflected.So "removeOne()"func replied.
                if(m_pidMyApps.contains(pid))
                    m_pidMyApps.removeOne(pid);
            }
        }

        for (const pid_t &pid : m_curPid) {
            if(!m_prePid.contains(pid)){ //add  new process pid
                Process proc(pid);
                proc.readProcessSimpleInfo();
                if(!m_simpleSet.contains(pid))
                     m_simpleSet.insert(proc.pid(), proc);

                if (proc.appType() == kFilterApps && !wmwindowList->isTrayApp(proc.pid())) {
                     m_pidMyApps << proc.pid();
                }
            }
        }
        m_prePid = m_curPid;
    }

    // const QVariant &vindex = m_settings->getOption(kSettingKeyProcessTabIndex, kFilterApps);
    // int index = vindex.toInt();

    for (const pid_t &pid : m_prePid) {
        Process proc = m_simpleSet[pid];
        // if( ((kFilterApps == index) && (proc.appType()<= kFilterApps)) ||
        //     ((kFilterCurrentUser == index) && (proc.appType() <= kFilterCurrentUser)) ||
        //         (kNoFilter == index))
                {
             proc.readProcessVariableInfo();  //
               if (!proc.isValid())
                     continue;

               m_set.insert(proc.pid(), proc);
               m_pidPtoCMapping.insert(proc.ppid(), proc.pid());
               m_pidCtoPMapping.insert(proc.pid(), proc.ppid());
        }
    }

    const QMap<pid_t, QList<pid_t>> wineGroups =
            collapseWineContainerGroups(wmwindowList, ProcessDB::instance()->processEuid());
    for (auto it = wineGroups.cbegin(); it != wineGroups.cend(); ++it) {
        if (!m_pidMyApps.contains(it.key()))
            m_pidMyApps.append(it.key());
    }

    // Follow real parent links iteratively; a racy scan can contain cycles.
    const auto anyRootIsGuiProc = [&](pid_t ppid) -> bool {
        QSet<pid_t> visited;
        while (!visited.contains(ppid)) {
            visited.insert(ppid);
            if (wmwindowList->isGuiApp(ppid))
                return true;
            const auto parent = m_pidCtoPMapping.constFind(ppid);
            if (parent == m_pidCtoPMapping.cend())
                break;
            ppid = parent.value();
        }
        return false;
    };

    for (const pid_t &pid : m_pidMyApps) {
        auto proc = m_set.find(pid);
        if (proc == m_set.end() || proc->appType() != kFilterApps)
            continue;

        qreal recvBps = 0;
        qreal sendBps = 0;
        qreal ptotalCpu = 0.;
        mergeSubProcResources(pid, ptotalCpu, recvBps, sendBps);
        proc->setNetIoBps(recvBps, sendBps);
        proc->setCpu(ptotalCpu);

        if (wineGroups.contains(pid))
            continue;

        if (!wmwindowList->isGuiApp(pid))
        {
            // only if no ancestor process is gui app we keep this process
            if (m_pidCtoPMapping.contains(pid) &&
                    anyRootIsGuiProc(m_pidCtoPMapping[pid])) {

                // when we start app with deepin-terminal, we should skip setting apptype as CurrentUser
                const Process parentProc = getProcessById(m_pidCtoPMapping[pid]);
                QString parentCmdLineString = parentProc.cmdlineString();
                if (parentCmdLineString == QString("/bin/bash")) {
                    continue;
                }

                m_set[pid].setAppType(kFilterCurrentUser);
                wmwindowList->removeDesktopEntryApp(pid);
            }
        }
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

        advance();

            return proc;
    }

    return Process();
}

void ProcessSet::Iterator::advance()
{
    while ((m_dirent = readdir(m_dir.get()))) {
        if (isdigit(m_dirent->d_name[0]))
        if(pid_t(atoi(m_dirent->d_name)) < 10)
                continue;
        else 
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
    // 当系统读取到的m_set为空时,通过keys()函数返回会造成段错误 原因是keys函数效率低下,会造成大量的内存拷贝
    // 替换方案是
    QList<pid_t> pidList {};
    pidList.clear();
    int size = m_set.size();
    QMap<pid_t, Process>::key_iterator iterBegin = m_set.keyBegin();
    for (;iterBegin != m_set.keyEnd(); ++iterBegin) {
        pid_t tmpKey = *iterBegin;
        pidList.append(tmpKey);
        if (size != m_set.size())
            break;
    }
    return pidList;
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
