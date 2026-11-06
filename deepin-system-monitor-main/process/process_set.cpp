// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process_set.h"
#include "process/process_db.h"
#include "common/common.h"
#include "wm/wm_window_list.h"
// #include "settings.h"

#include <QDebug>
#include <QFileInfo>

#include <cctype>
#include <errno.h>

#define PROC_PATH "/proc"

using namespace common::error;

namespace core {
namespace process {

// ============================================================================
// Wine process aggregation helpers
//
// Wine creates multiple Linux processes for a single Windows application.
// Several of these processes may each hold an X11 window, causing each to be
// marked as kFilterApps and appear as a separate row in the "Applications"
// view — even though they represent the same application.  The helpers below
// identify wine processes and compute a grouping key so that scanProcess() can
// keep only one entry visible and merge the resource usage of the rest.
// ============================================================================

// Check whether a process is a wine process.
//
// A process is considered a wine process when its command-line starts with a
// Windows drive-letter path (e.g. "c:\\path\\app.exe") or its environment
// contains the WINEPREFIX variable.
static bool isWineProcess(const Process &proc)
{
    const QByteArrayList &cmdline = proc.cmdline();
    if (cmdline.size() > 0 && cmdline[0].length() > 3
        && std::isalpha(static_cast<unsigned char>(cmdline[0].at(0)))
        && cmdline[0].at(1) == ':'
        && (cmdline[0].at(2) == '/' || cmdline[0].at(2) == '\\')) {
        return true;
    }
    if (proc.environ().contains("WINEPREFIX")) {
        return true;
    }
    return false;
}

// Compute a grouping key that identifies a wine application.
//
// The key is built from the .exe file name (extracted from the command-line
// when it is a Windows path, or from the normalised process name otherwise)
// combined with the WINEPREFIX environment variable when available, so that
// two different wine applications sharing the same .exe name but using
// different wine prefixes are not mistakenly merged.
static QString wineAppKey(const Process &proc)
{
    const QByteArrayList &cmdline = proc.cmdline();
    QString exeName;
    if (cmdline.size() > 0 && cmdline[0].length() > 3
        && std::isalpha(static_cast<unsigned char>(cmdline[0].at(0)))
        && cmdline[0].at(1) == ':'
        && (cmdline[0].at(2) == '/' || cmdline[0].at(2) == '\\')) {
        // Convert Windows back-slash separators to forward slashes so that
        // QFileInfo::fileName() works correctly on Linux.
        QString winPath = QString::fromLocal8Bit(cmdline[0]);
        winPath.replace('\\', '/');
        exeName = QFileInfo(winPath).fileName();
    } else {
        exeName = proc.name();
    }

    if (exeName.isEmpty())
        return {};

    QString winePrefix = proc.environ().value("WINEPREFIX");
    if (!winePrefix.isEmpty())
        return winePrefix + QLatin1Char('/') + exeName;
    return exeName;
}

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

void ProcessSet::mergeSubProcNetIO(pid_t ppid, qreal &recvBps, qreal &sendBps)
{
    auto it = m_pidPtoCMapping.find(ppid);
    while (it != m_pidPtoCMapping.end() && it.key() == ppid) {
        mergeSubProcNetIO(it.value(), recvBps, sendBps);
        ++it;
    }

    const Process &proc = m_set[ppid];
    recvBps += proc.recvBps();
    sendBps += proc.sentBps();
}

void ProcessSet::mergeSubProcCpu(pid_t ppid, qreal &cpu)
{
    auto it = m_pidPtoCMapping.find(ppid);
    while (it != m_pidPtoCMapping.end() && it.key() == ppid) {
        mergeSubProcCpu(it.value(), cpu);
        ++it;
    }

    const Process &proc = m_set[ppid];
    cpu += proc.cpu();
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
                m_prePid.removeAt(pid);  //remove disappear process pid
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

    std::function<bool(pid_t ppid)> anyRootIsGuiProc;
    // find if any ancestor processes is gui application
    anyRootIsGuiProc = [&](pid_t ppid) -> bool {
        bool b;
        b = wmwindowList->isGuiApp(ppid);
        if (!b && m_pidCtoPMapping.contains(ppid))
        {
            b = anyRootIsGuiProc(m_pidCtoPMapping[ppid]);
        }
        return b;
    };

    for (const pid_t &pid : m_pidMyApps) {
        qreal recvBps = 0;
        qreal sendBps = 0;
        mergeSubProcNetIO(pid, recvBps, sendBps);
        m_set[pid].setNetIoBps(recvBps, sendBps);

        qreal ptotalCpu = 0.;
        mergeSubProcCpu(pid, ptotalCpu);
        m_set[pid].setCpu(ptotalCpu);

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

    // ===== Wine application aggregation =====
    //
    // Wine runs a single Windows application through multiple Linux processes.
    // When more than one of those processes owns an X11 window they are each
    // classified as kFilterApps and appear as duplicate rows in the
    // "Applications" view.  This block groups wine processes that belong to
    // the same application, keeps a single representative visible and merges
    // the CPU / network-IO of the hidden processes into it so that the user
    // sees one entry whose resource usage reflects the whole application.
    {
        // Group every wine process still flagged as an application by key.
        QMap<QString, QList<pid_t>> wineAppGroups;
        for (const pid_t &pid : m_pidMyApps) {
            if (m_set[pid].appType() != kFilterApps)
                continue;
            if (!isWineProcess(m_set[pid]))
                continue;
            QString key = wineAppKey(m_set[pid]);
            if (key.isEmpty())
                continue;
            wineAppGroups[key].append(pid);
        }

        // Walk the parent chain to decide whether *pid* is a descendant of
        // *ancestorPid* using the child→parent map built earlier in the scan.
        auto isDescendant = [&](pid_t pid, pid_t ancestorPid) -> bool {
            pid_t cur = pid;
            while (m_pidCtoPMapping.contains(cur)) {
                cur = m_pidCtoPMapping[cur];
                if (cur == ancestorPid)
                    return true;
            }
            return false;
        };

        for (auto it = wineAppGroups.begin(); it != wineAppGroups.end(); ++it) {
            QList<pid_t> &group = it.value();
            if (group.size() <= 1)
                continue;

            // Choose the representative process: prefer one that owns a GUI
            // window, otherwise the one with the highest CPU usage.
            pid_t mainPid = group[0];
            for (int i = 1; i < group.size(); ++i) {
                pid_t pid = group[i];
                bool mainGui = wmwindowList->isGuiApp(mainPid);
                bool pidGui  = wmwindowList->isGuiApp(pid);
                if (pidGui && !mainGui) {
                    mainPid = pid;
                } else if (pidGui == mainGui
                           && m_set[pid].cpu() > m_set[mainPid].cpu()) {
                    mainPid = pid;
                }
            }

            // The representative's CPU / net-IO were already filled with its
            // own sub-tree by mergeSubProcCpu / mergeSubProcNetIO above.
            // Capture that baseline once: when a demoted process is an
            // *ancestor* of the representative, its merged value already
            // contains the representative's sub-tree, so adding it verbatim
            // would double-count the representative's CPU / net-IO.
            // Subtracting the baseline yields only the ancestor's exclusive
            // contribution (its own usage plus any branches that are not
            // under the representative), which is exactly what must be folded
            // into the representative.
            qreal mainBaseCpu  = m_set[mainPid].cpu();
            qreal mainBaseRecv = m_set[mainPid].recvBps();
            qreal mainBaseSent = m_set[mainPid].sentBps();

            // Demote every non-representative process.
            for (pid_t pid : group) {
                if (pid == mainPid)
                    continue;

                // If the process is a descendant of any other member of the
                // group its CPU / net-IO has already been merged into that
                // ancestor (either by mergeSubProcCpu / mergeSubProcNetIO or
                // by the manual merge below), so we only need to hide it.
                // Otherwise it is an independent (sibling) process, or an
                // ancestor of the representative, whose resource usage must
                // be folded into the representative.
                bool descendantOfGroup = false;
                for (pid_t otherPid : group) {
                    if (otherPid != pid && isDescendant(pid, otherPid)) {
                        descendantOfGroup = true;
                        break;
                    }
                }

                if (!descendantOfGroup) {
                    // When *pid* is an ancestor of the representative its
                    // merged value already contains the representative's
                    // sub-tree; add only the exclusive part to avoid
                    // double-counting the representative's CPU / net-IO.
                    // Otherwise the two sub-trees are disjoint and the full
                    // value is added.
                    bool pidIsAncestorOfMain = isDescendant(mainPid, pid);

                    qreal addCpu  = m_set[pid].cpu();
                    qreal addRecv = m_set[pid].recvBps();
                    qreal addSent = m_set[pid].sentBps();
                    if (pidIsAncestorOfMain) {
                        addCpu  = qMax(qreal(0), addCpu  - mainBaseCpu);
                        addRecv = qMax(qreal(0), addRecv - mainBaseRecv);
                        addSent = qMax(qreal(0), addSent - mainBaseSent);
                    }

                    m_set[mainPid].setCpu(m_set[mainPid].cpu() + addCpu);
                    m_set[mainPid].setNetIoBps(
                        m_set[mainPid].recvBps() + addRecv,
                        m_set[mainPid].sentBps() + addSent);
                }

                // NOTE: memory is intentionally not aggregated here.  The
                // Process API exposes no memory setter, and the existing
                // merge logic (mergeSubProcCpu / mergeSubProcNetIO) likewise
                // never merges memory for any application, so the
                // representative's memory column reflects only its own
                // process — consistent with the rest of the model.

                // Hide the duplicate from the "Applications" view.  The
                // process remains visible in the "All processes" /
                // "My processes" views.
                m_set[pid].setAppType(kFilterCurrentUser);
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
