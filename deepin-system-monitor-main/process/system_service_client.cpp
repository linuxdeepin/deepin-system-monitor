// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "system_service_client.h"
#include "ddlog.h"

#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusArgument>
#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>

using namespace DDLog;

namespace core {
namespace process {

const QString SystemServiceClient::SERVICE_NAME = "org.deepin.SystemMonitorSystemServer";
const QString SystemServiceClient::SERVICE_PATH = "/org/deepin/SystemMonitorSystemServer";
const QString SystemServiceClient::SERVICE_INTERFACE = "org.deepin.SystemMonitorSystemServer";

SystemServiceClient::SystemServiceClient(QObject *parent)
    : QObject(parent)
    , m_interface(nullptr)
    , m_serviceWatcher(nullptr)
    , m_connectionTimer(nullptr)
    , m_serviceAvailable(false)
    , m_dkaptureAvailable(false)
{
    qCDebug(app) << "SystemServiceClient created";
    
    // 初始化 D-Bus 服务监视器
    m_serviceWatcher = new QDBusServiceWatcher(SERVICE_NAME, 
                                               QDBusConnection::systemBus(),
                                               QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
                                               this);
    
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered,
            this, &SystemServiceClient::onServiceRegistered);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &SystemServiceClient::onServiceUnregistered);
    
    // 连接检查定时器
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setSingleShot(false);
    m_connectionTimer->setInterval(5000); // 每5秒检查一次
    connect(m_connectionTimer, &QTimer::timeout,
            this, &SystemServiceClient::checkServiceConnection);
    
    // 立即尝试连接
    connectToService();
    m_connectionTimer->start();
}

SystemServiceClient::~SystemServiceClient()
{
    qCDebug(app) << "SystemServiceClient destroyed";
    disconnectFromService();
}

bool SystemServiceClient::isServiceAvailable() const
{
    return m_serviceAvailable && m_interface && m_interface->isValid();
}

bool SystemServiceClient::isDKaptureAvailable()
{
    if (!isServiceAvailable()) {
        return false;
    }
    
    QDBusReply<bool> reply = m_interface->call("isDKaptureAvailable");
    if (reply.isValid()) {
        m_dkaptureAvailable = reply.value();
        return m_dkaptureAvailable;
    }
    
    qCWarning(app) << "Failed to check DKapture availability:" << reply.error().message();
    return false;
}

QVariantMap SystemServiceClient::getProcessInfoBatch(const QList<int> &pids)
{
    QVariantMap result;
    result["success"] = false;
    
    if (!isServiceAvailable()) {
        result["error"] = "Service not available";
        return result;
    }
    
    qCInfo(app) << "Calling getProcessInfoBatch with" << pids.size() << "PIDs";
    QDBusReply<QVariantMap> reply = m_interface->call("getProcessInfoBatch", QVariant::fromValue(pids));
    if (reply.isValid()) {
        QVariantMap data = reply.value();
        qCInfo(app) << "D-Bus call successful, received data with keys:" << data.keys();
        qCInfo(app) << "success field:" << data["success"];
        if (data.contains("data")) {
            QVariant dataField = data["data"];
            QVariantMap processData;
            
            // 处理 QDBusArgument 类型
            if (dataField.canConvert<QDBusArgument>()) {
                // qCInfo(app) << "Converting QDBusArgument to QVariantMap";
                QDBusArgument arg = dataField.value<QDBusArgument>();
                arg >> processData;
                 
                 // 直接转换所有QDBusArgument数据，无需预检查
                 QVariantMap fixedProcessData;
                 for (auto it = processData.begin(); it != processData.end(); ++it) {
                     QVariant pVar = it.value();
                     if (pVar.canConvert<QDBusArgument>()) {
                         QDBusArgument pArg = pVar.value<QDBusArgument>();
                         QVariantMap pMap;
                         pArg >> pMap;
                         fixedProcessData[it.key()] = pMap;
                     } else {
                         // 已经是QVariantMap或其他格式，直接使用
                         fixedProcessData[it.key()] = pVar;
                     }
                 }
                 processData = fixedProcessData;
                 qCInfo(app) << "Converted process data, total processes:" << processData.size();
                
                // 更新返回数据中的 data 字段
                QVariantMap result = data;
                result["data"] = processData;
                return result;
            } else {
                processData = dataField.toMap();
                qCInfo(app) << "Direct conversion - Process data has" << processData.size() << "entries";
            }
        }
        return data;
    }
    
    result["error"] = QString("D-Bus call failed: %1").arg(reply.error().message());
    qCWarning(app) << "getProcessInfoBatch failed:" << reply.error().message();
    return result;
}



bool SystemServiceClient::startSystemService()
{
    qCDebug(app) << "Attempting to start system service";
    
    // 尝试找到 systemctl 命令
    QStringList possiblePaths = {
        "/usr/bin/systemctl",
        "/bin/systemctl",
        "/sbin/systemctl"
    };
    
    QString systemctlPath;
    for (const QString &path : possiblePaths) {
        if (QFileInfo::exists(path)) {
            systemctlPath = path;
            break;
        }
    }
    
    if (systemctlPath.isEmpty()) {
        qCWarning(app) << "systemctl not found";
        return false;
    }
    
    // 启动服务
    QProcess process;
    process.start(systemctlPath, QStringList() << "start" << "deepin-system-monitor-system-server.service");
    
    if (!process.waitForFinished(10000)) { // 10秒超时
        qCWarning(app) << "Failed to start system service: timeout";
        return false;
    }
    
    if (process.exitCode() != 0) {
        qCWarning(app) << "Failed to start system service:" << process.readAllStandardError();
        return false;
    }
    
    qCInfo(app) << "System service start command executed successfully";
    
    // 等待一下服务启动
    QTimer::singleShot(2000, this, [this]() {
        connectToService();
    });
    
    return true;
}

void SystemServiceClient::onServiceRegistered(const QString &serviceName)
{
    qCInfo(app) << "Service registered:" << serviceName;
    if (serviceName == SERVICE_NAME) {
        connectToService();
    }
}

void SystemServiceClient::onServiceUnregistered(const QString &serviceName)
{
    qCInfo(app) << "Service unregistered:" << serviceName;
    if (serviceName == SERVICE_NAME) {
        disconnectFromService();
    }
}

void SystemServiceClient::checkServiceConnection()
{
    if (!m_serviceAvailable) {
        qCDebug(app) << "Service not available, attempting to connect";
        connectToService();
    }
}

void SystemServiceClient::connectToService()
{
    if (m_interface) {
        delete m_interface;
        m_interface = nullptr;
    }
    
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        qCWarning(app) << "Cannot connect to system D-Bus";
        m_serviceAvailable = false;
        emit serviceConnectionChanged(false);
        return;
    }
    
    m_interface = new QDBusInterface(SERVICE_NAME, SERVICE_PATH, SERVICE_INTERFACE, bus, this);
    
    if (m_interface->isValid()) {
        m_serviceAvailable = true;
        qCInfo(app) << "Successfully connected to system service";
        emit serviceConnectionChanged(true);
        
        // 检查 DKapture 可用性
        if (isDKaptureAvailable()) {
            qCInfo(app) << "DKapture is available in system service";
            emit dkaptureAvailabilityChanged(true);
        }
    } else {
        qCWarning(app) << "Failed to connect to system service:" << m_interface->lastError().message();
        m_serviceAvailable = false;
        emit serviceConnectionChanged(false);
    }
}

void SystemServiceClient::disconnectFromService()
{
    if (m_interface) {
        delete m_interface;
        m_interface = nullptr;
    }
    
    m_serviceAvailable = false;
    m_dkaptureAvailable = false;
    emit serviceConnectionChanged(false);
    emit dkaptureAvailabilityChanged(false);
}

} // namespace process
} // namespace core 