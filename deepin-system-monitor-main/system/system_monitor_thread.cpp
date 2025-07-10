// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "system_monitor_thread.h"
#include "ddlog.h"

#include "system/system_monitor.h"
#include "process/process_db.h"

#include <QReadLocker>
#include <QWriteLocker>
#include <QDebug>
#include <QTimer>

using namespace DDLog;
namespace core {
namespace system {

SystemMonitorThread::SystemMonitorThread(QObject *parent)
    : BaseThread(parent)
    , m_monitor(new SystemMonitor())
{
    qCDebug(app) << "SystemMonitorThread created";
    m_monitor->moveToThread(this);
    connect(this, &QThread::finished, this, &QObject::deleteLater);
    connect(this, &QThread::started, m_monitor, &SystemMonitor::startMonitorJob);
}

SystemMonitorThread::~SystemMonitorThread()
{
    qCDebug(app) << "SystemMonitorThread destroyed, quitting and waiting for thread to finish";
    m_monitor->deleteLater();
    quit();
    wait();
}

SystemMonitor *SystemMonitorThread::systemMonitorInstance() const
{
    // qCDebug(app) << "Accessing SystemMonitor instance";
    return m_monitor;
}

} // namespace system
} // namespace core
