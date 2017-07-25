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
#include "find_window_title.h"
#include "memory_monitor.h"
#include "network_monitor.h"
#include "network_traffic_filter.h"
#include "process_item.h"
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
    void updateCpuStatus(double cpuPercent);
    void updateMemoryStatus(long usedMemory, long totalMemory, long usedSwap, long totalSwap);
    void updateNetworkStatus(long totalRecvBytes, long totalSentBytes, float totalRecvKbs, float totalSentKbs);
    void updateProcessNumber(QString tabName, int guiProcessNumber, int systemProcessNumber);
    void updateProcessStatus(QList<ListItem*> items);

public slots:
    void switchToAllProcess();
    void switchToOnlyGui();
    void switchToOnlyMe();
    void updateStatus();

private:
    DiskStatus getProcessDiskStatus(int pid);
    
    CpuMonitor *cpuMonitor;
    FilterType filterType;
    FindWindowTitle *findWindowTitle;
    MemoryMonitor *memoryMonitor;
    NetworkMonitor *networkMonitor;
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
};

#endif
