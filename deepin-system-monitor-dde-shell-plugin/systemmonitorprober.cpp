// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemmonitorprober.h"

#include <QFile>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QTimer>

namespace {
const QLatin1String kProcStat("/proc/stat");
const QLatin1String kProcMeminfo("/proc/meminfo");
const QLatin1String kProcNetDev("/proc/net/dev");
const QLatin1String kZeroSpeed("0 KB/s");
}


SystemMonitorProber::SystemMonitorProber(QObject *parent)
    : QObject(parent)
{
}

int SystemMonitorProber::cpuUsagePercent()
{
    quint64 totalTime = 0;
    quint64 idleTime = 0;
    if (!readCpuTimes(&totalTime, &idleTime))
        return -1;

    if (m_prevCpuTotalTime > 0 && totalTime > m_prevCpuTotalTime) {
        const quint64 totalDelta = totalTime - m_prevCpuTotalTime;
        const quint64 idleDelta = idleTime - m_prevCpuIdleTime;
        const double busyRatio = totalDelta > 0
            ? (static_cast<double>(totalDelta - qMin(idleDelta, totalDelta)) * 100.0 / totalDelta)
            : 0.0;
        m_prevCpuTotalTime = totalTime;
        m_prevCpuIdleTime = idleTime;
        return qBound(0, qRound(busyRatio), 100);
    }

    m_prevCpuTotalTime = totalTime;
    m_prevCpuIdleTime = idleTime;
    return 0;
}

int SystemMonitorProber::memoryUsagePercent()
{
    return readMemoryUsagePercent();
}

void SystemMonitorProber::networkSpeed(QString *download, QString *upload)
{
    if (!download || !upload)
        return;

    quint64 receiveBytes = 0;
    quint64 transmitBytes = 0;
    readNetworkBytes(&receiveBytes, &transmitBytes);

    if (!m_networkSampleTimer.isValid()) {
        m_networkSampleTimer.start();
        m_prevReceiveBytes = receiveBytes;
        m_prevTransmitBytes = transmitBytes;
        *download = kZeroSpeed;
        *upload = kZeroSpeed;
        return;
    }

    const double elapsedSeconds = qMax(0.001, m_networkSampleTimer.restart() / 1000.0);

    quint64 receiveDelta = receiveBytes >= m_prevReceiveBytes
        ? (receiveBytes - m_prevReceiveBytes) : 0;
    quint64 transmitDelta = transmitBytes >= m_prevTransmitBytes
        ? (transmitBytes - m_prevTransmitBytes) : 0;

    m_prevReceiveBytes = receiveBytes;
    m_prevTransmitBytes = transmitBytes;

    *download = formatTransferRate(receiveDelta / elapsedSeconds);
    *upload = formatTransferRate(transmitDelta / elapsedSeconds);
}

bool SystemMonitorProber::readCpuTimes(quint64 *totalTime, quint64 *idleTime)
{
    if (!totalTime || !idleTime)
        return false;

    QFile file(kProcStat);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    const QString line = QString::fromUtf8(file.readLine()).simplified();
    const QStringList fields = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (fields.size() < 5 || fields.first() != QLatin1String("cpu"))
        return false;

    quint64 total = 0;
    for (qsizetype i = 1; i < fields.size(); ++i)
        total += fields.at(i).toULongLong();

    // idle = idle + iowait (fields 4, 5, 0-indexed from "cpu")
    const quint64 idle = fields.value(4).toULongLong() + fields.value(5).toULongLong();
    *totalTime = total;
    *idleTime = idle;
    return true;
}

int SystemMonitorProber::readMemoryUsagePercent()
{
    QFile file(kProcMeminfo);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;

    quint64 totalMemory = 0;
    quint64 availableMemory = 0;
    quint64 freeMemory = 0;
    quint64 bufferMemory = 0;
    quint64 cachedMemory = 0;
    quint64 reclaimableMemory = 0;
    quint64 shmemMemory = 0;

    while (true) {
        const QByteArray rawLine = file.readLine();
        if (rawLine.isEmpty())
            break;

        const QString line = QString::fromUtf8(rawLine);
        if (line.startsWith(QLatin1String("MemTotal:")))
            totalMemory = line.section(QLatin1Char(':'), 1).simplified().section(QLatin1Char(' '), 0, 0).toULongLong();
        else if (line.startsWith(QLatin1String("MemAvailable:")))
            availableMemory = line.section(QLatin1Char(':'), 1).simplified().section(QLatin1Char(' '), 0, 0).toULongLong();
        else if (line.startsWith(QLatin1String("MemFree:")))
            freeMemory = line.section(QLatin1Char(':'), 1).simplified().section(QLatin1Char(' '), 0, 0).toULongLong();
        else if (line.startsWith(QLatin1String("Buffers:")))
            bufferMemory = line.section(QLatin1Char(':'), 1).simplified().section(QLatin1Char(' '), 0, 0).toULongLong();
        else if (line.startsWith(QLatin1String("Cached:")))
            cachedMemory = line.section(QLatin1Char(':'), 1).simplified().section(QLatin1Char(' '), 0, 0).toULongLong();
        else if (line.startsWith(QLatin1String("SReclaimable:")))
            reclaimableMemory = line.section(QLatin1Char(':'), 1).simplified().section(QLatin1Char(' '), 0, 0).toULongLong();
        else if (line.startsWith(QLatin1String("Shmem:")))
            shmemMemory = line.section(QLatin1Char(':'), 1).simplified().section(QLatin1Char(' '), 0, 0).toULongLong();

        if (totalMemory > 0 && availableMemory > 0)
            break;
    }

    // Fallback estimation when MemAvailable is absent
    if (availableMemory == 0 && totalMemory > 0) {
        const quint64 estimated = freeMemory + bufferMemory + cachedMemory + reclaimableMemory;
        availableMemory = estimated > shmemMemory ? (estimated - shmemMemory) : 0;
    }

    if (totalMemory == 0)
        return -1;

    const quint64 usedMemory = totalMemory > availableMemory ? (totalMemory - availableMemory) : 0;
    return qBound(0, qRound(static_cast<double>(usedMemory) * 100.0 / totalMemory), 100);
}

void SystemMonitorProber::readNetworkBytes(quint64 *receiveBytes, quint64 *transmitBytes)
{
    if (!receiveBytes || !transmitBytes)
        return;

    *receiveBytes = 0;
    *transmitBytes = 0;

    QFile file(kProcNetDev);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // Skip the first two header lines
    file.readLine();
    file.readLine();

    const QStringList interfaces = preferredNetworkInterfaces();
    QSet<QString> interfaceSet(interfaces.begin(), interfaces.end());

    while (true) {
        const QByteArray rawLine = file.readLine();
        if (rawLine.isEmpty())
            break;

        const QString line = QString::fromUtf8(rawLine).simplified();
        // Format: "eth0:  1234  0  0  0  5678  0  0  0  9012  0  0  0  0  0  0  0"
        const int colonPos = line.indexOf(':');
        if (colonPos < 0)
            continue;

        const QString ifaceName = line.left(colonPos).trimmed();
        if (!interfaceSet.isEmpty() && !interfaceSet.contains(ifaceName))
            continue;

        const QStringList fields = line.mid(colonPos + 1).split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        // receive: bytes(0), packets(1), errs(2), drop(3), ...
        // transmit: bytes(8), packets(9), errs(10), ...
        if (fields.size() >= 9) {
            *receiveBytes += fields.at(0).toULongLong();
            *transmitBytes += fields.at(8).toULongLong();
        }
    }
}

QStringList SystemMonitorProber::preferredNetworkInterfaces()
{
    const QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    QStringList result;

    for (const QNetworkInterface &iface : allInterfaces) {
        const auto flags = iface.flags();
        const bool isUp = (flags & QNetworkInterface::IsUp) && (flags & QNetworkInterface::IsRunning);
        const bool isLoopback = (flags & QNetworkInterface::IsLoopBack);
        const QString name = iface.name().trimmed();

        if (!isUp || isLoopback || name.isEmpty())
            continue;

        // Skip virtual/tunnel interfaces
        if (name.startsWith(QLatin1String("veth"))
            || name.startsWith(QLatin1String("br-"))
            || name.startsWith(QLatin1String("docker"))
            || name.startsWith(QLatin1String("virbr"))
            || name.startsWith(QLatin1String("tun"))
            || name.startsWith(QLatin1String("tap"))
            || name.startsWith(QLatin1String("wg"))
            || name.startsWith(QLatin1String("dummy"))) {
            continue;
        }

        result << name;
    }
    return result;
}

QString SystemMonitorProber::formatTransferRate(double bytesPerSecond)
{
    if (bytesPerSecond < 1024)
        return QStringLiteral("%1 B/s").arg(qRound(bytesPerSecond));
    if (bytesPerSecond < 1024 * 1024)
        return QStringLiteral("%1 KB/s").arg(qRound(bytesPerSecond / 1024));
    return QStringLiteral("%1 MB/s").arg(bytesPerSecond / (1024 * 1024), 0, 'f', 1);
}

