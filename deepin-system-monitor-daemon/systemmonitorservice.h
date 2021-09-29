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

#ifndef SYSTEMMONITORSERVICE_H
#define SYSTEMMONITORSERVICE_H

#include "settinghandler.h"
#include "cpuprofile.h"
#include "memoryprofile.h"

#include <DSettings>
#include <qsettingbackend.h>

#include <QObject>
#include <QDBusContext>
#include <QDBusVariant>
#include <QDBusAbstractAdaptor>
#include <QTimer>

#define DAEMON_DBUS_NAME "com.deepin.SystemMonitor.Daemon"
#define DAEMON_DBUS_PATH "/com/deepin/SystemMonitor"

DCORE_USE_NAMESPACE
DTK_USE_NAMESPACE

class SystemMonitorService : public QObject, QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", DAEMON_DBUS_NAME)
public:
    explicit SystemMonitorService(QObject *parent = nullptr);
    ~SystemMonitorService();
    /*!
     * DBus Adaptor接口: 获取检测开关状态
     */
    bool getSystemProtectionStatus();
    /*!
     * DBus Adaptor接口: 设置检测开关状态
     */
    void setSystemProtectionStatus(bool status);
    /*!
     * DBus Adaptor接口: 获取当前Cpu占用率
     */
    int getCpuUsage();
    /*!
     * DBus Adaptor接口: 获取当前Memory占用率
     */
    int getMemoryUsage();
    /*!
     * DBus Adaptor接口: 获取当前报警信息间隔时间(minute)
     */
    int getAlarmMsgInterval();
    /*!
     * DBus Adaptor接口: 设置当前报警信息间隔时间(minute)
     */
    void setAlarmMsgInterval(int interval);


    /*!
     * \brief getAlaramLastTimeInterval 获取上次告警时间
     * \return 上次告警时间
     */
    qint64 getAlaramLastTimeInterval();
    /*!
     * \brief setAlaramLastTimeInterval 设置上次告警时间
     * \param lastTime 设置的参数值
     */
    void setAlaramLastTimeInterval(qint64 lastTime);


    /*!
     * DBus Adaptor接口: 获取当前监测报警Cpu占用率(%)
     */
    int getAlarmUsageOfCpu();
    /*!
     * DBus Adaptor接口: 设置当前监测报警Cpu占用率(%)
     */
    void setAlarmUsageOfCpu(int usage);
    /*!
     * DBus Adaptor接口: 获取当前监测报警Memory占用率(%)
     */
    int getAlarmUsageOfMemory();
    /*!
     * DBus Adaptor接口: 设置当前监测报警Memory占用率(%)
     */
    void setAlarmUsageOfMemory(int usage);
    /*!
     * DBus Adaptor接口: 显示系统监视器主页面
     */
    void showDeepinSystemMoniter();
    /*!
     * DBus Adaptor接口: 修改监控设置
     */
    void changeAlarmItem(const QString &item, const QDBusVariant &value);
    /*!
     * 检查是否触发Cpu报警
     */
    bool checkCpuAlarm();
    /*!
     * 检查是否触发Memory报警
     */
    bool checkMemoryAlarm();
    /*!
     * 显示系统通知
     */
    bool showAlarmNotify(QString topic, QString msg, int timeout = 0);

signals:
    /*!
     * 监测设置变更信号
     */
    void alarmItemChanged(const QString &item, const QDBusVariant &value);

public slots:
    /*!
     * 监测由此计时器槽处理
     */
    void onMonitorTimeout();

private:
    /*!
     * DBbus Adaptor
     */
    QDBusAbstractAdaptor *mAdaptor;
    /*!
     * setting值的副本
     */
    bool mProtectionStatus;
    int mAlarmInterval;
    int mAlarmCpuUsage;
    int mAlarmMemoryUsage;
    /*!
     * 当前CPU和内存占用率(%)
     */
    int mCpuUsage;
    int mMemoryUsage;
    /*!
     * 监测计时器及时间戳
     */
    QTimer mMoniterTimer;
    qint64 mLastAlarmTimeStamp;
    /*!
     * 设置数据类
     */
    SettingHandler mSettings;
    /*!
     * Cpu数据获取类
     */
    CpuProfile mCpu;
    /*!
     * Memory数据获取类
     */
    MemoryProfile mMem;
};

#endif // SYSTEMMONITORSERVICE_H
