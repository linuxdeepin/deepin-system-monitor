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

#include "unit_file_info.h"

#include <QString>
#include <QDebug>

class UnitFileInfoData : public QSharedData
{
public:
    UnitFileInfoData() {}
    UnitFileInfoData(const UnitFileInfoData &rhs)
        : QSharedData(rhs)
        , m_name(rhs.m_name)
        , m_status(rhs.m_status)
    {
    }
    //重载赋值运算符
    UnitFileInfoData &operator=(const UnitFileInfoData &rhs)
    {
        Q_UNUSED(__alignment);

        if (this != &rhs) {
            m_name = rhs.m_name;
            m_status = rhs.m_status;
        }
        return *this;
    }
    ~UnitFileInfoData() {}

    friend class UnitFileInfo;

private:
    int __alignment;  // simply used for suppressing compiler warnings
    QString m_name {};
    QString m_status {};
};

UnitFileInfo::UnitFileInfo()
    : data(new UnitFileInfoData)
{
}

UnitFileInfo::UnitFileInfo(const QString &name, const QString &status)
{
    data = new UnitFileInfoData();
    data->m_name = name;
    data->m_status = status;
}

UnitFileInfo::UnitFileInfo(const UnitFileInfo &rhs)
    : data(rhs.data)
{
}

UnitFileInfo &UnitFileInfo::operator=(const UnitFileInfo &rhs)
{
    if (this != &rhs)
        data.operator = (rhs.data);
    return *this;
}

UnitFileInfo::~UnitFileInfo() {}

bool UnitFileInfo::operator==(const UnitFileInfo &other) const
{
    return data->m_name == other.getName() && data->m_status == other.getStatus();
}

//获取名称
QString UnitFileInfo::getName() const
{
    return data->m_name;
}

//设置名称
void UnitFileInfo::setName(const QString &name)
{
    data->m_name = name;
}

QString UnitFileInfo::getStatus() const
{
    return data->m_status;
}

void UnitFileInfo::setStatus(const QString &status)
{
    data->m_status = status;
}

//注册自定义类型
void UnitFileInfo::registerMetaType()
{
    qRegisterMetaType<UnitFileInfo>("UnitFileInfo");
    qDBusRegisterMetaType<UnitFileInfo>();
    qRegisterMetaType<UnitFileInfoList>("UnitFileInfoList");
    qDBusRegisterMetaType<UnitFileInfoList>();
}

//重载
QDebug &operator<<(QDebug &debug, const UnitFileInfo &unit)
{
    debug << unit.getName() << unit.getStatus();
    return debug;
}

QDBusArgument &operator<<(QDBusArgument &argument, const UnitFileInfo &unit)
{
    argument.beginStructure();
    argument << unit.getName() << unit.getStatus();
    argument.endStructure();
    return argument;
}

QDataStream &operator<<(QDataStream &stream, const UnitFileInfo &unit)
{
    stream << unit.getName() << unit.getStatus();
    return stream;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, UnitFileInfo &unit)
{
    QString name, status;
    argument.beginStructure();
    argument >> name >> status;
    unit.setName(name);
    unit.setStatus(status);
    argument.endStructure();
    return argument;
}

const QDataStream &operator>>(QDataStream &stream, UnitFileInfo &unit)
{
    QString name, status;
    stream >> name >> status;
    unit.setName(name);
    unit.setStatus(status);
    return stream;
}

QDBusArgument &operator<<(QDBusArgument &argument, const UnitFileInfoList &list)
{
    argument.beginArray(qMetaTypeId<UnitFileInfoList>());
    foreach (const UnitFileInfo &unit, list) {
        argument << unit;
    }
    argument.endArray();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, UnitFileInfoList &list)
{
    argument.beginArray();
    while (!argument.atEnd()) {
        UnitFileInfo unit;
        argument >> unit;
        list << unit;
    }
    argument.endArray();
    return argument;
}
