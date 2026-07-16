// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class SystemMonitorProber;

class SystemMonitorApplet : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cpuUsage READ cpuUsage NOTIFY systemStatsChanged FINAL)
    Q_PROPERTY(int memoryUsage READ memoryUsage NOTIFY systemStatsChanged FINAL)
    Q_PROPERTY(QString downloadSpeedText READ downloadSpeedText NOTIFY systemStatsChanged FINAL)
    Q_PROPERTY(QString uploadSpeedText READ uploadSpeedText NOTIFY systemStatsChanged FINAL)

public:
    explicit SystemMonitorApplet(QObject *parent = nullptr);
    ~SystemMonitorApplet() override;

    int cpuUsage() const;
    int memoryUsage() const;
    QString downloadSpeedText() const;
    QString uploadSpeedText() const;

    Q_INVOKABLE void openSystemMonitor();

signals:
    void systemStatsChanged();

private slots:
    void refreshSystemStats();

private:
    SystemMonitorProber *m_prober = nullptr;

    int m_cpuUsage = 0;
    int m_memoryUsage = 0;
    QString m_downloadSpeedText = QStringLiteral("0 KB/s");
    QString m_uploadSpeedText = QStringLiteral("0 KB/s");
};
