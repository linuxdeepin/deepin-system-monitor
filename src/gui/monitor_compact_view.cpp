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

#include "monitor_compact_view.h"

#include "compact_cpu_monitor.h"
#include "compact_disk_monitor.h"
#include "compact_memory_monitor.h"
#include "compact_network_monitor.h"

#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>

#include <QVBoxLayout>

// constructor
MonitorCompactView::MonitorCompactView(QWidget *parent)
    : DFrame(parent)
{
    // disable auto fill frame background
    setAutoFillBackground(false);
    // set frame background role
    setBackgroundRole(DPalette::Window);
    // set no frame borders
    setFrameStyle(DFrame::NoFrame);

    // cpu monitor view instance
    m_cpuMonitor = new CompactCpuMonitor(this);
    // memory monitor view instance
    m_memoryMonitor = new CompactMemoryMonitor(this);
    // network monitor view instance
    m_networkMonitor = new CompactNetworkMonitor(this);
    // disk monitor view instance
    m_diskMonitor = new CompactDiskMonitor(this);

    // vertical layout to hold monitor instances
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(10);
    layout->addStretch(1);
    layout->addWidget(m_cpuMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_memoryMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_networkMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_diskMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(1);

    // set frame layout
    setLayout(layout);

    connect(m_cpuMonitor, &CompactCpuMonitor::signalDetailInfoClicked, this, &MonitorCompactView::signalDetailInfoClicked);
}

void MonitorCompactView::setDetailButtonVisible(bool visible)
{
    m_cpuMonitor->setDetailButtonVisible(visible);
}
