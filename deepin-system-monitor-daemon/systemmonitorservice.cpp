/*
* Copyright (C) 2011 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      wangchao <wangchao@uniontech.com>
* Maintainer:  wangchao <wangchao@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "systemmonitorservice.h"
#include "daemonadaptor.h"

#include <DSettingsOption>
#include <QDBusInterface>

#include <QDebug>
#include <QProcess>
#include <QDBusVariant>

// 打印DBus调用者信息
#define PrintDBusCaller() { \
        if(calledFromDBus()) { \
            QDBusConnection conn = connection(); \
            QDBusMessage msg = message(); \
            pid_t callerPid = static_cast<pid_t>(conn.interface()->servicePid(msg.service()).value()); \
            qDebug() << "调用者DBus服务:" << conn.interface()->serviceOwner(msg.service()).value() \
                     << ",Uid:" << conn.interface()->serviceUid(msg.service()).value() \
                     << ",Pid:" << callerPid \
                     << ",进程名:" << getNameByPid(callerPid); } }

// 通过PID获取进程名称
QString getNameByPid(pid_t pid)
{
    QString name;
    QFile file(QString("/proc/%1/cmdline").arg(pid));
    if (file.open(QFile::ReadOnly)) {
        name = file.readLine();
        file.close();
    }
    return name;
}

#define InitAlarmCpuUsage 90
#define InitAlarmMemUsage 90
#define InitAlarmInterval 10
#define InitAlarmOn false
#define MonitorTimeOut 1000
#define AlarmMessageTimeOut 10000

SystemMonitorService::SystemMonitorService(QObject *parent)
    : QObject (parent)
    , mAdaptor(new DaemonAdaptor(this))
    , mProtectionStatus(InitAlarmOn)
    , mAlarmInterval(InitAlarmInterval)
    , mAlarmCpuUsage(InitAlarmCpuUsage)
    , mAlarmMemoryUsage(InitAlarmMemUsage)
    , mCpuUsage(0)
    , mMemoryUsage(0)
    , mMoniterTimer(this)
//    , mLastAlarmTimeStamp(0)
    , mSettings(this)
    , mCpu(this)
    , mMem(this)
{
    if(mAdaptor == nullptr) {
        qWarning() << __FUNCTION__ << __LINE__ << " DaemonAdaptor create fail!";
        exit(1);
    }

    if(mSettings.isCompelted()) {
        mProtectionStatus = mSettings.getOptionValue(AlarmStatusOptionName).toBool();
        mAlarmCpuUsage = mSettings.getOptionValue(AlarmCpuUsageOptionName).toInt();
        mAlarmMemoryUsage = mSettings.getOptionValue(AlarmMemUsageOptionName).toInt();
        mAlarmInterval = mSettings.getOptionValue(AlarmIntervalOptionName).toInt();
        mLastAlarmTimeStamp = mSettings.getOptionValue(AlarmLastTimeOptionName).toLongLong();

    }

    qDebug() << ",初始化的数据："  << endl
             << AlarmStatusOptionName << "=" << mProtectionStatus << endl
             << AlarmCpuUsageOptionName << "=" << mAlarmCpuUsage << endl
             << AlarmMemUsageOptionName << "=" << mAlarmMemoryUsage << endl
             << AlarmIntervalOptionName << "=" << mAlarmInterval << endl
             << AlarmLastTimeOptionName << "=" << mLastAlarmTimeStamp << endl;

    // 初始化Cpu占用率
    mCpuUsage = static_cast<int>(mCpu.updateSystemCpuUsage());
    // 初始化Memory占用率
    mMemoryUsage = static_cast<int>(mMem.updateSystemMemoryUsage());

    // 从配置文件，初始化： mProtectionStatus mAlarmInterval mAlarmCpuUsage mAlarmMemoryUsage
    mMoniterTimer.setInterval(MonitorTimeOut);
    connect(&mMoniterTimer, &QTimer::timeout, this, &SystemMonitorService::onMonitorTimeout);

    // 启动监测定时器
    mMoniterTimer.start();
}

SystemMonitorService::~SystemMonitorService()
{
}

bool SystemMonitorService::getSystemProtectionStatus()
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Get Protection Status:" << mProtectionStatus;
    return mProtectionStatus;
}

void SystemMonitorService::setSystemProtectionStatus(bool status)
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Set Protection Status:" << status;

    if(mProtectionStatus != status) {
        mProtectionStatus = status;
        // 更改设置文件
        mSettings.changedOptionValue(AlarmStatusOptionName, mProtectionStatus);
        // 监测设置变更，DBus信号
        emit alarmItemChanged(AlarmStatusOptionName, QDBusVariant(mProtectionStatus));
    }
}

int SystemMonitorService::getCpuUsage()
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Get Cpu Usage:" << mCpuUsage;
    return mCpuUsage;
}

int SystemMonitorService::getMemoryUsage()
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Get Memory Usage:" << mMemoryUsage;
    return mMemoryUsage;
}

int SystemMonitorService::getAlarmMsgInterval()
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Get Alarm Msg Interval:" << mAlarmInterval;
    return mAlarmInterval;
}

void SystemMonitorService::setAlarmMsgInterval(int interval)
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Set Alarm Msg Interval:" << interval;

    // 根据需求
    if(mSettings.isVaildValue(AlarmIntervalOptionName, interval)) {
        mAlarmInterval = interval;
        // 更改设置文件
        mSettings.changedOptionValue(AlarmIntervalOptionName, mAlarmInterval);
        // 监测设置变更，DBus信号
        emit alarmItemChanged(AlarmIntervalOptionName, QDBusVariant(mAlarmInterval));
    } else {
        QPair<double, double> range = mSettings.getValueRange(AlarmIntervalOptionName);
        sendErrorReply(QDBusError::NotSupported, QString("invalid value! value range[%1,%2]").arg(range.first).arg(range.second));
    }
}

qint64 SystemMonitorService::getAlaramLastTimeInterval()
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Get Alarm Last Time:" << mLastAlarmTimeStamp;
    return mLastAlarmTimeStamp;
}

void SystemMonitorService::setAlaramLastTimeInterval(qint64 lastTime)
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Set Alarm Last Time:" << lastTime;

    // 根据需求
    if(mSettings.isVaildValue(AlarmLastTimeOptionName, lastTime)) {
        mLastAlarmTimeStamp = lastTime;
        // 更改设置文件
        mSettings.changedOptionValue(AlarmLastTimeOptionName, mLastAlarmTimeStamp);
        // 监测设置变更，DBus信号
        emit alarmItemChanged(AlarmLastTimeOptionName, QDBusVariant(mLastAlarmTimeStamp));
    } else {
        QPair<double, double> range = mSettings.getValueRange(AlarmLastTimeOptionName);
        sendErrorReply(QDBusError::NotSupported, QString("invalid value! value range[%1,%2]").arg(range.first).arg(range.second));
    }
}

int SystemMonitorService::getAlarmUsageOfCpu()
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Get Alarm Usage Of Cpu:" << mAlarmCpuUsage;
    return mAlarmCpuUsage;
}

void SystemMonitorService::setAlarmUsageOfCpu(int usage)
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Set Alarm Usage Of Cpu:" << usage;

    // 根据需求
    if(mSettings.isVaildValue(AlarmCpuUsageOptionName, usage)) {
        mAlarmCpuUsage = usage;
        // 更改设置文件
        mSettings.changedOptionValue(AlarmCpuUsageOptionName, mAlarmCpuUsage);
        // 监测设置变更，DBus信号
        emit alarmItemChanged(AlarmCpuUsageOptionName, QDBusVariant(mAlarmCpuUsage));
    } else {
        QPair<double, double> range = mSettings.getValueRange(AlarmCpuUsageOptionName);
        sendErrorReply(QDBusError::NotSupported, QString("invalid value! value range[%1,%2]").arg(range.first).arg(range.second));
    }
}

int SystemMonitorService::getAlarmUsageOfMemory()
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Get Alarm Usage Of Memory:" << mAlarmMemoryUsage;
    return mAlarmMemoryUsage;
}

void SystemMonitorService::setAlarmUsageOfMemory(int usage)
{
    PrintDBusCaller()
    qDebug() << __FUNCTION__ << __LINE__ << " Set Alarm Usage Of Memory:" << usage;

    // 根据需求
    if(mSettings.isVaildValue(AlarmMemUsageOptionName, usage)) {
        mAlarmMemoryUsage = usage;
        // 更改设置文件
        mSettings.changedOptionValue(AlarmMemUsageOptionName, mAlarmMemoryUsage);
        // 监测设置变更，DBus信号
        emit alarmItemChanged(AlarmMemUsageOptionName, QDBusVariant(mAlarmMemoryUsage));
    } else {
        QPair<double, double> range = mSettings.getValueRange(AlarmMemUsageOptionName);
        sendErrorReply(QDBusError::NotSupported, QString("invalid value! value range[%1,%2]").arg(range.first).arg(range.second));
    }
}

void SystemMonitorService::showDeepinSystemMoniter()
{
    PrintDBusCaller()
    // 显示系统监视器
    QProcess::startDetached("/usr/bin/deepin-system-monitor");
    QString cmd("qdbus com.deepin.SystemMonitorMain /com/deepin/SystemMonitorMain com.deepin.SystemMonitorMain.slotRaiseWindow");
    QTimer::singleShot(100, this, [=] () { QProcess::startDetached(cmd); });
}

void SystemMonitorService::changeAlarmItem(const QString &item, const QDBusVariant &value)
{
    qDebug() << __FUNCTION__ << __LINE__ << ", item:" << item << ", value:" << value.variant();
    if(mSettings.itemKeys().contains(item) && mSettings.isVaildValue(item, value.variant())) {
        if(mSettings.isVaildValue(item, value.variant())) {
            if(item == AlarmStatusOptionName) {
                mProtectionStatus = value.variant().toBool();
            } else if (item == AlarmCpuUsageOptionName) {
                mAlarmCpuUsage = value.variant().toInt();
            } else if (item == AlarmMemUsageOptionName) {
                mAlarmMemoryUsage = value.variant().toInt();
            } else if (item == AlarmIntervalOptionName) {
                mAlarmInterval = value.variant().toInt();
            }
            // 更改设置文件
            mSettings.changedOptionValue(item, value.variant());
        } else {
            QPair<double, double> range = mSettings.getValueRange(item);
            sendErrorReply(QDBusError::NotSupported, QString("invalid value! value range[%1,%2]").arg(range.first).arg(range.second));
        }
    } else {
        sendErrorReply(QDBusError::NotSupported, QString("invalid item(%1) !").arg(item));
    }
}

bool SystemMonitorService::checkCpuAlarm()
{
    qint64 curTimeStamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    qint64 diffTime = curTimeStamp - mLastAlarmTimeStamp;
    qint64 timeGap = 1000 * 60 * mAlarmInterval;

    if(mCpuUsage >= mAlarmCpuUsage && diffTime >= timeGap) {
        // 构造消息内容
        QString topic(tr("Warning"));
        QString msg = QString(tr("Your CPU usage is higher than %1%!")).arg(mCpuUsage);
        int timeout = AlarmMessageTimeOut;
        return showAlarmNotify(topic, msg, timeout);
    }

    return false;
}

bool SystemMonitorService::checkMemoryAlarm()
{
    qint64 curTimeStamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    qint64 diffTime = curTimeStamp - mLastAlarmTimeStamp;
    qint64 timeGap = 1000 * 60 * mAlarmInterval;

    if(mMemoryUsage >= mAlarmMemoryUsage && diffTime > timeGap) {
        // 构造消息内容
        QString topic(tr("Warning"));
        QString msg = QString(tr("Your memory usage is higher than %1%!")).arg(mMemoryUsage);
        int timeout = AlarmMessageTimeOut;
        return showAlarmNotify(topic, msg, timeout);
    }

    return false;
}

bool SystemMonitorService::showAlarmNotify(QString topic, QString msg, int timeout)
{
    QDBusMessage ddeNotify = QDBusMessage::createMethodCall("com.deepin.dde.Notification",
                                                    "/com/deepin/dde/Notification",
                                                    "com.deepin.dde.Notification",
                                                    "Notify");
    QStringList action;
    action << "_open1" << tr("View"); //添加按钮
    QVariantMap inform; //按钮的点击操作
    // 操作打开系统监视器
    QString openSystemMonitor = QString("qdbus,com.deepin.SystemMonitor.Daemon,"
                                           "/com/deepin/SystemMonitor,"
                                           "com.deepin.SystemMonitor.Daemon.showDeepinSystemMoniter");

    inform.insert(QString("x-deepin-action-_open1"), openSystemMonitor);

    QList<QVariant> ddeArgs;
    ddeArgs << QString("deepin-system-monitor"); // app name
    ddeArgs << uint(0);                          // id = 0 不指定窗口 id
    ddeArgs << QString("deepin-system-monitor"); // icon
    ddeArgs << topic;                            // notify topic
    ddeArgs << msg;                              // notify msg body
    ddeArgs << action;                           // button
    ddeArgs << inform;                           // button operation
    ddeArgs << timeout;                          // notify timeout

    ddeNotify.setArguments(ddeArgs);

    QDBusMessage replyMsg = QDBusConnection::sessionBus().call(ddeNotify);

    if(replyMsg.type() == QDBusMessage::ErrorMessage) {
        qWarning() << __FUNCTION__ << __LINE__ << ", dde notify dbus method call fail , error name :"
                   << replyMsg.errorName() << " , error msg :" << replyMsg.errorMessage();
        return false;
    } else {
        mLastAlarmTimeStamp =  QDateTime::currentDateTime().toMSecsSinceEpoch();
        setAlaramLastTimeInterval(mLastAlarmTimeStamp);
    }

    return true;
}

void SystemMonitorService::onMonitorTimeout()
{
    // 获取CPU和内存占用
    mCpuUsage = static_cast<int>(mCpu.updateSystemCpuUsage());
    mMemoryUsage = static_cast<int>(mMem.updateSystemMemoryUsage());

    // 进行警报检测
    if(mProtectionStatus) {
        checkCpuAlarm();
        checkMemoryAlarm();
    }
}
