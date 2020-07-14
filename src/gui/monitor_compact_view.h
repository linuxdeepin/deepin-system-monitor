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
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MONITOR_COMPACT_VIEW_H
#define MONITOR_COMPACT_VIEW_H

#include <DFrame>

DWIDGET_USE_NAMESPACE

class CompactCpuMonitor;
class CompactMemoryMonitor;
class CompactNetworkMonitor;
class CompactDiskMonitor;

class MonitorCompactView : public DFrame
{
    Q_OBJECT

public:
    explicit MonitorCompactView(QWidget *parent = nullptr);
    ~MonitorCompactView() = default;

private:
    CompactCpuMonitor *m_cpuMonitor;
    CompactMemoryMonitor *m_memoryMonitor;
    CompactNetworkMonitor *m_networkMonitor;
    CompactDiskMonitor *m_diskMonitor;
};

#endif  // MONITOR_COMPACT_VIEW_H
