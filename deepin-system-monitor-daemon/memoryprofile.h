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

#ifndef MEMORYPROFILE_H
#define MEMORYPROFILE_H

#include <QObject>

class MemoryProfile : public QObject
{
    Q_OBJECT
public:
    explicit MemoryProfile(QObject *parent = nullptr);

signals:

public slots:

public:
    /*!
     * 更新内存占用率
     */
    double updateSystemMemoryUsage();
    /*!
     * 获取内存占用率
     */
    double getMemUsage();

private:
    double mMemUsage;
};

#endif // MEMORYPROFILE_H
