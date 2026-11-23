// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemmonitorcontroller.h"
#include "systemmonitorprober.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QProcess>
#include <QTimer>
#include <QDebug>

namespace {
const QLatin1String kMonitorServerService("com.deepin.SystemMonitorServer");
const QLatin1String kMonitorServerPath("/com/deepin/SystemMonitorServer");
const QLatin1String kMonitorServerInterface("com.deepin.SystemMonitorServer");
const QLatin1String kShowMonitorMethod("showDeepinSystemMoniter");
const QLatin1String kMonitorApp("deepin-system-monitor");
}

SystemMonitorController::SystemMonitorController(QObject *parent)
    : QObject(parent)
    , m_prober(new SystemMonitorProber(this))
{
    auto *timer = new QTimer(this);
    timer->setInterval(2000);
    connect(timer, &QTimer::timeout, this, &SystemMonitorController::refreshSystemStats);
    timer->start();
    m_refreshTimer = timer;

    refreshSystemStats();
}

SystemMonitorController::~SystemMonitorController()
{
}

int SystemMonitorController::cpuUsage() const
{
    return m_cpuUsage;
}

int SystemMonitorController::memoryUsage() const
{
    return m_memoryUsage;
}

QString SystemMonitorController::downloadSpeedText() const
{
    return m_downloadSpeedText;
}

QString SystemMonitorController::uploadSpeedText() const
{
    return m_uploadSpeedText;
}

bool SystemMonitorController::dataValid() const
{
    return m_dataValid;
}

bool SystemMonitorController::networkConnected() const
{
    return m_networkConnected;
}

void SystemMonitorController::setSamplingPaused(bool paused)
{
    if (m_samplingPaused == paused)
        return;

    m_samplingPaused = paused;
    if (!m_refreshTimer)
        return;

    if (paused) {
        m_refreshTimer->stop();
        return;
    }

    // 切回可见时立即刷新首帧，再恢复周期采样
    m_refreshTimer->start();
    refreshSystemStats();
}

void SystemMonitorController::openSystemMonitor()
{
    QDBusInterface serverIface(kMonitorServerService,
                               kMonitorServerPath,
                               kMonitorServerInterface,
                               QDBusConnection::sessionBus());
    if (serverIface.isValid()) {
        serverIface.asyncCall(kShowMonitorMethod);
        return;
    }

    if (!QProcess::startDetached(kMonitorApp))
        qWarning() << "system-monitor: failed to launch" << kMonitorApp;
}

void SystemMonitorController::refreshSystemStats()
{
    const int prevCpu = m_cpuUsage;
    const int prevMem = m_memoryUsage;
    const QString prevDown = m_downloadSpeedText;
    const QString prevUp = m_uploadSpeedText;
    const bool prevDataValid = m_dataValid;
    const bool prevNetworkConnected = m_networkConnected;

    int nextCpu = m_prober->cpuUsagePercent();
    if (nextCpu >= 0)
        m_cpuUsage = nextCpu;

    int nextMem = m_prober->memoryUsagePercent();
    if (nextMem >= 0)
        m_memoryUsage = nextMem;

    // 采集任一失败即视为无效，QML 降级展示占位符
    m_dataValid = nextCpu >= 0 && nextMem >= 0;

    QString nextDown;
    QString nextUp;
    m_prober->networkSpeed(&nextDown, &nextUp);
    m_downloadSpeedText = nextDown;
    m_uploadSpeedText = nextUp;

    m_networkConnected = m_prober->hasActiveNetwork();

    if (prevCpu != m_cpuUsage
        || prevMem != m_memoryUsage
        || prevDown != m_downloadSpeedText
        || prevUp != m_uploadSpeedText
        || prevDataValid != m_dataValid
        || prevNetworkConnected != m_networkConnected) {
        emit systemStatsChanged();
    }
}
