// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemdbusserver.h"
#include "ddlog.h"

#ifdef ENABLE_DKAPTURE
#include "dkapture_manager.h"
#include "dkapture-api.h"
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
    // 调用者限制前台系统监视器程序
    if (!checkCaller()) {
        qCWarning(app) << "Unauthorized caller attempt to modify service:" << serviceName;
        return QString(strerror(EPERM));
    }

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

/**
   @return 返回调用者是否为前台的系统监视器程序
 */
bool SystemDBusServer::checkCaller() const
{
    if (!calledFromDBus()) {
        qCDebug(app) << "Not called from DBus";
        return false;
    }

    qint64 callerPid = dbusCallerPid();
    QString callerExe = getProcIdExe(callerPid);
    QString dbmExe = QStandardPaths::findExecutable("deepin-system-monitor", { "/usr/bin" });

    qCDebug(app) << "Checking caller authorization - PID:" << callerPid << "Executable:" << callerExe;
    if (callerExe != dbmExe) {
        qCWarning(app) << "Unauthorized caller:" << callerExe;
        return false;
    }

    qCDebug(app) << "Caller authorized:" << callerExe;
    return true;
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
    
    // 检查调用者授权
    if (!checkCaller()) {
        qCWarning(app) << "SystemServer: Unauthorized caller for isDKaptureAvailable";
        return false;
    }

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
    
    // 检查调用者授权
    if (!checkCaller()) {
        qCWarning(app) << "SystemServer: Unauthorized caller for getProcessInfoBatch";
        result["error"] = "Unauthorized";
        return result;
    }

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
                DKapture::PROC_PID_STATM    // 内存信息
                // DKapture::PROC_PID_traffic  // 网络流量信息 - 已禁用，前端使用传统方式
            };
            
            // 创建一个结构体来传递上下文
            struct DKaptureContext {
                QSet<int> targetPids;
                QVariantMap *processData;
                SystemDBusServer *server;
            } context;
            
            context.targetPids = QSet<int>(pids.begin(), pids.end());
            context.processData = &processData;
            context.server = this;
            
            // 设置回调函数来处理 DKapture 数据
            auto callback = [](void *ctx, const void *data, size_t data_sz) -> int {
                DKaptureContext *context = static_cast<DKaptureContext *>(ctx);
                const DKapture::DataHdr *hdr = static_cast<const DKapture::DataHdr *>(data);
                
                // qCDebug(app) << "SystemServer: DKapture callback called for PID" << hdr->pid << "type" << hdr->type;
                
                // 只处理我们关心的 PID
                if (!context->targetPids.contains(hdr->pid)) {
                    // qCDebug(app) << "SystemServer: Skipping PID" << hdr->pid << "(not in target list)";
                    return 0;
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
                        
                        // 恢复增量计算处理DKapture的累积CPU时间数据
                        qulonglong deltaUtime = context->server->calculateDelta(hdr->pid, "utime", stat->utime);
                        qulonglong deltaStime = context->server->calculateDelta(hdr->pid, "stime", stat->stime);
                        qulonglong deltaCutime = context->server->calculateDelta(hdr->pid, "cutime", stat->cutime);
                        qulonglong deltaCstime = context->server->calculateDelta(hdr->pid, "cstime", stat->cstime);
                        
                        // 使用新的转换公式：值 / (每秒纳秒数 * sysconf(_SC_CLK_TCK))
                        static const qulonglong NANOS_PER_SECOND = 1000000000ULL; // 1秒 = 10^9 纳秒
                        static const long CLK_TCK = sysconf(_SC_CLK_TCK);
                        static const qulonglong CONVERSION_FACTOR = NANOS_PER_SECOND * CLK_TCK;
                        
                        qulonglong utimeTicks = deltaUtime / CONVERSION_FACTOR;
                        qulonglong stimeTicks = deltaStime / CONVERSION_FACTOR;
                        qulonglong cutimeTicks = deltaCutime / CONVERSION_FACTOR;
                        qulonglong cstimeTicks = deltaCstime / CONVERSION_FACTOR;

                        // // 添加合理性检查，防止异常大的CPU值（例如超过1000 ticks认为是异常）
                        // static const qulonglong MAX_REASONABLE_CPU_TICKS = 1000;
                        // if (utimeTicks > MAX_REASONABLE_CPU_TICKS) utimeTicks = 0;
                        // if (stimeTicks > MAX_REASONABLE_CPU_TICKS) stimeTicks = 0;
                        // if (cutimeTicks > MAX_REASONABLE_CPU_TICKS) cutimeTicks = 0;
                        // if (cstimeTicks > MAX_REASONABLE_CPU_TICKS) cstimeTicks = 0;
                        
                        pidData["utime"] = utimeTicks;
                        pidData["stime"] = stimeTicks;
                        pidData["cutime"] = cutimeTicks;
                        pidData["cstime"] = cstimeTicks;
                        pidData["cpu_time"] = utimeTicks + stimeTicks;
                        

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
                        
                        // 应用增量计算处理DKapture的累积IO数据
                        qulonglong deltaReadBytes = context->server->calculateDelta(hdr->pid, "read_bytes", io->read_bytes);
                        qulonglong deltaWriteBytes = context->server->calculateDelta(hdr->pid, "write_bytes", io->write_bytes);
                        qulonglong deltaCancelledWriteBytes = context->server->calculateDelta(hdr->pid, "cancelled_write_bytes", io->cancelled_write_bytes);
                        
                        pidData["rchar"] = static_cast<qulonglong>(io->rchar);
                        pidData["wchar"] = static_cast<qulonglong>(io->wchar);
                        pidData["syscr"] = static_cast<qulonglong>(io->syscr);
                        pidData["syscw"] = static_cast<qulonglong>(io->syscw);
                        pidData["read_bytes"] = deltaReadBytes;
                        pidData["write_bytes"] = deltaWriteBytes;
                        pidData["cancelled_write_bytes"] = deltaCancelledWriteBytes;
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
                    case DKapture::PROC_PID_traffic: {
                        // 网络流量信息处理已禁用 - 前端将使用传统方式获取网络数据
                        // 这样可以避免DKapture系统级流量vs传统用户态socket流量的数据量差异问题
                        qCDebug(app) << "SystemServer: Skipping traffic data for PID" << hdr->pid << "- using traditional network monitoring";
                        break;
                    }
                    default:
                        break;
                }
                
                context->processData->insert(pidKey, pidData);
                // qCDebug(app) << "SystemServer: Updated PID" << hdr->pid << "final keys:" << pidData.keys();
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







#ifdef ENABLE_DKAPTURE

qulonglong SystemDBusServer::calculateDelta(int pid, const QString &field, qulonglong currentValue)
{
    QMutexLocker locker(&m_deltaDataMutex);
    
    ProcessDeltaData &lastData = m_processLastValues[pid];
    qulonglong delta = 0;
    qulonglong *lastValuePtr = nullptr;
    bool *firstTimePtr = nullptr;
    
    // 获取对应字段的上一次值指针和第一次标记指针
    if (field == "utime") {
        lastValuePtr = &lastData.utime;
        firstTimePtr = &lastData.utimeFirstTime;
    } else if (field == "stime") {
        lastValuePtr = &lastData.stime;
        firstTimePtr = &lastData.stimeFirstTime;
    } else if (field == "cutime") {
        lastValuePtr = &lastData.cutime;
        firstTimePtr = &lastData.cutimeFirstTime;
    } else if (field == "cstime") {
        lastValuePtr = &lastData.cstime;
        firstTimePtr = &lastData.cstimeFirstTime;
    // 网络字段已禁用 - 前端使用传统方式
    // } else if (field == "rbytes") {
    //     lastValuePtr = &lastData.rbytes;
    //     firstTimePtr = &lastData.rbytesFirstTime;
    // } else if (field == "wbytes") {
    //     lastValuePtr = &lastData.wbytes;
    //     firstTimePtr = &lastData.wbytesFirstTime;
    } else if (field == "read_bytes") {
        lastValuePtr = &lastData.read_bytes;
        firstTimePtr = &lastData.readBytesFirstTime;
    } else if (field == "write_bytes") {
        lastValuePtr = &lastData.write_bytes;
        firstTimePtr = &lastData.writeBytesFirstTime;
    } else if (field == "cancelled_write_bytes") {
        lastValuePtr = &lastData.cancelled_write_bytes;
        firstTimePtr = &lastData.cancelledWriteBytesFirstTime;
    }
    
    if (lastValuePtr && firstTimePtr) {
        if (*firstTimePtr) {
            // 第一次调用，不计算增量，只记录当前值
            delta = 0;
            *firstTimePtr = false;
            *lastValuePtr = currentValue;
            
            // 网络数据调试已禁用 - 前端使用传统方式
            // if ((field == "rbytes" || field == "wbytes") && pid == 4121171) {
            //     qCInfo(app) << "增量计算首次调用 PID" << pid << "field:" << field << "- 初始累积值:" << currentValue << "返回增量:" << delta;
            // }
        } else {
            // 非第一次调用，计算增量
            if (currentValue >= *lastValuePtr) {
                delta = currentValue - *lastValuePtr;
            } else {
                // 处理数值回绕或重置的情况 - 这种情况下返回0增量更安全
                // qCDebug(app) << "增量计算异常 PID" << pid << "field:" << field << "- current:" << currentValue << "< last:" << *lastValuePtr << "，返回0增量并重置基准";
                delta = 0;
            }
            
            // 网络数据调试已禁用 - 前端使用传统方式
            // if ((field == "rbytes" || field == "wbytes") && pid == 4121171) {
            //     qCInfo(app) << "增量计算 PID" << pid << "field:" << field << "- 上次:" << *lastValuePtr << "当前:" << currentValue << "增量:" << delta;
            // }
            
            *lastValuePtr = currentValue;
        }
    } else {
        qCWarning(app) << "calculateDelta: Unknown field" << field << "for PID" << pid;
    }
    
    return delta;
}

void SystemDBusServer::resetProcessDeltaData(int pid)
{
    QMutexLocker locker(&m_deltaDataMutex);
    m_processLastValues.remove(pid);
}

void SystemDBusServer::compareDKaptureWithProc(int pid, const ProcPidStatm* dkStatm, const ProcPidStat* dkStat)
{
    qCInfo(app) << "================ DKapture vs /proc Comparison for PID" << pid << "================";
    
    // 读取/proc文件
    QString statPath = QString("/proc/%1/stat").arg(pid);
    QString statmPath = QString("/proc/%1/statm").arg(pid);
    
    QFile statFile(statPath);
    QFile statmFile(statmPath);
    
    if (!statFile.open(QIODevice::ReadOnly) || !statmFile.open(QIODevice::ReadOnly)) {
        qCWarning(app) << "Failed to open /proc files for PID" << pid;
        return;
    }
    
    // 读取并解析/proc/pid/stat
    if (dkStat) {
        QString statContent = statFile.readAll().trimmed();
        QStringList statFields = statContent.split(' ');
        
        if (statFields.size() >= 24) {
            qulonglong procUtime = statFields[13].toULongLong();
            qulonglong procStime = statFields[14].toULongLong();
            qulonglong procVsize = statFields[22].toULongLong();
            qlonglong procRss = statFields[23].toLongLong();
            
            qCInfo(app) << "STAT comparison:";
            qCInfo(app) << "  DKapture utime:" << dkStat->utime << "  /proc utime:" << procUtime << "  diff:" << (qint64(dkStat->utime) - qint64(procUtime));
            qCInfo(app) << "  DKapture stime:" << dkStat->stime << "  /proc stime:" << procStime << "  diff:" << (qint64(dkStat->stime) - qint64(procStime));
            qCInfo(app) << "  DKapture vsize:" << dkStat->vsize << "  /proc vsize:" << procVsize << "  diff:" << (qint64(dkStat->vsize) - qint64(procVsize));
            qCInfo(app) << "  DKapture rss:" << dkStat->rss << "    /proc rss:" << (procRss * 4096) << "  diff:" << (qint64(dkStat->rss) - qint64(procRss * 4096));
        }
    }
    
    // 读取并解析/proc/pid/statm
    if (dkStatm) {
        QString statmContent = statmFile.readAll().trimmed();
        QStringList statmFields = statmContent.split(' ');
        
        if (statmFields.size() >= 3) {
            qulonglong procSize = statmFields[0].toULongLong();
            qulonglong procResident = statmFields[1].toULongLong();
            qulonglong procShared = statmFields[2].toULongLong();
            
            qCInfo(app) << "STATM comparison:";
            qCInfo(app) << "  DKapture size:" << dkStatm->size << "     /proc size:" << procSize << "     diff:" << (qint64(dkStatm->size) - qint64(procSize));
            qCInfo(app) << "  DKapture resident:" << dkStatm->resident << " /proc resident:" << procResident << " diff:" << (qint64(dkStatm->resident) - qint64(procResident));
            qCInfo(app) << "  DKapture shared:" << dkStatm->shared << "   /proc shared:" << procShared << "   diff:" << (qint64(dkStatm->shared) - qint64(procShared));
        }
    }
    
    qCInfo(app) << "================================================================";
}
#endif
