// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "monitor_expand_view.h"

#include "cpu_monitor.h"
#include "gpu_monitor.h"
#include "memory_monitor.h"
#include "network_monitor.h"
#include "detailwidgetmanager.h"
#include "ddlog.h"
#include "common/common.h"
#include "model/gpu_info_model.h"

#include <DPalette>
#include <DStyle>

#include <QHBoxLayout>
#include <QVBoxLayout>

using namespace DDLog;

// constructor
MonitorExpandView::MonitorExpandView(QWidget *parent)
    : DFrame(parent)
{
    qCDebug(app) << "MonitorExpandView constructor";
    // disable auto fill frame background
    setAutoFillBackground(false);
    // set background role
    setBackgroundRole(DPalette::Window);
    // frameless style
    setFrameStyle(DFrame::NoFrame);

    // cpu monitor view
    m_cpuMonitor = new CpuMonitor(this);
    // gpu monitor view
    m_gpuMonitor = new GpuMonitor(this);
    // memory monitor view
    m_memoryMonitor = new MemoryMonitor(this);
    // network monitor view
    m_networkMonitor = new NetworkMonitor(this);

    const int totalWidth = common::getStatusBarMaxWidth();
    const int pairSpacing = 8;
    const int cpuWidth = totalWidth * 63 / 100;
    const int gpuWidth = totalWidth - cpuWidth - pairSpacing;
    m_cpuMonitor->setFixedWidth(cpuWidth);
    m_gpuMonitor->setFixedWidth(gpuWidth);

    // monitor view layout
    QVBoxLayout *layout = new QVBoxLayout(this);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    layout->setMargin(0);
#else
    layout->setContentsMargins(0, 0, 0, 0);
#endif
    layout->setSpacing(0);

    QWidget *cpuGpuRow = new QWidget(this);
    QHBoxLayout *cpuGpuLayout = new QHBoxLayout(cpuGpuRow);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    cpuGpuLayout->setMargin(0);
#else
    cpuGpuLayout->setContentsMargins(0, 0, 0, 0);
#endif
    cpuGpuLayout->setSpacing(pairSpacing);
    cpuGpuLayout->addWidget(m_cpuMonitor);
    cpuGpuLayout->addWidget(m_gpuMonitor);
    cpuGpuRow->setFixedWidth(totalWidth);

    auto refreshGpuVisibility = [=]() {
        const bool hasGpu = GPUInfoModel::instance()->gpuCount() > 0;
        m_gpuMonitor->setVisible(hasGpu);
        if (hasGpu) {
            m_cpuMonitor->setFixedWidth(cpuWidth);
            m_gpuMonitor->setFixedWidth(gpuWidth);
        } else {
            m_cpuMonitor->setFixedWidth(totalWidth);
        }
    };
    refreshGpuVisibility();
    connect(GPUInfoModel::instance(), &GPUInfoModel::modelUpdated, this, refreshGpuVisibility);

    layout->addStretch(1);
    layout->addWidget(cpuGpuRow, 0, Qt::AlignHCenter);
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
        qCDebug(app) << "Received signal from DBus to show detail info:" << msgCode;
        emit signalDetailInfoByDbus(msgCode);
    });

    //点击左侧区域触发跳转
    connect(m_cpuMonitor, &CpuMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_gpuMonitor, &GpuMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_memoryMonitor, &MemoryMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_networkMonitor, &NetworkMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
}

void MonitorExpandView::setDetailButtonVisible(bool visible)
{
    qCDebug(app) << "Setting detail button visibility to:" << visible;
    m_cpuMonitor->setDetailButtonVisible(visible);
}
