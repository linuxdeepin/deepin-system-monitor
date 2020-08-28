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

#include "unit_info.h"

#include <QDebug>

class UnitInfoData : public QSharedData
{
public:
    UnitInfoData() {}
    UnitInfoData(const UnitInfoData &rhs)
        : QSharedData(rhs)
        , m_jobId(rhs.m_jobId)
        , m_name(rhs.m_name)
        , m_description(rhs.m_description)
        , m_loadState(rhs.m_loadState)
        , m_activeState(rhs.m_activeState)
        , m_subState(rhs.m_subState)
        , m_followedBy(rhs.m_followedBy)
        , m_unitObjectPath(rhs.m_unitObjectPath)
        , m_jobType(rhs.m_jobType)
        , m_jobObjectPath(rhs.m_jobObjectPath)
    {
    }
    UnitInfoData &operator=(const UnitInfoData &rhs)
    {
        if (this != &rhs) {
            m_jobId = rhs.m_jobId;
            m_name = rhs.m_name;
            m_description = rhs.m_description;
            m_loadState = rhs.m_loadState;
            m_activeState = rhs.m_activeState;
            m_subState = rhs.m_subState;
            m_followedBy = rhs.m_followedBy;
            m_unitObjectPath = rhs.m_unitObjectPath;
            m_jobType = rhs.m_jobType;
            m_jobObjectPath = rhs.m_jobObjectPath;
        }
        return *this;
    }
    ~UnitInfoData() {}

    friend class UnitInfo;

private:
    quint32 m_jobId {0};
    QString m_name {};
    QString m_description {};
    QString m_loadState {};
    QString m_activeState {};
    QString m_subState {};
    QString m_followedBy {};
    QString m_unitObjectPath {};
    QString m_jobType {};
    QString m_jobObjectPath {};
};

UnitInfo::UnitInfo()
    : data(new UnitInfoData)
{
}

UnitInfo::UnitInfo(const QString &name, const QString &description, const QString &loadState,
                   const QString &activeState, const QString &subState, const QString &followedBy,
                   const QString &unitObjectPath, quint32 jobId, const QString &jobType,
                   const QString &jobObjectPath)
{
    data = new UnitInfoData();
    data->m_name = name;
    data->m_description = description;
    data->m_loadState = loadState;
    data->m_activeState = activeState;
    data->m_subState = subState;
    data->m_followedBy = followedBy;
    data->m_unitObjectPath = unitObjectPath;
    data->m_jobId = jobId;
    data->m_jobType = jobType;
    data->m_jobObjectPath = jobObjectPath;
}

UnitInfo::UnitInfo(const UnitInfo &rhs)
    : data(rhs.data)
{
}

//重载=
UnitInfo &UnitInfo::operator=(const UnitInfo &rhs)
{
    if (this != &rhs)
        data.operator = (rhs.data);
    return *this;
}

UnitInfo::~UnitInfo() {}

//重载==
bool UnitInfo::operator==(const UnitInfo &other) const
{
    return (
               data->m_name == other.getName() && data->m_description == other.getDescription() &&
               data->m_loadState == other.getLoadState() &&
               data->m_activeState == other.getActiveState() && data->m_subState == other.getSubState() &&
               data->m_followedBy == other.getFollowedBy() &&
               data->m_unitObjectPath == other.getUnitObjectPath() && data->m_jobId == other.getJobId() &&
               data->m_jobType == other.getJobType() && data->m_jobObjectPath == other.getJobObjectPath());
}

//获取名称
QString UnitInfo::getName() const
{
    return data->m_name;
}

//设置名称
void UnitInfo::setName(const QString &name)
{
    data->m_name = name;
}

//获取描述
QString UnitInfo::getDescription() const
{
    return data->m_description;
}

//设置描述
void UnitInfo::setDescription(const QString &description)
{
    data->m_description = description;
}

//获取负载状态
QString UnitInfo::getLoadState() const
{
    return data->m_loadState;
}
//设置负载状态
void UnitInfo::setLoadState(const QString &loadState)
{
    data->m_loadState = loadState;
}

//获取激活状态
QString UnitInfo::getActiveState() const
{
    return data->m_activeState;
}

//设置激活状态
void UnitInfo::setActiveState(const QString &activeState)
{
    data->m_activeState = activeState;
}

QString UnitInfo::getSubState() const
{
    return data->m_subState;
}
void UnitInfo::setSubState(const QString &subState)
{
    data->m_subState = subState;
}

QString UnitInfo::getFollowedBy() const
{
    return data->m_followedBy;
}
void UnitInfo::setFollowedBy(const QString &followedBy)
{
    data->m_followedBy = followedBy;
}

//获取路径
QString UnitInfo::getUnitObjectPath() const
{
    return data->m_unitObjectPath;
}
//设置路径
void UnitInfo::setUnitObjectPath(const QString &unitObjectPath)
{
    data->m_unitObjectPath = unitObjectPath;
}

quint32 UnitInfo::getJobId() const
{
    return data->m_jobId;
}
void UnitInfo::setJobId(quint32 jobId)
{
    data->m_jobId = jobId;
}

QString UnitInfo::getJobType() const
{
    return data->m_jobType;
}
void UnitInfo::setJobType(const QString &jobType)
{
    data->m_jobType = jobType;
}

QString UnitInfo::getJobObjectPath() const
{
    return data->m_jobObjectPath;
}
void UnitInfo::setJobObjectPath(const QString &jobObjectPath)
{
    data->m_jobObjectPath = jobObjectPath;
}

//注册自定义类型
void UnitInfo::registerMetaType()
{
    qRegisterMetaType<UnitInfo>("UnitInfo");
    qDBusRegisterMetaType<UnitInfo>();
    qRegisterMetaType<UnitInfoList>("UnitInfoList");
    qDBusRegisterMetaType<UnitInfoList>();
}

//重载<<
QDebug &operator<<(QDebug &debug, const UnitInfo &unit)
{
    debug << unit.getName() << unit.getDescription() << unit.getLoadState() << unit.getActiveState()
          << unit.getSubState() << unit.getFollowedBy() << unit.getUnitObjectPath()
          << unit.getJobId() << unit.getJobType() << unit.getJobObjectPath();
    return debug;
}

QDBusArgument &operator<<(QDBusArgument &argument, const UnitInfo &unit)
{
    argument.beginStructure();
    argument << unit.getName() << unit.getDescription() << unit.getLoadState()
             << unit.getActiveState() << unit.getSubState() << unit.getFollowedBy()
             << unit.getUnitObjectPath() << unit.getJobId() << unit.getJobType()
             << unit.getJobObjectPath();
    argument.endStructure();
    return argument;
}

QDataStream &operator<<(QDataStream &stream, const UnitInfo &unit)
{
    stream << unit.getName() << unit.getDescription() << unit.getLoadState()
           << unit.getActiveState() << unit.getSubState() << unit.getFollowedBy()
           << unit.getUnitObjectPath() << unit.getJobId() << unit.getJobType()
           << unit.getJobObjectPath();
    return stream;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, UnitInfo &unit)
{
    QString name, description, loadState, activeState, subState, followedBy, unitObjectPath,
            jobType, jobObjectPath;
    quint32 jobId;
    argument.beginStructure();
    argument >> name >> description >> loadState >> activeState >> subState >> followedBy >>
             unitObjectPath >> jobId >> jobType >> jobObjectPath;
    unit.setName(name);
    unit.setDescription(description);
    unit.setLoadState(loadState);
    unit.setActiveState(activeState);
    unit.setSubState(subState);
    unit.setFollowedBy(followedBy);
    unit.setUnitObjectPath(unitObjectPath);
    unit.setJobId(jobId);
    unit.setJobType(jobType);
    unit.setJobObjectPath(jobObjectPath);
    argument.endStructure();
    return argument;
}

const QDataStream &operator>>(QDataStream &stream, UnitInfo &unit)
{
    QString name, description, loadState, activeState, subState, followedBy, unitObjectPath,
            jobType, jobObjectPath;
    quint32 jobId;
    stream >> name >> description >> loadState >> activeState >> subState >> followedBy >>
           unitObjectPath >> jobId >> jobType >> jobObjectPath;
    unit.setName(name);
    unit.setDescription(description);
    unit.setLoadState(loadState);
    unit.setActiveState(activeState);
    unit.setSubState(subState);
    unit.setFollowedBy(followedBy);
    unit.setUnitObjectPath(unitObjectPath);
    unit.setJobId(jobId);
    unit.setJobType(jobType);
    unit.setJobObjectPath(jobObjectPath);
    return stream;
}

QDBusArgument &operator<<(QDBusArgument &argument, const UnitInfoList &list)
{
    argument.beginArray(qMetaTypeId<UnitInfo>());
    foreach (const UnitInfo &unit, list) {
        argument << unit;
    }
    argument.endArray();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, UnitInfoList &list)
{
    argument.beginArray();
    while (!argument.atEnd()) {
        UnitInfo unit;
        argument >> unit;
        list << unit;
    }
    argument.endArray();
    return argument;
}
