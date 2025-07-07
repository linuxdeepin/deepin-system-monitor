// SPDX-FileCopyrightText: 2011 ~ 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemmonitorservice.h"
#include "ddlog.h"
#include <DSettingsOption>
#include <QDBusInterface>

#include <QDebug>
#include <QProcess>
#include <QDBusVariant>
#include <QFile>
#include <QDBusConnectionInterface>
#include <QDateTime>

using namespace DDLog;

// 打印DBus调用者信息
#define PrintDBusCaller()                                                                                   \
    {                                                                                                       \
        if (calledFromDBus()) {                                                                             \
            QDBusConnection conn = connection();                                                            \
            QDBusMessage msg = message();                                                                   \
            pid_t callerPid = static_cast<pid_t>(conn.interface()->servicePid(msg.service()).value());      \
            qCDebug(app) << "DBus service caller:" << conn.interface()->serviceOwner(msg.service()).value() \
                         << ",Uid:" << conn.interface()->serviceUid(msg.service()).value()                  \
                         << ",Pid:" << callerPid                                                            \
                         << ",Process name:" << getNameByPid(callerPid);                                    \
        }                                                                                                   \
    }

// 通过PID获取进程名称
QString getNameByPid(pid_t pid)
{
    // qCDebug(app) << "getNameByPid with pid:" << pid;
    QString name;
    QFile file(QString("/proc/%1/cmdline").arg(pid));
    if (file.open(QFile::ReadOnly)) {
        name = file.readLine();
        file.close();
        // qCDebug(app) << "get process name success, name is" << name;
    }
    return name;
}

#define InitAlarmCpuUsage 90
#define InitAlarmMemUsage 90
#define InitAlarmInterval 10
#define InitAlarmOn false
#define MonitorTimeOut 1000
#define AlarmMessageTimeOut 10000

SystemMonitorService::SystemMonitorService(const char *name, QObject *parent)
    : QObject(parent), mProtectionStatus(InitAlarmOn), mAlarmInterval(InitAlarmInterval), mAlarmCpuUsage(InitAlarmCpuUsage), mAlarmMemoryUsage(InitAlarmMemUsage), mCpuUsage(0), mMemoryUsage(0), mMoniterTimer(this)
      //    , mLastAlarmTimeStamp(0)
      ,
      mSettings(this),
      mCpu(this),
      mMem(this)
{
    qCDebug(app) << "SystemMonitorService constructor";
    if (mSettings.isCompelted()) {
        qCDebug(app) << "Loading settings from configuration";
        mProtectionStatus = mSettings.getOptionValue(AlarmStatusOptionName).toBool();
        mAlarmCpuUsage = mSettings.getOptionValue(AlarmCpuUsageOptionName).toInt();
        mAlarmMemoryUsage = mSettings.getOptionValue(AlarmMemUsageOptionName).toInt();
        mAlarmInterval = mSettings.getOptionValue(AlarmIntervalOptionName).toInt();
        mLastAlarmTimeStamp = mSettings.getOptionValue(AlarmLastTimeOptionName).toLongLong();
        qCDebug(app) << "Loaded settings - Protection:" << mProtectionStatus 
                     << "CPU Alarm:" << mAlarmCpuUsage 
                     << "Memory Alarm:" << mAlarmMemoryUsage 
                     << "Interval:" << mAlarmInterval;
    } else {
        qCWarning(app) << "Failed to load settings, using default values";
    }

    // 初始化Cpu占用率
    mCpuUsage = static_cast<int>(mCpu.updateSystemCpuUsage());
    // 初始化Memory占用率
    mMemoryUsage = static_cast<int>(mMem.updateSystemMemoryUsage());
    qCDebug(app) << "Initial system state - CPU:" << mCpuUsage << "% Memory:" << mMemoryUsage << "%";

    // 从配置文件，初始化： mProtectionStatus mAlarmInterval mAlarmCpuUsage mAlarmMemoryUsage
    mMoniterTimer.setInterval(MonitorTimeOut);
    connect(&mMoniterTimer, &QTimer::timeout, this, &SystemMonitorService::onMonitorTimeout);

    // 启动监测定时器
    if (mProtectionStatus) {
        mMoniterTimer.start();
    }

    qCInfo(app) << "Started monitoring timer with interval:" << MonitorTimeOut << "ms";

    QDBusConnection::RegisterOptions opts =
            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties;

    QDBusConnection::connectToBus(QDBusConnection::SessionBus, QString(name))
            .registerObject("/org/deepin/SystemMonitorDaemon", this, opts);
    qCInfo(app) << "Registered DBus object at /org/deepin/SystemMonitorDaemon";
}

SystemMonitorService::~SystemMonitorService()
{
}

bool SystemMonitorService::getSystemProtectionStatus()
{
    PrintDBusCaller()
    qCDebug(app) << "Get Protection Status:" << mProtectionStatus;
    return mProtectionStatus;
}

void SystemMonitorService::setSystemProtectionStatus(bool status)
{
    PrintDBusCaller()
    qCDebug(app) << "Set Protection Status:" << status;

    if (mProtectionStatus != status) {
        mProtectionStatus = status;
        // 更改设置文件
        mSettings.changedOptionValue(AlarmStatusOptionName, mProtectionStatus);
        // 监测设置变更，DBus信号
        emit alarmItemChanged(AlarmStatusOptionName, QDBusVariant(mProtectionStatus));
        qCInfo(app) << "System protection status changed to:" << status;
    }
}

int SystemMonitorService::getCpuUsage()
{
    PrintDBusCaller()
    qCDebug(app) << "Get CPU Usage:" << mCpuUsage << "%";
    return mCpuUsage;
}

int SystemMonitorService::getMemoryUsage()
{
    PrintDBusCaller()
    qCDebug(app) << "Get Memory Usage:" << mMemoryUsage << "%";
    return mMemoryUsage;
}

int SystemMonitorService::getAlarmMsgInterval()
{
    PrintDBusCaller()
    qCDebug(app) << "Get Alarm Message Interval:" << mAlarmInterval << "minutes";
    return mAlarmInterval;
}

void SystemMonitorService::setAlarmMsgInterval(int interval)
{
    PrintDBusCaller()
    qCDebug(app) << "Set Alarm Message Interval:" << interval << "minutes";

    // 根据需求
    if (mSettings.isVaildValue(AlarmIntervalOptionName, interval)) {
        mAlarmInterval = interval;
        // 更改设置文件
        mSettings.changedOptionValue(AlarmIntervalOptionName, mAlarmInterval);
        // 监测设置变更，DBus信号
        emit alarmItemChanged(AlarmIntervalOptionName, QDBusVariant(mAlarmInterval));
        qCInfo(app) << "Alarm message interval changed to:" << interval << "minutes";
    } else {
        QPair<double, double> range = mSettings.getValueRange(AlarmIntervalOptionName);
        qCWarning(app) << "Invalid alarm interval value:" << interval << "Valid range:" << range;
        sendErrorReply(QDBusError::NotSupported, QString("invalid value! value range[%1,%2]").arg(range.first).arg(range.second));
    }
}

qint64 SystemMonitorService::getAlaramLastTimeInterval()
{
    PrintDBusCaller()
    qCDebug(app) << "Get Last Alarm Time:" << mLastAlarmTimeStamp;
    return mLastAlarmTimeStamp;
}

void SystemMonitorService::setAlaramLastTimeInterval(qint64 lastTime)
{
    PrintDBusCaller()
    qCDebug(app) << "Set Last Alarm Time:" << lastTime;

    // 根据需求
    if (mSettings.isVaildValue(AlarmLastTimeOptionName, lastTime)) {
        mLastAlarmTimeStamp = lastTime;
        // 更改设置文件
        mSettings.changedOptionValue(AlarmLastTimeOptionName, mLastAlarmTimeStamp);
        // 监测设置变更，DBus信号
        emit alarmItemChanged(AlarmLastTimeOptionName, QDBusVariant(mLastAlarmTimeStamp));
        qCInfo(app) << "Last alarm time updated to:" << lastTime;
    } else {
        QPair<double, double> range = mSettings.getValueRange(AlarmLastTimeOptionName);
        qCWarning(app) << "Invalid last alarm time value:" << lastTime << "Valid range:" << range;
        sendErrorReply(QDBusError::NotSupported, QString("invalid value! value range[%1,%2]").arg(range.first).arg(range.second));
    }
}

int SystemMonitorService::getAlarmUsageOfCpu()
{
    PrintDBusCaller()
    qCDebug(app) << "Get CPU Alarm Threshold:" << mAlarmCpuUsage << "%";
    return mAlarmCpuUsage;
}

void SystemMonitorService::setAlarmUsageOfCpu(int usage)
{
    PrintDBusCaller()
    qCDebug(app) << "Set CPU Alarm Threshold:" << usage << "%";

    // 根据需求
    if (mSettings.isVaildValue(AlarmCpuUsageOptionName, usage)) {
        mAlarmCpuUsage = usage;
        // 更改设置文件
        mSettings.changedOptionValue(AlarmCpuUsageOptionName, mAlarmCpuUsage);
        // 监测设置变更，DBus信号
        emit alarmItemChanged(AlarmCpuUsageOptionName, QDBusVariant(mAlarmCpuUsage));
        qCInfo(app) << "CPU alarm threshold changed to:" << usage << "%";
    } else {
        QPair<double, double> range = mSettings.getValueRange(AlarmCpuUsageOptionName);
        qCWarning(app) << "Invalid CPU alarm threshold:" << usage << "Valid range:" << range;
        sendErrorReply(QDBusError::NotSupported, QString("invalid value! value range[%1,%2]").arg(range.first).arg(range.second));
    }
}

int SystemMonitorService::getAlarmUsageOfMemory()
{
    PrintDBusCaller()
    qCDebug(app) << "Get Memory Alarm Threshold:" << mAlarmMemoryUsage << "%";
    return mAlarmMemoryUsage;
}

void SystemMonitorService::setAlarmUsageOfMemory(int usage)
{
    PrintDBusCaller()
    qCDebug(app) << "Set Memory Alarm Threshold:" << usage << "%";

    // 根据需求
    if (mSettings.isVaildValue(AlarmMemUsageOptionName, usage)) {
        mAlarmMemoryUsage = usage;
        // 更改设置文件
        mSettings.changedOptionValue(AlarmMemUsageOptionName, mAlarmMemoryUsage);
        // 监测设置变更，DBus信号
        emit alarmItemChanged(AlarmMemUsageOptionName, QDBusVariant(mAlarmMemoryUsage));
        qCInfo(app) << "Memory alarm threshold changed to:" << usage << "%";
    } else {
        QPair<double, double> range = mSettings.getValueRange(AlarmMemUsageOptionName);
        qCWarning(app) << "Invalid memory alarm threshold:" << usage << "Valid range:" << range;
        sendErrorReply(QDBusError::NotSupported, QString("invalid value! value range[%1,%2]").arg(range.first).arg(range.second));
    }
}

void SystemMonitorService::showDeepinSystemMoniter()
{
    PrintDBusCaller()
    qCDebug(app) << "showDeepinSystemMoniter";

    QString cmd("gdbus call -e -d  com.deepin.SystemMonitorServer -o /com/deepin/SystemMonitorServer -m com.deepin.SystemMonitorServer.showDeepinSystemMoniter");
    QTimer::singleShot(100, this, [=]() { QProcess::startDetached(cmd); });
}

void SystemMonitorService::changeAlarmItem(const QString &item, const QDBusVariant &value)
{
    qCDebug(app) << "Changing alarm item:" << item << "to value:" << value.variant();
    if (mSettings.itemKeys().contains(item) && mSettings.isVaildValue(item, value.variant())) {
        if (mSettings.isVaildValue(item, value.variant())) {
            qCDebug(app) << "value is vaild";
            if (item == AlarmStatusOptionName) {
                mProtectionStatus = value.variant().toBool();
                if (mProtectionStatus) {
                    qCDebug(app) << "mProtectionStatus value:" << mProtectionStatus;
                    mMoniterTimer.start();
                }
            } else if (item == AlarmCpuUsageOptionName) {
                mAlarmCpuUsage = value.variant().toInt();
            } else if (item == AlarmMemUsageOptionName) {
                mAlarmMemoryUsage = value.variant().toInt();
            } else if (item == AlarmIntervalOptionName) {
                mAlarmInterval = value.variant().toInt();
            }
            // 更改设置文件
            mSettings.changedOptionValue(item, value.variant());
            qCInfo(app) << "Successfully changed alarm item:" << item << "to:" << value.variant();
        } else {
            QPair<double, double> range = mSettings.getValueRange(item);
            qCWarning(app) << "Invalid value for" << item << ":" << value.variant() << "Valid range:" << range;
            sendErrorReply(QDBusError::NotSupported, QString("invalid value! value range[%1,%2]").arg(range.first).arg(range.second));
        }
    } else {
        qCWarning(app) << "Invalid alarm item:" << item;
        sendErrorReply(QDBusError::NotSupported, QString("invalid item(%1) !").arg(item));
    }
}

bool SystemMonitorService::checkCpuAlarm()
{
    qCDebug(app) << "checkCpuAlarm";
    qint64 curTimeStamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    qint64 diffTime = curTimeStamp - mLastAlarmTimeStamp;
    qint64 timeGap = 1000 * 60 * mAlarmInterval;

    if (mCpuUsage >= mAlarmCpuUsage && diffTime >= timeGap) {
        qCInfo(app) << "CPU usage alarm triggered - Usage:" << mCpuUsage << "% Threshold:" << mAlarmCpuUsage << "%";
        mLastAlarmTimeStamp = curTimeStamp;
        QString cmd = QString("gdbus call -e -d  com.deepin.SystemMonitorServer -o /com/deepin/SystemMonitorServer -m com.deepin.SystemMonitorServer.showCpuAlarmNotify \"%1\" ").arg(QString::number(mCpuUsage));
        QTimer::singleShot(100, this, [=]() {
            qCDebug(app) << "showCpuAlarmNotify";
            QStringList args;
            args << "call" << "-e" << "-d" << "com.deepin.SystemMonitorServer"
                 << "-o" << "/com/deepin/SystemMonitorServer"
                 << "-m" << "com.deepin.SystemMonitorServer.showCpuAlarmNotify"
                 << QString::number(mCpuUsage);
            QProcess process;
            process.start("gdbus", args);
            process.waitForFinished(5000);
            if (process.exitCode() != 0) {
                qCWarning(app) << "Failed to show CPU alarm notification, retrying...";
                process.start("gdbus", args);
                process.waitForFinished(5000);
            }
        });
    }

    return false;
}

bool SystemMonitorService::checkMemoryAlarm()
{
    qCDebug(app) << "checkMemoryAlarm";
    qint64 curTimeStamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    qint64 diffTime = curTimeStamp - mLastAlarmTimeStamp;
    qint64 timeGap = 1000 * 60 * mAlarmInterval;

    if (mMemoryUsage >= mAlarmMemoryUsage && diffTime > timeGap) {
        qCInfo(app) << "Memory usage alarm triggered - Usage:" << mMemoryUsage << "% Threshold:" << mAlarmMemoryUsage << "%";
        mLastAlarmTimeStamp = curTimeStamp;
        QString cmd = QString("gdbus call -e -d  com.deepin.SystemMonitorServer -o /com/deepin/SystemMonitorServer -m com.deepin.SystemMonitorServer.showMemoryAlarmNotify \"%1\" ").arg(QString::number(mMemoryUsage));
        QTimer::singleShot(100, this, [=]() {
            qCDebug(app) << "showMemoryAlarmNotify";
            QStringList args;
            args << "call" << "-e" << "-d" << "com.deepin.SystemMonitorServer"
                 << "-o" << "/com/deepin/SystemMonitorServer"
                 << "-m" << "com.deepin.SystemMonitorServer.showMemoryAlarmNotify"
                 << QString::number(mMemoryUsage);
            QProcess process;
            process.start("gdbus", args);
            process.waitForFinished(5000);
            if (process.exitCode() != 0) {
                qCWarning(app) << "Failed to show memory alarm notification, retrying...";
                process.start("gdbus", args);
                process.waitForFinished(5000);
            }
        });
    }

    return false;
}

void SystemMonitorService::onMonitorTimeout()
{
    qCDebug(app) << "onMonitorTimeout";
    // 获取CPU和内存占用
    mCpuUsage = static_cast<int>(mCpu.updateSystemCpuUsage());
    mMemoryUsage = static_cast<int>(mMem.updateSystemMemoryUsage());
    qCDebug(app) << "System state updated - CPU:" << mCpuUsage << "% Memory:" << mMemoryUsage << "%";

    // 进行警报检测
    if (mProtectionStatus) {
        qCDebug(app) << "Protection status is enabled. Checking alarms...";
        checkCpuAlarm();
        checkMemoryAlarm();
    }
}
