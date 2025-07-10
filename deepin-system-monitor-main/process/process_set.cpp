// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process_set.h"
#include "ddlog.h"
#include "process/process_db.h"
#include "common/common.h"
#include "wm/wm_window_list.h"
// #include "settings.h"

#include <QDebug>

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
    qCDebug(app) << "ProcessSet object created";
}

ProcessSet::ProcessSet(const ProcessSet &other)
    : m_set(other.m_set)
    , m_recentProcStage(other.m_recentProcStage)
    , m_pidCtoPMapping(other.m_pidCtoPMapping)
    , m_pidPtoCMapping(other.m_pidPtoCMapping)
{
    qCDebug(app) << "ProcessSet object copied";
    m_prePid.clear();
    m_curPid.clear();
    m_pidMyApps.clear();
    m_simpleSet.clear();
    // m_settings = Settings::instance();
}

void ProcessSet::mergeSubProcNetIO(pid_t ppid, qreal &recvBps, qreal &sendBps)
{
    qCDebug(app) << "Merging sub-process net IO for ppid" << ppid;
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
    qCDebug(app) << "Merging sub-process CPU for ppid" << ppid;
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
    qCDebug(app) << "Refreshing process set";
    scanProcess();
}

void ProcessSet::scanProcess()
{
    qCDebug(app) << "Scanning processes";
    for (auto iter = m_set.begin(); iter != m_set.end(); iter++) {
        qCDebug(app) << "Storing recent stage for pid" << iter->pid();
        std::shared_ptr<RecentProcStage> procstage = std::make_shared<RecentProcStage>();
        procstage->ptime = iter->utime() + iter->stime();
        procstage->read_bytes = iter->readBytes();
        procstage->write_bytes = iter->writeBytes();
        procstage->cancelled_write_bytes = iter->cancelledWriteBytes();
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

        if(!m_curPid.contains(proc.pid())) {
            qCDebug(app) << "Found process with pid" << proc.pid();
            m_curPid.append(proc.pid());
        }

    }

    if(m_prePid != m_curPid) {
        qCDebug(app) << "Process list changed";
        for (auto it = m_prePid.begin(); it != m_prePid.end();) {
            if (!m_curPid.contains(*it)) {
                // qCDebug(app) << "Removing obsolete pid" << *it;
                if (m_simpleSet.contains(*it))
                    m_simpleSet.remove(*it);
                if (m_pidMyApps.contains(*it))
                    m_pidMyApps.removeOne(*it);
                it = m_prePid.erase(it);
            } else {
                ++it;
            }
        }

        for (const pid_t &pid : m_curPid) {
            if(!m_prePid.contains(pid)){ //add  new process pid
                // qCDebug(app) << "Adding new pid" << pid;
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
            //  qCDebug(app) << "Reading variable info for pid" << pid;
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
            qCDebug(app) << "Recursively checking for GUI ancestor for ppid" << ppid;
            b = anyRootIsGuiProc(m_pidCtoPMapping[ppid]);
        }
        return b;
    };

    for (const pid_t &pid : m_pidMyApps) {
        // qCDebug(app) << "Merging stats for my app with pid" << pid;
        qreal recvBps = 0;
        qreal sendBps = 0;
        mergeSubProcNetIO(pid, recvBps, sendBps);
        m_set[pid].setNetIoBps(recvBps, sendBps);

        qreal ptotalCpu = 0.;
        mergeSubProcCpu(pid, ptotalCpu);
        m_set[pid].setCpu(ptotalCpu);

        if (!wmwindowList->isGuiApp(pid))
        {
            qCDebug(app) << "Process is not a GUI app, checking for GUI ancestor. Pid:" << pid;
            // only if no ancestor process is gui app we keep this process
            if (m_pidCtoPMapping.contains(pid) &&
                    anyRootIsGuiProc(m_pidCtoPMapping[pid])) {
                qCDebug(app) << "Found GUI ancestor for pid" << pid;

                // when we start app with deepin-terminal, we should skip setting apptype as CurrentUser
                const Process parentProc = getProcessById(m_pidCtoPMapping[pid]);
                QString parentCmdLineString = parentProc.cmdlineString();

                /* 通过窗管接口获取到玲珑版本浏览器，wid 对应的 pid
                 * 与玲珑浏览器本身的 pid 不一致，GuiApps 变量里
                 * 没有该浏览器进程，因此进入到了这段代码被覆写了 appType 变量。
                 * 而这段代码本身用于对“应用列表”内的浏览器进程进行去重，
                 * 下面的代码原本条件是 ==，用于处理另一个 Bug
                 * https://pms.uniontech.com/zentao/bug-view-82161.html
                 * 此处兼容沿用该代码，放宽一点判定条件，改为 contains
                 * 进行判定。玲珑应用也是通过 /bin/bash 启动的，因此需要
                 * 兼容。
                 */
                if (parentCmdLineString.contains(QString("/bin/bash"))) {
                  qCDebug(app) << "Parent process is bash, skipping app type change for pid" << pid;
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
    // qCDebug(app) << "ProcessSet::Iterator created";
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
        // qCDebug(app) << "Iterator returning next pid" << pid;
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
    // qCDebug(app) << "Iterator advanced to" << (m_dirent ? m_dirent->d_name : "end");
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
    qCDebug(app) << "Getting PID list";
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
    // qCDebug(app) << "Removing process with pid" << pid;
    m_set.remove(pid);
}

void ProcessSet::updateProcessState(pid_t pid, char state)
{
    qCDebug(app) << "Updating process state for pid" << pid << "to" << state;
    if (m_set.contains(pid))
        m_set[pid].setState(state);
}

void ProcessSet::updateProcessPriority(pid_t pid, int priority)
{
    qCDebug(app) << "Updating process priority for pid" << pid << "to" << priority;
    if (m_set.contains(pid))
        m_set[pid].setPriority(priority);
}

} // namespace process
} // namespace core
