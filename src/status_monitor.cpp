/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#include "constant.h"
#include "process_item.h"
#include "process_tree.h"
#include "status_monitor.h"
#include "utils.h"
#include <QDebug>
#include <QPainter>
#include <proc/sysinfo.h>
#include <thread>
#include <unistd.h>

using namespace Utils;

StatusMonitor::StatusMonitor(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(Constant::STATUS_BAR_WIDTH);

    filterType = OnlyGUI;
    tabName = "应用程序";

    layout = new QVBoxLayout(this);

    cpuMonitor = new CpuMonitor();
    memoryMonitor = new MemoryMonitor();
    networkMonitor = new NetworkMonitor();

    layout->addWidget(cpuMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(memoryMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(networkMonitor, 0, Qt::AlignHCenter);

    totalSentBytes = 0;
    totalRecvBytes = 0;
    totalSentKbs = 0;
    totalRecvKbs = 0;

    findWindowTitle = new FindWindowTitle();

    processSentBytes = new QMap<int, uint32_t>();
    processRecvBytes = new QMap<int, uint32_t>();

    processWriteKbs = new QMap<int, unsigned long>();
    processReadKbs = new QMap<int, unsigned long>();

    connect(this, &StatusMonitor::updateMemoryStatus, memoryMonitor, &MemoryMonitor::updateStatus, Qt::QueuedConnection);
    connect(this, &StatusMonitor::updateCpuStatus, cpuMonitor, &CpuMonitor::updateStatus, Qt::QueuedConnection);
    connect(this, &StatusMonitor::updateNetworkStatus, networkMonitor, &NetworkMonitor::updateStatus, Qt::QueuedConnection);

    updateStatusTimer = new QTimer();
    connect(updateStatusTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    updateStatusTimer->start(updateDuration);
}

StatusMonitor::~StatusMonitor()
{
    delete cpuMonitor;
    delete findWindowTitle;
    delete memoryMonitor;
    delete networkMonitor;
    delete processRecvBytes;
    delete processSentBytes;
    delete processReadKbs;
    delete processWriteKbs;
    delete updateStatusTimer;
    delete layout;
}

void StatusMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
}

void StatusMonitor::switchToAllProcess()
{
    filterType = AllProcess;
    tabName = "所有进程";

    updateStatus();
}

void StatusMonitor::switchToOnlyGui()
{
    filterType = OnlyGUI;
    tabName = "应用程序";

    updateStatus();
}

void StatusMonitor::switchToOnlyMe()
{
    filterType = OnlyMe;
    tabName = "我的进程";

    updateStatus();
}

void StatusMonitor::updateStatus()
{
    // Read the list of open processes information.
    PROCTAB* proc = openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLUSR | PROC_FILLCOM);
    static proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_t));

    storedProcType processes;
    while (readproc(proc, &proc_info) != NULL) {
        processes[proc_info.tid]=proc_info;
    }
    closeproc(proc);

    // Fill in CPU.
    if (prevProcesses.size()>0) {
        // we have previous proc info
        for(auto &newItr:processes) {
            for(auto &prevItr:prevProcesses) {
                if (newItr.first == prevItr.first) {
                    // PID matches, calculate the cpu
                    newItr.second.pcpu = (unsigned int) calculateCPUPercentage(&prevItr.second, &newItr.second, totalCpuTime);
                    break;
                }
            }
        }
    }

    // Update the cpu time for next loop.
    totalCpuTime = getTotalCpuTime();

    // Read processes information.
    QString username = qgetenv("USER");

    QList<ListItem*> items;
    int cpuNumber = sysconf(_SC_NPROCESSORS_ONLN);
    double totalCpuPercent = 0;

    findWindowTitle->updateWindowInfos();

    int guiProcessNumber = 0;
    int systemProcessNumber = 0;

    ProcessTree *processTree = new ProcessTree();
    processTree->scanProcesses(processes);

    // Fill gui chlid process information when filterType is OnlyGUI.
    QMap<int, ChildPidInfo> childInfoMap;
    if (filterType == OnlyGUI) {
        QList<int> guiPids = findWindowTitle->getWindowPids();
        for (int guiPid : guiPids) {
            QList<int> childPids;
            childPids = processTree->getAllChildPids(guiPid);

            for (int childPid : childPids) {
                DiskStatus dStatus = {
                    0, 0
                };
                NetworkStatus nStatus = {
                    0, 0, 0, 0
                };
                ChildPidInfo childPidInfo;

                childPidInfo.cpu = 0;
                childPidInfo.memory = 0;
                childPidInfo.diskStatus = dStatus;
                childPidInfo.networkStatus = nStatus;

                childInfoMap[childPid] = childPidInfo;
            }
        }
    }

    for(auto &i:processes) {
        int pid = (&i.second)->tid;
        QString user = (&i.second)->euser;

        double cpu = (&i.second)->pcpu;
        QString name = getProcessName(&i.second);

        std::string desktopFile = getDesktopFileFromName(name);
        bool isGui = (findWindowTitle->getWindowTitle(pid) != "");

        if (isGui) {
            guiProcessNumber++;
        } else {
            systemProcessNumber++;
        }

        bool appendItem = false;
        if (filterType == OnlyGUI) {
            appendItem = (user == username && isGui);
        } else if (filterType == OnlyMe) {
            appendItem = (user == username);
        } else if (filterType == AllProcess) {
            appendItem = true;
        }

        if (appendItem) {
            QString displayName;

            QString title = findWindowTitle->getWindowTitle(pid);
            if (title != "") {
                if (filterType == AllProcess) {
                    displayName = QString("[%1] %2").arg(user).arg(title);
                } else {
                    displayName = title;
                }
            } else {
                if (filterType == AllProcess) {
                    displayName = QString("[%1] %2").arg(user).arg(getDisplayNameFromName(name, desktopFile));
                } else {
                    displayName = getDisplayNameFromName(name, desktopFile);
                }
            }
            long memory = ((&i.second)->resident - (&i.second)->share) * sysconf(_SC_PAGESIZE);
            QPixmap icon;
            if (desktopFile.size() == 0) {
                icon = findWindowTitle->getWindowIcon(findWindowTitle->getWindow(pid), 24);
            } else {
                icon = getDesktopFileIcon(desktopFile, 24);
            }
            ProcessItem *item = new ProcessItem(icon, name, displayName, cpu / cpuNumber, memory, pid, user, (&i.second)->state);
            items << item;
        } else {
            if (filterType == OnlyGUI) {
                if (childInfoMap.contains(pid)) {
                    long memory = ((&i.second)->resident - (&i.second)->share) * sysconf(_SC_PAGESIZE);
                    childInfoMap[pid].cpu = cpu / cpuNumber;
                    childInfoMap[pid].memory = memory;
                }
            }
        }

        totalCpuPercent += cpu;
    }

    // Remove dead process from network status maps.
    for (auto pid : processSentBytes->keys()) {
        bool foundProcess = false;
        for (auto &i:processes) {
            if ((&i.second)->tid == pid) {
                foundProcess = true;
                break;
            }
        }

        if (!foundProcess) {
            processSentBytes->remove(pid);

            qDebug() << QString("Remove %1 from sent bytes map.").arg(pid);
        }
    }
    for (auto pid : processRecvBytes->keys()) {
        bool foundProcess = false;
        for (auto &i:processes) {
            if ((&i.second)->tid == pid) {
                foundProcess = true;
                break;
            }
        }

        if (!foundProcess) {
            processRecvBytes->remove(pid);

            qDebug() << QString("Remove %1 from recv bytes map.").arg(pid);
        }
    }

    // Have procps read the memory。
    meminfo();

    // Update memory status.
    if (kb_swap_total > 0.0)  {
        updateMemoryStatus(kb_main_used * 1024, kb_main_total * 1024, kb_swap_used * 1024, kb_swap_total * 1024);
    } else {
        updateMemoryStatus(kb_main_used * 1024, kb_main_total * 1024, 0, 0);
    }

    if (NetworkTrafficFilter::getNetHogsMonitorStatus() != NETHOGS_STATUS_OK) {
        qDebug() << "Failed to access network device(s).";
    }

    // Update network status.
    NetworkTrafficFilter::Update update;

    QMap<int, NetworkStatus> networkStatusSnapshot;
    totalSentKbs = 0;
    totalRecvKbs = 0;
    while (NetworkTrafficFilter::getRowUpdate(update)) {
        if (update.action != NETHOGS_APP_ACTION_REMOVE) {
            uint32_t prevSentBytes = 0;
            uint32_t prevRecvBytes = 0;

            if (processSentBytes->contains(update.record.pid)) {
                prevSentBytes = processSentBytes->value(update.record.pid);
            }
            if (processRecvBytes->contains(update.record.pid)) {
                prevRecvBytes = processRecvBytes->value(update.record.pid);
            }

            totalSentKbs += update.record.sent_kbs;
            totalRecvKbs += update.record.recv_kbs;

            totalSentBytes += (update.record.sent_bytes - prevSentBytes);
            totalRecvBytes += (update.record.recv_bytes - prevRecvBytes);

            (*processSentBytes)[update.record.pid] = update.record.sent_bytes;
            (*processRecvBytes)[update.record.pid] = update.record.recv_bytes;

            NetworkStatus status = {
                update.record.sent_bytes,
                update.record.recv_bytes,
                update.record.sent_kbs,
                update.record.recv_kbs
            };

            (networkStatusSnapshot)[update.record.pid] = status;
        }
    }

    // Update ProcessItem's network status.
    for (ListItem *item : items) {
        ProcessItem *processItem = static_cast<ProcessItem*>(item);
        if (networkStatusSnapshot.contains(processItem->getPid())) {
            processItem->setNetworkStatus(networkStatusSnapshot.value(processItem->getPid()));
        }

        processItem->setDiskStatus(getProcessDiskStatus(processItem->getPid()));
    }

    for (int childPid : childInfoMap.keys()) {
        // Update network status.
        if (networkStatusSnapshot.contains(childPid)) {
            childInfoMap[childPid].networkStatus = networkStatusSnapshot.value(childPid);
        }

        // Update disk status.
        childInfoMap[childPid].diskStatus = getProcessDiskStatus(childPid);
    }

    // Update cpu status.
    updateCpuStatus(totalCpuPercent / cpuNumber);

    // Merge child process when filterType is OnlyGUI.
    if (filterType == OnlyGUI) {
        for (ListItem *item : items) {
            ProcessItem *processItem = static_cast<ProcessItem*>(item);
            QList<int> childPids;
            childPids = processTree->getAllChildPids(processItem->getPid());

            for (int childPid : childPids) {
                if (childInfoMap.contains(childPid)) {
                    ChildPidInfo info = childInfoMap[childPid];

                    processItem->mergeItemInfo(info.cpu, info.memory, info.diskStatus, info.networkStatus);
                } else {
                    qDebug() << QString("IMPOSSIBLE: process %1 not exist in childInfoMap").arg(childPid);
                }
            }
        }

    }
    delete processTree;

    // Update process status.
    updateProcessStatus(items);

    // Update network status.
    updateNetworkStatus(totalRecvBytes, totalSentBytes, totalRecvKbs, totalSentKbs);

    // Update process number.
    updateProcessNumber(tabName, guiProcessNumber, systemProcessNumber);

    // Keep processes we've read for cpu calculations next cycle.
    prevProcesses = processes;
}

DiskStatus StatusMonitor::getProcessDiskStatus(int pid)
{
    ProcPidIO pidIO;
    getProcPidIO(pid, pidIO);

    DiskStatus status = {0, 0};

    if (processWriteKbs->contains(pid)) {
        status.writeKbs = (pidIO.wchar - processWriteKbs->value(pid)) / (updateDuration / 1000.0);
    }
    (*processWriteKbs)[pid] = pidIO.wchar;

    if (processReadKbs->contains(pid)) {
        status.readKbs = (pidIO.rchar - processReadKbs->value(pid)) / (updateDuration / 1000.0);
    }
    (*processReadKbs)[pid] = pidIO.rchar;

    return status;
}
