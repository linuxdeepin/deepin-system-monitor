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

#ifndef SETTINGHANDLER_H
#define SETTINGHANDLER_H

#include <DSettings>
#include <qsettingbackend.h>

#include <QObject>
#include <QMap>
#include <QPair>

DCORE_USE_NAMESPACE
DTK_USE_NAMESPACE

#define AlarmStatusOptionName   "setting.systemprotection.alarm_switch"
#define AlarmCpuUsageOptionName "setting.systemprotection.alarm_cpu_usage"
#define AlarmMemUsageOptionName "setting.systemprotection.alarm_memory_usage"
#define AlarmIntervalOptionName "setting.systemprotection.alarm_interval"

class SettingHandler : public QObject
{
    Q_OBJECT
public:
    explicit SettingHandler(QObject *parent = nullptr);
    ~SettingHandler();

signals:
public slots:

public:
    /*!
     * 数据加载完成
     */
    bool isCompelted();
    /*!
     * 获取设置项值
     */
    QVariant getOptionValue(const QString key);
    /*!
     * 设定设置项值
     */
    bool changedOptionValue(const QString key, const QVariant value);
    /*!
     * 判断此值是否是有效值
     */
    bool isVaildValue(const QString key, const QVariant value);
    /*!
     * 获取有效值范围
     */
    QPair<double,double> getValueRange(const QString key);
    QList<QString> itemKeys();

private:
    DSettings *mSettings;
    Dtk::Core::QSettingBackend *mBackend;
    QMap<QString, QPair<double,double>> mValueRange;
};

#endif // SETTINGHANDLER_H
