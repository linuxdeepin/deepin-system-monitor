// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYSTEM_SERVICE_CLIENT_H
#define SYSTEM_SERVICE_CLIENT_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QTimer>
#include <QVariantMap>

namespace core {
namespace process {

class SystemServiceClient : public QObject
{
    Q_OBJECT

public:
    explicit SystemServiceClient(QObject *parent = nullptr);
    ~SystemServiceClient();

    // 检查服务是否可用
    bool isServiceAvailable() const;
    
    // 检查 DKapture 是否可用
    bool isDKaptureAvailable();
    
    // 批量获取进程信息
    QVariantMap getProcessInfoBatch(const QList<int> &pids);
    
    // 启动系统服务
    bool startSystemService();

signals:
    void serviceConnectionChanged(bool connected);
    void dkaptureAvailabilityChanged(bool available);

private slots:
    void onServiceRegistered(const QString &serviceName);
    void onServiceUnregistered(const QString &serviceName);
    void checkServiceConnection();

private:
    void connectToService();
    void disconnectFromService();

    QDBusInterface *m_interface;
    QDBusServiceWatcher *m_serviceWatcher;
    QTimer *m_connectionTimer;
    bool m_serviceAvailable;
    bool m_dkaptureAvailable;
    
    static const QString SERVICE_NAME;
    static const QString SERVICE_PATH;
    static const QString SERVICE_INTERFACE;
};

} // namespace process
} // namespace core

#endif // SYSTEM_SERVICE_CLIENT_H 