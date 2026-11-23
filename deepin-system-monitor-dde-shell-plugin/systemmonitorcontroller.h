// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYSTEMMONITORCONTROLLER_H
#define SYSTEMMONITORCONTROLLER_H

#include <QObject>

class SystemMonitorProber;
class QTimer;

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
    // 采集是否有效：失败时 QML 降级展示占位符 --% / --
    Q_PROPERTY(bool dataValid READ dataValid NOTIFY systemStatsChanged FINAL)
    // 是否存在活跃网卡连接：断网时 QML 将网速列置灰提示未连接
    Q_PROPERTY(bool networkConnected READ networkConnected NOTIFY systemStatsChanged FINAL)

public:
    explicit SystemMonitorController(QObject *parent = nullptr);
    ~SystemMonitorController() override;

    int cpuUsage() const;
    int memoryUsage() const;
    QString downloadSpeedText() const;
    QString uploadSpeedText() const;
    bool dataValid() const;
    bool networkConnected() const;

    Q_INVOKABLE void openSystemMonitor();
    // 按需可见采样：卡片不可见/任务栏隐藏时挂起轮询，恢复可见时立即刷新首帧
    Q_INVOKABLE void setSamplingPaused(bool paused);

signals:
    void systemStatsChanged();

private slots:
    void refreshSystemStats();

private:
    SystemMonitorProber *m_prober = nullptr;
    QTimer *m_refreshTimer = nullptr;

    int m_cpuUsage = 0;
    int m_memoryUsage = 0;
    QString m_downloadSpeedText = QStringLiteral("0 KB/s");
    QString m_uploadSpeedText = QStringLiteral("0 KB/s");
    bool m_dataValid = false;
    bool m_networkConnected = false;
    bool m_samplingPaused = false;
};

#endif // SYSTEMMONITORCONTROLLER_H
