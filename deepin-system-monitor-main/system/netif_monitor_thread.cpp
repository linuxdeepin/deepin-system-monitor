// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netif_monitor_thread.h"
#include "netif_monitor.h"
#include "ddlog.h"

using namespace DDLog;

namespace core {
namespace system {

NetifMonitorThread::NetifMonitorThread(QObject *parent)
    : BaseThread(parent)
    , m_netifMonitor(new NetifMonitor)
{
    qCDebug(app) << "NetifMonitorThread constructor";
    m_netifMonitor->moveToThread(&m_netIfmoniterThread);
    connect(&m_netIfmoniterThread, &QThread::finished, this, &QObject::deleteLater);
    connect(&m_netIfmoniterThread, &QThread::started, m_netifMonitor, &NetifMonitor::startNetmonitorJob);
    m_netIfmoniterThread.start();
    qCDebug(app) << "NetifMonitorThread: m_netIfmoniterThread started";
}

NetifMonitorThread::~NetifMonitorThread()
{
    // qCDebug(app) << "NetifMonitorThread destructor";
    m_netifMonitor->requestQuit();
    m_netIfmoniterThread.quit();
    m_netIfmoniterThread.wait();
    // qCDebug(app) << "NetifMonitorThread: m_netIfmoniterThread finished";

    m_netifMonitor->deleteLater();
    quit();
    wait();
    // qCDebug(app) << "NetifMonitorThread finished";
}

NetifMonitor *NetifMonitorThread::netifJobInstance() const
{
    return m_netifMonitor;
}

void NetifMonitorThread::run()
{
    qCDebug(app) << "NetifMonitorThread::run() started, calling handleNetData()";
    m_netifMonitor->handleNetData();
    qCDebug(app) << "NetifMonitorThread::run() finished";
}

} // namespace system
} // namespace core
