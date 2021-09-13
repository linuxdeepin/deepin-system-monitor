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

#ifndef CPUPROFILE_H
#define CPUPROFILE_H

#include <QObject>
#include <QMap>

class CpuProfile : public QObject
{
    Q_OBJECT
public:
    explicit CpuProfile(QObject *parent = nullptr);

signals:

public slots:

public:
    /*!
     * 更新CPU占用率
     */
    double updateSystemCpuUsage();
    /*!
     * 获取CPU占用率
     */
    double getCpuUsage();
    /*!
     * 获取当前CPU状态
     */
    QMap<QString,int> cpuStat();

private:
    QMap<QString,int> mLastCpuStat;
    double mCpuUsage;
};

#endif // CPUPROFILE_H
