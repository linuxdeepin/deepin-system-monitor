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

#ifndef STATUSMONITOR_H
#define STATUSMONITOR_H

#include "cpu_monitor.h"
#include "utils.h"
#include "find_window_title.h"
#include "settings.h"
#include "memory_monitor.h"
#include "network_monitor.h"
#include "disk_monitor.h"
#include "network_traffic_filter.h"
#include "process_item.h"
#include "compact_cpu_monitor.h"
#include "compact_memory_monitor.h"
#include "compact_disk_monitor.h"
#include "compact_network_monitor.h"
#include <QMap>
#include <QPointF>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <deque>
#include <proc/readproc.h>
#include <proc/sysinfo.h>

class StatusMonitor : public QWidget
{
    Q_OBJECT

    typedef std::map<int, proc_t> StoredProcType;

    enum FilterType {OnlyGUI, OnlyMe, AllProcess};

    struct ChildPidInfo {
        double cpu;
        long memory;
        DiskStatus diskStatus;
        NetworkStatus networkStatus;
    };

public:
    StatusMonitor(int tabIndex);
    ~StatusMonitor();

signals:
    void updateCpuStatus(double cpuPercent, std::vector<double> cpuPercents);
    void updateMemoryStatus(long usedMemory, long totalMemory, long usedSwap, long totalSwap);
    void updateNetworkStatus(long totalRecvBytes, long totalSentBytes, float totalRecvKbs, float totalSentKbs);
    void updateDiskStatus(float totalReadKbs, float totalWriteKbs);
    void updateProcessNumber(QString tabName, int guiProcessNumber, int systemProcessNumber);
    void updateProcessStatus(QList<DSimpleListItem*> items);

public slots:
    void switchToAllProcess();
    void switchToOnlyGui();
    void switchToOnlyMe();
    void updateStatus();
    void showDiskMonitor();
    void hideDiskMonitor();
    
    void handleMemoryStatus(long usedMemory, long totalMemory, long usedSwap, long totalSwap);
    void handleCpuStatus(double cpuPercent, std::vector<double> cpuPercents);
    void handleNetworkStatus(long totalRecvBytes, long totalSentBytes, float totalRecvKbs, float totalSentKbs);
    void handleDiskStatus(float totalReadKbs, float totalWriteKbs);
    
    void initCompactMode();
    
    void enableCompactMode();
    void disableCompactMode();
    
private:
    DiskStatus getProcessDiskStatus(int pid);
    
    bool isCompactMode;
    CpuMonitor *cpuMonitor;
    FilterType filterType;
    FindWindowTitle *findWindowTitle;
    MemoryMonitor *memoryMonitor;
    NetworkMonitor *networkMonitor;
    DiskMonitor *diskMonitor;
    CompactCpuMonitor *compactCpuMonitor;
    CompactNetworkMonitor *compactNetworkMonitor;
    CompactDiskMonitor *compactDiskMonitor;
    CompactMemoryMonitor *compactMemoryMonitor;
    QMap<QString, int> *wineApplicationDesktopMaps;
    QMap<int, QString> *wineServerDesktopMaps;
    QMap<int, double> *processCpuPercents;
    QMap<int, long> *processRecvBytes;
    QMap<int, long> *processSentBytes;
    QMap<int, unsigned long> *processReadKbs;
    QMap<int, unsigned long> *processWriteKbs;
    QString currentUsername;
    QString tabName;
    QTimer *updateStatusTimer;
    QVBoxLayout *layout;
    StoredProcType prevProcesses;
    Settings *settings;
    int updateDuration = 2000;
    qreal updateSeconds;
    unsigned long long int prevTotalRecvBytes;
    unsigned long long int prevTotalSentBytes;
    unsigned long long int totalRecvBytes;
    unsigned long long int totalSentBytes;
    unsigned long long prevTotalCpuTime;
    unsigned long long prevWorkCpuTime;
    unsigned long long totalCpuTime;
    unsigned long long workCpuTime;
    std::vector<CpuStruct> prevCpuTimes;
};

#endif
