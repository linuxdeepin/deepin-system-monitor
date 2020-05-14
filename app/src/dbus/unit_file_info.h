#ifndef UNIT_FILE_INFO_H
#define UNIT_FILE_INFO_H

#include <QDebug>
#include <QExplicitlySharedDataPointer>
#include <QList>
#include <QtDBus>

class UnitFileInfoData;

class UnitFileInfo
{
public:
    UnitFileInfo();
    UnitFileInfo(const QString& name, const QString& status);
    UnitFileInfo(const UnitFileInfo&);
    UnitFileInfo& operator=(const UnitFileInfo&);
    ~UnitFileInfo();

    bool operator==(const UnitFileInfo& other) const;

    QString getName() const;
    void setName(const QString& name);

    QString getStatus() const;
    void setStatus(const QString& status);

    static void registerMetaType();

    friend QDebug& operator<<(QDebug debug, const UnitFileInfo& unit);
    friend QDBusArgument& operator<<(QDBusArgument& argument, const UnitFileInfo& unit);
    friend QDataStream& operator<<(QDataStream& stream, const UnitFileInfo& unit);
    friend const QDBusArgument& operator>>(const QDBusArgument& argument, UnitFileInfo& unit);
    friend const QDataStream& operator>>(QDataStream& stream, UnitFileInfo& unit);

private:
    QExplicitlySharedDataPointer<UnitFileInfoData> data;
};

typedef QList<UnitFileInfo> UnitFileInfoList;

Q_DECLARE_METATYPE(UnitFileInfo)
Q_DECLARE_METATYPE(UnitFileInfoList)

QDBusArgument& operator<<(QDBusArgument& argument, const UnitFileInfoList& list);
const QDBusArgument& operator>>(const QDBusArgument& argument, UnitFileInfoList& list);

#endif  // UNIT_FILE_INFO_H
