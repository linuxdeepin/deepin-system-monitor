/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
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

#ifndef UNIT_FILE_INFO_H
#define UNIT_FILE_INFO_H

#include <QExplicitlySharedDataPointer>
#include <QList>
#include <QtDBus>

class UnitFileInfoData;
class QDebug;

class UnitFileInfo
{
public:
    UnitFileInfo();
    UnitFileInfo(const QString &name, const QString &status);
    UnitFileInfo(const UnitFileInfo &);
    UnitFileInfo &operator=(const UnitFileInfo &);
    ~UnitFileInfo();

    bool operator==(const UnitFileInfo &other) const;

    QString getName() const;
    /**
    * @brief setName
    * name 名称
    */
    void setName(const QString &name);

    QString getStatus() const;
    void setStatus(const QString &status);

    //注册自定义类型
    static void registerMetaType();

    //重载
    friend QDebug &operator<<(QDebug debug, const UnitFileInfo &unit);
    friend QDBusArgument &operator<<(QDBusArgument &argument, const UnitFileInfo &unit);
    friend QDataStream &operator<<(QDataStream &stream, const UnitFileInfo &unit);
    friend const QDBusArgument &operator>>(const QDBusArgument &argument, UnitFileInfo &unit);
    friend const QDataStream &operator>>(QDataStream &stream, UnitFileInfo &unit);

private:
    QExplicitlySharedDataPointer<UnitFileInfoData> data;
};

typedef QList<UnitFileInfo> UnitFileInfoList;

//声明
Q_DECLARE_METATYPE(UnitFileInfo)
Q_DECLARE_METATYPE(UnitFileInfoList)

//重载
QDBusArgument &operator<<(QDBusArgument &argument, const UnitFileInfoList &list);
const QDBusArgument &operator>>(const QDBusArgument &argument, UnitFileInfoList &list);

#endif  // UNIT_FILE_INFO_H
