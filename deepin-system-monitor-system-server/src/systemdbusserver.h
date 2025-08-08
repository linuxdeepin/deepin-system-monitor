// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBUSALARMNOTIFY_H
#define DBUSALARMNOTIFY_H

#include <QObject>
#include <QDBusContext>
#include <QTimer>
#include <QVariantMap>
#include <QMap>
#include <QMutex>

#ifdef ENABLE_DKAPTURE
#include "dkapture_manager.h"
#endif

class SystemDBusServer : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.SystemMonitorSystemServer")

public:
    SystemDBusServer(QObject *parent = nullptr);
    ~SystemDBusServer() override {}

    void exitDBusServer(int msec);
    void resetExitTimer();

public Q_SLOTS:
    QString setServiceEnable(const QString &serviceName, bool enable);
    
    // DKapture 相关方法
    bool isDKaptureAvailable();
    QVariantMap getProcessInfoBatch(const QList<int> &pids);


private:
    QString setServiceEnableImpl(const QString &serviceName, bool enable);
    qint64 dbusCallerPid() const;
    bool checkCaller() const;

private:
    void initializeDKapture();
    void cleanupDKapture();
    QVariantMap processDataToVariant(const void *data, const QString &dataType);
    
    QTimer m_timer;

#ifdef ENABLE_DKAPTURE
    DKaptureManager *m_dkaptureManager;
    bool m_dkaptureInitialized;
    
    // 用于增量计算的数据结构
    struct ProcessDeltaData {
        qulonglong utime = 0;
        qulonglong stime = 0; 
        qulonglong cutime = 0;
        qulonglong cstime = 0;
        // 网络字段已禁用 - 前端使用传统方式避免数据量差异
        // qulonglong rbytes = 0;  // 网络接收字节数
        // qulonglong wbytes = 0;  // 网络发送字节数
        qulonglong read_bytes = 0;   // 磁盘读字节数
        qulonglong write_bytes = 0;  // 磁盘写字节数
        qulonglong cancelled_write_bytes = 0;
        // 每个字段分别跟踪是否为第一次采集
        bool utimeFirstTime = true;
        bool stimeFirstTime = true;
        bool cutimeFirstTime = true;
        bool cstimeFirstTime = true;
        // 网络字段标记已禁用 - 前端使用传统方式
        // bool rbytesFirstTime = true;
        // bool wbytesFirstTime = true;
        bool readBytesFirstTime = true;
        bool writeBytesFirstTime = true;
        bool cancelledWriteBytesFirstTime = true;
    };
    QMap<int, ProcessDeltaData> m_processLastValues;  // 存储每个进程的上一次累积值
    QMutex m_deltaDataMutex;  // 保护增量数据的互斥锁
#endif
};

#endif  // DBUS_OBJECT_H
