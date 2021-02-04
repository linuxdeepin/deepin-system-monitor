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

#ifndef MONITOR_EXPAND_VIEW_H
#define MONITOR_EXPAND_VIEW_H

#include <DFrame>

DWIDGET_USE_NAMESPACE

class CpuMonitor;
class MemoryMonitor;
class NetworkMonitor;

/**
 * @brief Expand monitor view frame
 */
class MonitorExpandView : public DFrame
{
    Q_OBJECT

public:
    /**
     * @brief Default constructor
     * @param parent Parent object
     */
    explicit MonitorExpandView(QWidget *parent = nullptr);
    // Default destructor
    ~MonitorExpandView() = default;

public:
    void setDetailButtonVisible(bool visible);

signals:
    void signalDetailInfoClicked();

private:
    // Cpu monitor view
    CpuMonitor *m_cpuMonitor {};
    // Memory monitor view
    MemoryMonitor *m_memoryMonitor {};
    // Network monitor view
    NetworkMonitor *m_networkMonitor {};
};

#endif  // MONITOR_EXPAND_VIEW_H
