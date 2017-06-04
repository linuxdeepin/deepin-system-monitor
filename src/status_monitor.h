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

    typedef std::map<int, proc_t> storedProcType;
    
    enum FilterType {OnlyGUI, OnlyMe, AllProcess};

public:
    StatusMonitor(QWidget *parent = 0);
    ~StatusMonitor();

protected:
    void paintEvent(QPaintEvent *event);

signals:
    void updateCpuStatus(double cpuPercent);
    void updateMemoryStatus(long usedMemory, long totalMemory, long usedSwap, long totalSwap);
    void updateNetworkStatus(uint32_t totalRecvBytes, uint32_t totalSentBytes, float totalRecvKbs, float totalSentKbs);
    void updateProcessNumber(QString tabName, int guiProcessNumber, int systemProcessNumber);
    void updateProcessStatus(QList<ListItem*> items);

public slots:
    void switchToAllProcess();
    void switchToOnlyGui();
    void switchToOnlyMe();
    void updateStatus();
                                       
private:
    CpuMonitor *cpuMonitor;
    FilterType filterType;
    FindWindowTitle *findWindowTitle;
    MemoryMonitor *memoryMonitor;
    NetworkMonitor *networkMonitor;
    QMap<QString, QPixmap> *processIconCache;
    QMap<int, uint32_t> *processRecvBytes;
    QMap<int, uint32_t> *processSentBytes;
    QMap<int, unsigned long> *processReadKbs;
    QMap<int, unsigned long> *processWriteKbs;
    QString tabName;
    QTimer *updateStatusTimer;
    QVBoxLayout *layout;
    float totalRecvKbs;
    float totalSentKbs;
    // int updateDuration = 200;
    int updateDuration = 2000;
    storedProcType prevProcesses;
    uint32_t totalRecvBytes;
    uint32_t totalSentBytes;
    unsigned long long totalCpuTime;
};

#endif
