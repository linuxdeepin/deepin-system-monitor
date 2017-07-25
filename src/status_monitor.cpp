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

StatusMonitor::StatusMonitor(int tabIndex)
{
    // Init size.
    setFixedWidth(Utils::getStatusBarMaxWidth());

    // Init attributes.
    findWindowTitle = new FindWindowTitle();
    processReadKbs = new QMap<int, unsigned long>();
    processRecvBytes = new QMap<int, long>();
    processSentBytes = new QMap<int, long>();
    processWriteKbs = new QMap<int, unsigned long>();
    processCpuPercents = new QMap<int, double>();
    wineApplicationDesktopMaps = new QMap<QString, int>();
    wineServerDesktopMaps = new QMap<int, QString>();

    if (tabIndex == 0) {
        tabName = tr("Applications");
        filterType = OnlyGUI;
    } else if (tabIndex == 1) {
        tabName = tr("My processes");
        filterType = OnlyMe;
    } else {
        tabName = tr("All processes");
        filterType = AllProcess;
    }

    totalCpuTime = 0;
    workCpuTime = 0;
    prevTotalCpuTime = 0;
    prevWorkCpuTime = 0;
    currentUsername = qgetenv("USER");

    prevTotalRecvBytes = 0;
    prevTotalSentBytes = 0;

    updateSeconds = updateDuration / 1000.0;

    // Init widgets.
    layout = new QVBoxLayout(this);

    cpuMonitor = new CpuMonitor();
    memoryMonitor = new MemoryMonitor();
    networkMonitor = new NetworkMonitor();

    layout->addWidget(cpuMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(memoryMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(networkMonitor, 0, Qt::AlignHCenter);

    connect(this, &StatusMonitor::updateMemoryStatus, memoryMonitor, &MemoryMonitor::updateStatus, Qt::QueuedConnection);
    connect(this, &StatusMonitor::updateCpuStatus, cpuMonitor, &CpuMonitor::updateStatus, Qt::QueuedConnection);
    connect(this, &StatusMonitor::updateNetworkStatus, networkMonitor, &NetworkMonitor::updateStatus, Qt::QueuedConnection);

    // Start timer.
    updateStatusTimer = new QTimer(this);
    connect(updateStatusTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    updateStatusTimer->start(updateDuration);
}

StatusMonitor::~StatusMonitor()
{
    delete wineApplicationDesktopMaps;
    delete wineServerDesktopMaps;
    delete findWindowTitle;
    delete processRecvBytes;
    delete processSentBytes;
    delete processReadKbs;
    delete processWriteKbs;
    delete processCpuPercents;
}

void StatusMonitor::switchToAllProcess()
{
    filterType = AllProcess;
    tabName = tr("All processes");

    updateStatus();
}

void StatusMonitor::switchToOnlyGui()
{
    filterType = OnlyGUI;
    tabName = tr("Applications");

    updateStatus();
}

void StatusMonitor::switchToOnlyMe()
{
    filterType = OnlyMe;
    tabName = tr("My processes");

    updateStatus();
}

void StatusMonitor::updateStatus()
{
    // Read the list of open processes information.
    PROCTAB* proc = openproc(
        PROC_FILLMEM |          // memory status: read information from /proc/#pid/statm
        PROC_FILLSTAT |         // cpu status: read information from /proc/#pid/stat
        PROC_FILLUSR            // user status: resolve user ids to names via /etc/passwd
        );
    static proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_t));

    StoredProcType processes;
    while (readproc(proc, &proc_info) != NULL) {
        processes[proc_info.tid] = proc_info;
    }
    closeproc(proc);

    // Fill in CPU.
    prevWorkCpuTime = workCpuTime;
    prevTotalCpuTime = totalCpuTime;
    totalCpuTime = getTotalCpuTime(workCpuTime);

    processCpuPercents->clear();
    if (prevProcesses.size()>0) {
        // we have previous proc info
        for (auto &newItr:processes) {
            for (auto &prevItr:prevProcesses) {
                if (newItr.first == prevItr.first) {
                    // PID matches, calculate the cpu
                    (*processCpuPercents)[newItr.second.tid] = calculateCPUPercentage(&prevItr.second, &newItr.second, prevTotalCpuTime, totalCpuTime);

                    break;
                }
            }
        }
    }

    // Fill gui chlid process information when filterType is OnlyGUI.
    findWindowTitle->updateWindowInfos();
    ProcessTree *processTree = new ProcessTree();
    processTree->scanProcesses(processes);
    QMap<int, ChildPidInfo> childInfoMap;
    if (filterType == OnlyGUI) {
        QList<int> guiPids = findWindowTitle->getWindowPids();
        for (int guiPid : guiPids) {
            QList<int> childPids;
            childPids = processTree->getAllChildPids(guiPid);

            for (int childPid : childPids) {
                DiskStatus dStatus = {0, 0};
                NetworkStatus nStatus = {0, 0, 0, 0};
                ChildPidInfo childPidInfo;

                childPidInfo.cpu = 0;
                childPidInfo.memory = 0;
                childPidInfo.diskStatus = dStatus;
                childPidInfo.networkStatus = nStatus;

                childInfoMap[childPid] = childPidInfo;
            }
        }
    }

    // Read processes information.
    int guiProcessNumber = 0;
    int systemProcessNumber = 0;
    QList<ListItem*> items;

    wineApplicationDesktopMaps->clear();
    wineServerDesktopMaps->clear();

    for (auto &i:processes) {
        int pid = (&i.second)->tid;
        QString cmdline = Utils::getProcessCmdline(pid);
        bool isWineProcess = cmdline.startsWith("c:\\");
        QString name = getProcessName(&i.second, cmdline);
        QString user = (&i.second)->euser;
        double cpu = (*processCpuPercents)[pid];

        std::string desktopFile = getDesktopFileFromName(pid, name, cmdline);
        QString title = findWindowTitle->getWindowTitle(pid);

        bool isGui = (title != "");

        // Record wine application and wineserver.real desktop file.
        // We need transfer wineserver.real network traffic to the corresponding wine program.
        if (name == "wineserver.real") {
            // Insert pid<->desktopFile to map to search in all network process list.
            QString gioDesktopFile = Utils::getProcessEnvironmentVariable(pid, "GIO_LAUNCHED_DESKTOP_FILE");
            if (gioDesktopFile != "") {
                (*wineServerDesktopMaps)[pid] = gioDesktopFile;
            }
        } else {
            // Insert desktopFile<->pid to map to search in all network process list.
            // If title is empty, it's just a wine program, but not wine GUI window.
            if (isWineProcess && title != "") {
                (*wineApplicationDesktopMaps)[QString::fromStdString(desktopFile)] = pid;
            }
        }

        if (isGui) {
            guiProcessNumber++;
        } else {
            systemProcessNumber++;
        }

        bool appendItem = false;
        if (filterType == OnlyGUI) {
            appendItem = (user == currentUsername && isGui);
        } else if (filterType == OnlyMe) {
            appendItem = (user == currentUsername);
        } else if (filterType == AllProcess) {
            appendItem = true;
        }

        if (appendItem) {
            if (title == "") {
                if (isWineProcess) {
                    // If wine process's window title is blank, it's not GUI window process.
                    // Title use process name instead.
                    title = name;
                } else {
                    title = getDisplayNameFromName(name, desktopFile);
                }
            }
            QString displayName;
            if (filterType == AllProcess) {
                displayName = QString("[%1] %2").arg(user).arg(title);
            } else {
                displayName = title;
            }

            long memory = ((&i.second)->resident - (&i.second)->share) * sysconf(_SC_PAGESIZE);

            QPixmap icon;
            if (desktopFile.size() == 0) {
                icon = findWindowTitle->getWindowIcon(findWindowTitle->getWindow(pid), 24);
            } else {
                icon = getDesktopFileIcon(desktopFile, 24);
            }

            ProcessItem *item = new ProcessItem(icon, name, displayName, cpu, memory, pid, user, (&i.second)->state);
            items << item;
        } else {
            // Fill GUI processes information for continue merge action.
            if (filterType == OnlyGUI) {
                if (childInfoMap.contains(pid)) {
                    long memory = ((&i.second)->resident - (&i.second)->share) * sysconf(_SC_PAGESIZE);
                    childInfoMap[pid].cpu = cpu;
                    childInfoMap[pid].memory = memory;
                }
            }
        }
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
        }
    }

    // Read memory information.
    meminfo();

    // Update memory status.
    if (kb_swap_total > 0.0)  {
        updateMemoryStatus((kb_main_total - kb_main_available) * 1024, kb_main_total * 1024, kb_swap_used * 1024, kb_swap_total * 1024);
    } else {
        updateMemoryStatus((kb_main_total - kb_main_available) * 1024, kb_main_total * 1024, 0, 0);
    }

    // Update process's network status.
    NetworkTrafficFilter::Update update;

    QMap<int, NetworkStatus> networkStatusSnapshot;

    while (NetworkTrafficFilter::getRowUpdate(update)) {
        if (update.action != NETHOGS_APP_ACTION_REMOVE) {
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

    // Transfer wineserver.real network traffic to the corresponding wine program.
    QMap<int, NetworkStatus>::iterator i;
    for (i = networkStatusSnapshot.begin(); i != networkStatusSnapshot.end(); ++i) {
        if (wineServerDesktopMaps->contains(i.key())) {
            QString wineDesktopFile = (*wineServerDesktopMaps)[i.key()];

            if (wineApplicationDesktopMaps->contains(wineDesktopFile)) {
                // Transfer wineserver.real network traffic to the corresponding wine program.
                int wineApplicationPid = (*wineApplicationDesktopMaps)[wineDesktopFile];
                networkStatusSnapshot[wineApplicationPid] = networkStatusSnapshot[i.key()];

                // Reset wineserver network status to zero.
                NetworkStatus networkStatus = {0, 0, 0, 0};
                networkStatusSnapshot[i.key()] = networkStatus;
            }
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
    if (prevWorkCpuTime != 0 && prevTotalCpuTime != 0) {
        updateCpuStatus((workCpuTime - prevWorkCpuTime) * 100.0 / (totalCpuTime - prevTotalCpuTime));
    } else {
        updateCpuStatus(0);
    }

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
    if (prevTotalRecvBytes == 0) {
        prevTotalRecvBytes = totalRecvBytes;
        prevTotalSentBytes = totalSentBytes;

        Utils::getNetworkBandWidth(totalRecvBytes, totalSentBytes);
        updateNetworkStatus(totalRecvBytes, totalSentBytes, 0, 0);
    } else {
        prevTotalRecvBytes = totalRecvBytes;
        prevTotalSentBytes = totalSentBytes;

        Utils::getNetworkBandWidth(totalRecvBytes, totalSentBytes);
        updateNetworkStatus(totalRecvBytes,
                            totalSentBytes,
                            ((totalRecvBytes - prevTotalRecvBytes) / 1024.0) / updateSeconds,
                            ((totalSentBytes - prevTotalSentBytes) / 1024.0) / updateSeconds);
    }

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
        status.writeKbs = (pidIO.wchar - processWriteKbs->value(pid)) / updateSeconds;
    }
    (*processWriteKbs)[pid] = pidIO.wchar;

    if (processReadKbs->contains(pid)) {
        status.readKbs = (pidIO.rchar - processReadKbs->value(pid)) / updateSeconds;
    }
    (*processReadKbs)[pid] = pidIO.rchar;

    return status;
}
