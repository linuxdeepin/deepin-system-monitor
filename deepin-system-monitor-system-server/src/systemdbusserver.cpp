// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemdbusserver.h"
#include "ddlog.h"

#ifdef ENABLE_DKAPTURE
#include "dkapture_manager.h"
#include <QSet>
#endif

#include <QCoreApplication>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QStandardPaths>
#include <QProcess>
#include <QTimer>
#include <QFile>

#include <sys/sysinfo.h>
#include <unistd.h>
#include <QStringList>
#include <QDebug>
#include <QRegularExpression>
#include <unistd.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <polkit-qt5-1/PolkitQt1/Authority>
#include <polkit-qt5-1/PolkitQt1/Subject>
#else
#include <polkit-qt6-1/PolkitQt1/Authority>
#include <polkit-qt6-1/PolkitQt1/Subject>
#endif

using namespace DDLog;

const QString s_PolkitActionSet = "org.deepin.systemmonitor.systemserver.set";

/**
   @brief polkit 鉴权，通过配置文件处理
 */
bool checkAuthorization(const QString &appBusName, const QString &action)
{
    PolkitQt1::Authority::Result ret = PolkitQt1::Authority::instance()->checkAuthorizationSync(
            action, PolkitQt1::SystemBusNameSubject(appBusName), PolkitQt1::Authority::AllowUserInteraction);
    if (PolkitQt1::Authority::Yes == ret) {
        qCDebug(app) << "Authorization check passed";
        return true;
    } else {
        qCWarning(app) << "Policy authorization check failed for bus:" << appBusName;
        return false;
    }
}

/**
   @return 返回PID对应的可执行程序名称
 */
static QString getProcIdExe(qint64 id)
{
    qCDebug(app) << "Getting executable for PID:" << id;
    QString execName;
    if (id > 0) {
        // Read contents of virtual /proc/{pid}/cmdline file
        QString exeSymlinkPath = QString("/proc/%1/exe").arg(id);
        char *actualpath = realpath(exeSymlinkPath.toStdString().c_str(), nullptr);
        execName = QString(actualpath);
        free(actualpath); // free the memory allocated by realpath
    }
    qCDebug(app) << "Executable for PID" << id << "is:" << execName;
    return execName;
}

SystemDBusServer::SystemDBusServer(QObject *parent)
    : QObject(parent)
#ifdef ENABLE_DKAPTURE
    , m_dkaptureManager(nullptr)
    , m_dkaptureInitialized(false)
#endif
{
    qCDebug(app) << "SystemDBusServer created";
    
    // 初始化 DKapture
    initializeDKapture();
    // name: 配置文件中的服务名称 org.deepin.SystemMonitorSystemServer
    QDBusConnection dbus = QDBusConnection::systemBus();
    if (dbus.registerService("org.deepin.SystemMonitorSystemServer")) {
        qCDebug(app) << "Successfully registered service org.deepin.SystemMonitorSystemServer";
        QDBusConnection::RegisterOptions opts =
                QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties;
        if (!dbus.registerObject("/org/deepin/SystemMonitorSystemServer", this, opts)) {
            qCWarning(app) << "Failed to register DBus object:" << dbus.lastError().message();
        }
    } else {
        qCWarning(app) << "Failed to register DBus service:" << dbus.lastError().message();
    }
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [=]() { 
        qCDebug(app) << "Timer timeout, exiting application";
        qApp->exit(0); 
    });
}

/**
   @brief 延时退出执行服务
 */
void SystemDBusServer::exitDBusServer(int msec)
{
    qCDebug(app) << "Scheduling exit in" << msec << "ms";
    qApp->processEvents();
    m_timer.start(msec);
}

/**
   @brief 重置退出定时器（当有新的D-Bus调用时）
 */
void SystemDBusServer::resetExitTimer()
{
    static const int EXIT_TIMEOUT = 5000; // 5秒无活动后退出
    qCDebug(app) << "Resetting exit timer to" << EXIT_TIMEOUT << "ms";
    m_timer.start(EXIT_TIMEOUT);
}

/**
   @brief 设置 systemctl 服务 \a serviceName 的启动方式为 \b enable ，仅用于非开发者模式
 */
QString SystemDBusServer::setServiceEnable(const QString &serviceName, bool enable)
{
    qCDebug(app) << "setServiceEnable called for service:" << serviceName << "enable:" << enable;

    // 重置退出定时器
    resetExitTimer();

    QString ret = setServiceEnableImpl(serviceName, enable);
    return ret;
}

/**
   @brief 执行设置 systemctl 服务 \a serviceName 启动方式 \a enable ，将返回详细处理结果
 */
QString SystemDBusServer::setServiceEnableImpl(const QString &serviceName, bool enable)
{
    qCDebug(app) << "setServiceEnableImpl called for service:" << serviceName << "enable:" << enable;

    // 不允许包含';' ' '字符，服务名称长度同样限制
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (serviceName.isEmpty() || (serviceName.size() > SHRT_MAX) || serviceName.contains(QRegExp("[; ]"))) {
#else
    if (serviceName.isEmpty() || (serviceName.size() > SHRT_MAX) || serviceName.contains(QRegularExpression("[; ]"))) {
#endif
        qCWarning(app) << "Invalid service name:" << serviceName;
        return QString(strerror(EINVAL));
    }

    // 判断服务是否存在
    QProcess listPorcess;
    listPorcess.start("systemctl", { "list-unit-files", "--type=service", "--no-pager" });
    listPorcess.waitForFinished();
    QByteArray serviceList = listPorcess.readAll();
    qCDebug(app) << "Checking for service existence...";
    if (!serviceList.contains(serviceName.toUtf8())) {
        qCWarning(app) << "Service does not exist:" << serviceName;
        return QString(strerror(EINVAL));
    }
    qCDebug(app) << "Service" << serviceName << "exists.";

    // 鉴权处理
    if (!checkAuthorization(message().service(), s_PolkitActionSet)) {
        qCWarning(app) << "Polkit authorization failed for service:" << serviceName;
        return QString(strerror(EPERM));
    }

    // 执行设置
    QProcess process;
    qCDebug(app) << "Executing 'systemctl" << (enable ? "enable" : "disable") << serviceName << "'";
    process.start("systemctl", { enable ? "enable" : "disable", serviceName });
    process.waitForFinished();
    QString errorRet = process.readAll();

    // 检测是否执行成功
    qCDebug(app) << "Checking service status after operation...";
    process.start("systemctl", { "is-enabled", serviceName });
    process.waitForFinished();
    QString checkRet = process.readAll();
    if (enable && ("enabled" == checkRet)) {
        qCDebug(app) << "Service successfully enabled:" << serviceName;
        return {};
    } else if (!enable && ("disabled" == checkRet)) {
        qCDebug(app) << "Service successfully disabled:" << serviceName;
        return {};
    }

    // 返回命令执行结果
    qCWarning(app) << "Service state change failed:" << errorRet;
    return errorRet;
}

/**
   @return DBus 调用者的PID
 */
qint64 SystemDBusServer::dbusCallerPid() const
{
    if (!calledFromDBus()) {
        qCDebug(app) << "Not called from DBus";
        return 0;
    }

    auto interface = connection().interface();
    if (interface) {
        return static_cast<qint64>(interface->servicePid(message().service()).value());
    }

    qCDebug(app) << "Failed to get DBus caller PID";
    return 0;
}

// ================== DKapture 相关实现 ==================

void SystemDBusServer::initializeDKapture()
{
#ifdef ENABLE_DKAPTURE
    qCInfo(app) << "SystemServer: Initializing DKapture...";
    
    m_dkaptureManager = DKaptureManager::instance();
    if (m_dkaptureManager && m_dkaptureManager->isAvailable()) {
        m_dkaptureInitialized = true;
        qCInfo(app) << "SystemServer: DKapture initialized successfully";
    } else {
        qCWarning(app) << "SystemServer: DKapture initialization failed";
        m_dkaptureInitialized = false;
    }
#else
    qCDebug(app) << "SystemServer: DKapture support not compiled";
#endif
}

void SystemDBusServer::cleanupDKapture()
{
#ifdef ENABLE_DKAPTURE
    if (m_dkaptureManager) {
        delete m_dkaptureManager;
        m_dkaptureManager = nullptr;
    }
    m_dkaptureInitialized = false;
    qCDebug(app) << "SystemServer: DKapture cleaned up";
#endif
}

bool SystemDBusServer::isDKaptureAvailable()
{
    qCDebug(app) << "SystemServer: isDKaptureAvailable called";

    // 重置退出定时器
    resetExitTimer();

#ifdef ENABLE_DKAPTURE
    bool available = m_dkaptureInitialized && m_dkaptureManager && m_dkaptureManager->isAvailable();
    qCDebug(app) << "SystemServer: DKapture availability:" << available;
    return available;
#else
    qCDebug(app) << "SystemServer: DKapture not compiled, returning false";
    return false;
#endif
}

QVariantMap SystemDBusServer::getProcessInfoBatch(const QList<int> &pids)
{
    qCDebug(app) << "SystemServer: getProcessInfoBatch called for" << pids.size() << "PIDs";

    // 重置退出定时器
    resetExitTimer();
    
    QVariantMap result;
    result["success"] = false;
    result["error"] = "Unknown error";

#ifdef ENABLE_DKAPTURE
    if (!isDKaptureAvailable()) {
        result["error"] = "DKapture not available";
        return result;
    }

    try {
        QVariantMap processData;
        
        // 使用 DKapture 获取进程信息
        if (m_dkaptureManager) {
            // 准备要获取的数据类型 - 禁用网络数据监听，前端使用传统方式
            std::vector<DKapture::DataType> dataTypes = {
                DKapture::PROC_PID_STAT,    // 基本进程信息
                DKapture::PROC_PID_IO,      // I/O 信息
                DKapture::PROC_PID_STATM,   // 内存信息
                DKapture::PROC_PID_STATUS,  // 进程状态信息（UID/GID等）
                DKapture::PROC_PID_SCHEDSTAT // 调度统计信息（cpu_time, rq_wait_time, timeslices）
                // DKapture::PROC_PID_traffic  // 网络流量信息 - 已禁用，前端使用传统方式
            };
            
            // 创建一个结构体来传递上下文
            struct DKaptureContext {
                QSet<int> targetPids;
                QVariantMap *processData;
                SystemDBusServer *server;
            } context;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            context.targetPids = QSet<int>(pids.begin(), pids.end());
#else
            context.targetPids = QSet<int>::fromList(pids);
#endif
            context.processData = &processData;
            context.server = this;
            
            // 设置回调函数来处理 DKapture 数据
            auto callback = [](void *ctx, const void *data, size_t /*data_sz*/) -> int {
                DKaptureContext *context = static_cast<DKaptureContext *>(ctx);
                const DKapture::DataHdr *hdr = static_cast<const DKapture::DataHdr *>(data);
                
                // qCDebug(app) << "SystemServer: DKapture callback called for PID" << hdr->pid << "type" << hdr->type;
                
                // 检查是否为目标PID
                bool isTargetPid = context->targetPids.contains(hdr->pid);
                if (!isTargetPid) {
                    // 对于子线程或子程序，记录基本信息但不立即打印详细数据
                    // 详细数据将在处理完成后统一格式化输出
                }
                
                QString pidKey = QString::number(hdr->pid);
                QVariantMap pidData;
                
                // 如果已有此 PID 的数据，则获取它
                if (context->processData->contains(pidKey)) {
                    pidData = context->processData->value(pidKey).toMap();
                } else {
                    pidData["pid"] = hdr->pid;
                    pidData["tgid"] = hdr->tgid;
                    pidData["comm"] = QString::fromUtf8(hdr->comm, TASK_COMM_LEN);
                }
                

                
                // 根据数据类型解析数据
                const char *payload = hdr->data;
                switch (hdr->type) {
                    case DKapture::PROC_PID_STAT: {
                        // qCDebug(app) << "SystemServer: Processing STAT for PID" << hdr->pid << "existing keys:" << pidData.keys();
                        const ProcPidStat *stat = reinterpret_cast<const ProcPidStat *>(payload);
                        // qCDebug(app) << "SystemServer: STAT raw data - vsize:" << stat->vsize << "rss:" << stat->rss;
    

                        pidData["state"] = stat->state;
                        pidData["ppid"] = stat->ppid;
                        
                        // CPU时间处理：不做增量计算，直接转换（除以10^7）
                        static const qulonglong DK_CONVERSION_FACTOR = 10000000ULL; // 10^7
                        
                        qulonglong dk_utime = stat->utime / DK_CONVERSION_FACTOR;
                        qulonglong dk_stime = stat->stime / DK_CONVERSION_FACTOR;
                        qulonglong dk_cutime = stat->cutime / DK_CONVERSION_FACTOR;
                        qulonglong dk_cstime = stat->cstime / DK_CONVERSION_FACTOR;

                        // Convert back to jiffies for frontend compatibility
                        // Frontend expects utime/stime in jiffies, not seconds
                        long hz = sysconf(_SC_CLK_TCK);  // Get system HZ value
                        qulonglong dk_utime_jiffies = dk_utime * hz;  // Convert seconds back to jiffies
                        qulonglong dk_stime_jiffies = dk_stime * hz;
                        qulonglong dk_cutime_jiffies = dk_cutime * hz;
                        qulonglong dk_cstime_jiffies = dk_cstime * hz;
                        
                        // Check for abnormal CPU time values that could cause overflow
                        // Use system uptime as a reasonable upper bound
                        struct sysinfo si;
                        if (sysinfo(&si) == 0) {
                            qulonglong system_uptime_jiffies = si.uptime * hz;
                            qulonglong total_cpu_jiffies = dk_utime_jiffies + dk_stime_jiffies;
                            
                            if (total_cpu_jiffies > system_uptime_jiffies * 2) { // Allow 2x system uptime as buffer
                                qCWarning(app) << "SystemServer: Abnormally large DKapture CPU time for PID" << hdr->pid
                                              << "- total CPU jiffies:" << total_cpu_jiffies 
                                              << "system uptime jiffies:" << system_uptime_jiffies
                                              << ". Data may be corrupted, setting to safe values.";
                                
                                // Set to very small values to prevent frontend calculation issues
                                // This ensures CPU usage will be close to 0% rather than astronomical values
                                dk_utime_jiffies = hz;  // 1 second worth of jiffies
                                dk_stime_jiffies = hz;  // 1 second worth of jiffies  
                                dk_cutime_jiffies = 0;
                                dk_cstime_jiffies = 0;
                            }
                        }

                        // Store in jiffies for frontend compatibility
                        pidData["utime"] = dk_utime_jiffies;
                        pidData["stime"] = dk_stime_jiffies;
                        pidData["cutime"] = dk_cutime_jiffies;
                        pidData["cstime"] = dk_cstime_jiffies;
                        pidData["cpu_time"] = dk_utime + dk_stime; // Keep original seconds for reference
                        

                        pidData["priority"] = stat->priority;
                        pidData["nice"] = stat->nice;
                        pidData["num_threads"] = stat->num_threads;
                        pidData["start_time"] = static_cast<qulonglong>(stat->start_time);
                        pidData["vsize"] = static_cast<qulonglong>(stat->vsize);
                        pidData["rss"] = static_cast<qulonglong>(stat->rss);
                        break;
                    }
                    case DKapture::PROC_PID_IO: {
                        // qCDebug(app) << "SystemServer: Processing IO for PID" << hdr->pid << "existing keys:" << pidData.keys();
                        const ProcPidIo *io = reinterpret_cast<const ProcPidIo *>(payload);
                        
                        pidData["rchar"] = static_cast<qulonglong>(io->rchar);
                        pidData["wchar"] = static_cast<qulonglong>(io->wchar);
                        pidData["syscr"] = static_cast<qulonglong>(io->syscr);
                        pidData["syscw"] = static_cast<qulonglong>(io->syscw);
                        pidData["read_bytes"] = static_cast<qulonglong>(io->read_bytes);
                        pidData["write_bytes"] = static_cast<qulonglong>(io->write_bytes);
                        pidData["cancelled_write_bytes"] = static_cast<qulonglong>(io->cancelled_write_bytes);
                        break;
                    }
                    case DKapture::PROC_PID_STATM: {
                        // qCDebug(app) << "SystemServer: Processing STATM for PID" << hdr->pid << "existing keys:" << pidData.keys();
                        const ProcPidStatm *statm = reinterpret_cast<const ProcPidStatm *>(payload);
                        // qCDebug(app) << "SystemServer: STATM raw data - size:" << statm->size << "resident:" << statm->resident << "shared:" << statm->shared;
    

                        pidData["memory_size"] = static_cast<qulonglong>(statm->size);
                        pidData["memory_resident"] = static_cast<qulonglong>(statm->resident);
                        pidData["memory_shared"] = static_cast<qulonglong>(statm->shared);
                        pidData["memory_text"] = static_cast<qulonglong>(statm->text);
                        pidData["memory_data"] = static_cast<qulonglong>(statm->data);
                        break;
                    }
                    case DKapture::PROC_PID_STATUS: {
                        // qCDebug(app) << "SystemServer: 🔍 Processing STATUS for PID" << hdr->pid << "existing keys:" << pidData.keys();
                        const ProcPidStatus *status = reinterpret_cast<const ProcPidStatus *>(payload);
                        
                        // // 调试：输出原始STATUS数据
                        // qCDebug(app) << "SystemServer: 📊 Raw STATUS data for PID" << hdr->pid 
                        //             << "- uid[0-3]:" << status->uid[0] << status->uid[1] << status->uid[2] << status->uid[3]
                        //             << "- gid[0-3]:" << status->gid[0] << status->gid[1] << status->gid[2] << status->gid[3]
                        //             << "- state:" << (char)status->state << "- tracer_pid:" << status->tracer_pid;
                        
                        // UID/GID信息 - 前端主要使用real uid/gid
                        pidData["uid"] = status->uid[0];        // real uid
                        pidData["euid"] = status->uid[1];       // effective uid  
                        pidData["suid"] = status->uid[2];       // saved uid
                        pidData["fsuid"] = status->uid[3];      // filesystem uid
                        pidData["gid"] = status->gid[0];        // real gid
                        pidData["egid"] = status->gid[1];       // effective gid
                        pidData["sgid"] = status->gid[2];       // saved gid
                        pidData["fsgid"] = status->gid[3];      // filesystem gid
                        
                        // 进程状态和调试信息
                        pidData["state_from_status"] = status->state;
                        pidData["tracer_pid"] = status->tracer_pid;
                        pidData["umask"] = status->umask;
                        
                        // qCDebug(app) << "SystemServer: ✅ STATUS data processed for PID" << hdr->pid 
                        //             << "- final uid:" << pidData["uid"].toUInt() << "gid:" << pidData["gid"].toUInt();
                        break;
                    }
                    case DKapture::PROC_PID_traffic: {
                        // 网络流量信息处理已禁用 - 前端将使用传统方式获取网络数据
                        // 这样可以避免DKapture系统级流量vs传统用户态socket流量的数据量差异问题
                        qCDebug(app) << "SystemServer: Skipping traffic data for PID" << hdr->pid << "- using traditional network monitoring";
                        break;
                    }
                    case DKapture::PROC_PID_SCHEDSTAT: {
                        // qCDebug(app) << "SystemServer: 🔍 Processing SCHEDSTAT for PID" << hdr->pid;
                        const ProcPidSchedstat *schedstat = reinterpret_cast<const ProcPidSchedstat *>(payload);
                        
                        // SchedStat数据：只传递rq_wait_time，与原有实现保持一致
                        pidData["rq_wait_time"] = static_cast<qulonglong>(schedstat->rq_wait_time);
                        
                        // qCDebug(app) << "SystemServer: ✅ SCHEDSTAT data processed for PID" << hdr->pid 
                        //             << "- rq_wait_time:" << pidData["rq_wait_time"].toULongLong();
                        break;
                    }
                    default:
                        break;
                }
                
                // 只有目标PID的数据才添加到结果中返回给前端
                if (isTargetPid) {
                    context->processData->insert(pidKey, pidData);
                    // qCDebug(app) << "SystemServer: Updated target PID" << hdr->pid << "final keys:" << pidData.keys();
                }
                return 0;
            };
            
            // 读取 DKapture 数据
            qCDebug(app) << "SystemServer: About to read DKapture data for" << pids.size() << "PIDs";
            // qCDebug(app) << "SystemServer: Target PIDs:" << pids;
            
            ssize_t bytesRead = m_dkaptureManager->read(dataTypes, callback, &context);
            
            // qCDebug(app) << "SystemServer: DKapture read returned" << bytesRead << "bytes";
            qCDebug(app) << "SystemServer: Process data collected for" << processData.size() << "processes";
            
            if (bytesRead >= 0) {
                result["success"] = true;
                result["data"] = processData;
                result.remove("error");
                qCInfo(app) << "SystemServer: Successfully read" << bytesRead << "bytes of DKapture data for" << processData.size() << "processes";
            } else {
                result["error"] = QString("Failed to read DKapture data: %1").arg(bytesRead);
                qCWarning(app) << "SystemServer: Failed to read DKapture data:" << bytesRead;
            }
        } else {
            result["error"] = "DKapture manager not initialized";
        }
        
    } catch (const std::exception &e) {
        qCWarning(app) << "SystemServer: Exception in getProcessInfoBatch:" << e.what();
        result["error"] = QString("Exception: %1").arg(e.what());
    }
#else
    result["error"] = "DKapture support not compiled";
#endif

    return result;
}
