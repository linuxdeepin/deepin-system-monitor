// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QElapsedTimer>

/**
 * @brief Reads CPU, memory and network statistics from /proc filesystem.
 */
class SystemMonitorProber : public QObject
{
public:
    explicit SystemMonitorProber(QObject *parent = nullptr);

    int cpuUsagePercent();
    int memoryUsagePercent();
    void networkSpeed(QString *download, QString *upload);

private:
    bool readCpuTimes(quint64 *totalTime, quint64 *idleTime);
    int readMemoryUsagePercent();
    void readNetworkBytes(quint64 *receiveBytes, quint64 *transmitBytes);
    QStringList preferredNetworkInterfaces();
    static QString formatTransferRate(double bytesPerSecond);

    quint64 m_prevCpuTotalTime = 0;
    quint64 m_prevCpuIdleTime = 0;
    quint64 m_prevReceiveBytes = 0;
    quint64 m_prevTransmitBytes = 0;
    QElapsedTimer m_networkSampleTimer;
};
