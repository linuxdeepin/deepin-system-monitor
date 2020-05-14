#include <QString>

#include "unit_file_info.h"

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
        data.operator=(rhs.data);
    return *this;
}

UnitFileInfo::~UnitFileInfo() {}

bool UnitFileInfo::operator==(const UnitFileInfo &other) const
{
    return data->m_name == other.getName() && data->m_status == other.getStatus();
}

QString UnitFileInfo::getName() const
{
    return data->m_name;
}

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

void UnitFileInfo::registerMetaType()
{
    qRegisterMetaType<UnitFileInfo>("UnitFileInfo");
    qDBusRegisterMetaType<UnitFileInfo>();
    qRegisterMetaType<UnitFileInfoList>("UnitFileInfoList");
    qDBusRegisterMetaType<UnitFileInfoList>();
}

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
