// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYSTEMMONITORCONTROLLER_H
#define SYSTEMMONITORCONTROLLER_H

#include <QObject>

class SystemMonitorProber;

/**
 * @brief The SystemMonitorController class
 * 卡片的数据/行为对象，以 QML 上下文属性暴露给 SystemMonitorCard.qml，
 * 负责轮询系统状态并提供打开主界面的动作。
 */
class SystemMonitorController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cpuUsage READ cpuUsage NOTIFY systemStatsChanged FINAL)
    Q_PROPERTY(int memoryUsage READ memoryUsage NOTIFY systemStatsChanged FINAL)
    Q_PROPERTY(QString downloadSpeedText READ downloadSpeedText NOTIFY systemStatsChanged FINAL)
    Q_PROPERTY(QString uploadSpeedText READ uploadSpeedText NOTIFY systemStatsChanged FINAL)

public:
    explicit SystemMonitorController(QObject *parent = nullptr);
    ~SystemMonitorController() override;

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

#endif // SYSTEMMONITORCONTROLLER_H
