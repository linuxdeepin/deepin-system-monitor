// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemmonitorapplet.h"
#include "systemmonitorprober.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QProcess>
#include <QTimer>

namespace {
const QLatin1String kMonitorServerService("com.deepin.SystemMonitorServer");
const QLatin1String kMonitorServerPath("/com/deepin/SystemMonitorServer");
const QLatin1String kMonitorServerInterface("com.deepin.SystemMonitorServer");
const QLatin1String kShowMonitorMethod("showDeepinSystemMoniter");
const QLatin1String kMonitorApp("deepin-system-monitor");
}

SystemMonitorApplet::SystemMonitorApplet(QObject *parent)
    : QObject(parent)
    , m_prober(new SystemMonitorProber(this))
{
    auto *timer = new QTimer(this);
    timer->setInterval(2000);
    connect(timer, &QTimer::timeout, this, &SystemMonitorApplet::refreshSystemStats);
    timer->start();

    refreshSystemStats();
}

SystemMonitorApplet::~SystemMonitorApplet()
{
}

int SystemMonitorApplet::cpuUsage() const
{
    return m_cpuUsage;
}

int SystemMonitorApplet::memoryUsage() const
{
    return m_memoryUsage;
}

QString SystemMonitorApplet::downloadSpeedText() const
{
    return m_downloadSpeedText;
}

QString SystemMonitorApplet::uploadSpeedText() const
{
    return m_uploadSpeedText;
}

void SystemMonitorApplet::openSystemMonitor()
{
    QDBusInterface serverIface(kMonitorServerService,
                               kMonitorServerPath,
                               kMonitorServerInterface,
                               QDBusConnection::sessionBus());
    if (serverIface.isValid()) {
        serverIface.asyncCall(kShowMonitorMethod);
        return;
    }

    QProcess::startDetached(kMonitorApp);
}

void SystemMonitorApplet::refreshSystemStats()
{
    const int prevCpu = m_cpuUsage;
    const int prevMem = m_memoryUsage;
    const QString prevDown = m_downloadSpeedText;
    const QString prevUp = m_uploadSpeedText;

    int nextCpu = m_prober->cpuUsagePercent();
    if (nextCpu >= 0)
        m_cpuUsage = nextCpu;

    int nextMem = m_prober->memoryUsagePercent();
    if (nextMem >= 0)
        m_memoryUsage = nextMem;

    QString nextDown;
    QString nextUp;
    m_prober->networkSpeed(&nextDown, &nextUp);
    m_downloadSpeedText = nextDown;
    m_uploadSpeedText = nextUp;

    if (prevCpu != m_cpuUsage
        || prevMem != m_memoryUsage
        || prevDown != m_downloadSpeedText
        || prevUp != m_uploadSpeedText) {
        emit systemStatsChanged();
    }
}
