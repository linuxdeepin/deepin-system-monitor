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

#include "monitor_expand_view.h"

#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "network_monitor.h"
#include "detailwidgetmanager.h"

#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>

#include <QVBoxLayout>

// constructor
MonitorExpandView::MonitorExpandView(QWidget *parent)
    : DFrame(parent)
{
    // disable auto fill frame background
    setAutoFillBackground(false);
    // set background role
    setBackgroundRole(DPalette::Window);
    // frameless style
    setFrameStyle(DFrame::NoFrame);

    // cpu monitor view
    m_cpuMonitor = new CpuMonitor(this);
    // memory monitor view
    m_memoryMonitor = new MemoryMonitor(this);
    // network monitor view
    m_networkMonitor = new NetworkMonitor(this);

    // monitor view layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addStretch(1);
    layout->addWidget(m_cpuMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_memoryMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_networkMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(1);
    // set monitor view layout
    setLayout(layout);

    connect(m_cpuMonitor, &CpuMonitor::signalDetailInfoClicked, this, &MonitorExpandView::signalDetailInfoClicked);
    // 连接dbus发来的信号
    connect(&DetailWidgetManager::getInstance(), &DetailWidgetManager::sigJumpToDetailWidget, this, [=](QString msgCode) {
        emit signalDetailInfoByDbus(msgCode);
    });

    //点击左侧区域触发跳转
    connect(m_cpuMonitor, &CpuMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_memoryMonitor, &MemoryMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_networkMonitor, &NetworkMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
}

void MonitorExpandView::setDetailButtonVisible(bool visible)
{
    m_cpuMonitor->setDetailButtonVisible(visible);
}
