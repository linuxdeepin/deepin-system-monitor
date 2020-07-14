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

#include "monitor_expand_view.h"

#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "network_monitor.h"

#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>

#include <QVBoxLayout>

MonitorExpandView::MonitorExpandView(QWidget *parent)
    : DFrame(parent)
{
    auto margin = DStyle::pixelMetric(style(), DStyle::PM_ContentsMargins);

    setAutoFillBackground(false);
    setBackgroundRole(DPalette::Window);
    setFrameStyle(DFrame::NoFrame);

    m_cpuMonitor = new CpuMonitor(this);
    m_memoryMonitor = new MemoryMonitor(this);
    m_networkMonitor = new NetworkMonitor(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->addStretch(1);
    layout->addWidget(m_cpuMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_memoryMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_networkMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(1);

    layout->setContentsMargins(margin, margin, margin, margin);
    setLayout(layout);
}
