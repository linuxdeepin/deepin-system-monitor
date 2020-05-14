#ifndef UNIT_INFO_H
#define UNIT_INFO_H

#include <QDebug>
#include <QExplicitlySharedDataPointer>
#include <QList>
#include <QtDBus>

class UnitInfoData;

class UnitInfo
{
public:
    UnitInfo();
    UnitInfo(const QString& name, const QString& description, const QString& loadState,
             const QString& activeState, const QString& subState, const QString& followedBy,
             const QString& unitObjectPath, quint32 jobId, const QString& jobType,
             const QString& jobObjectPath);
    UnitInfo(const UnitInfo&);
    UnitInfo& operator=(const UnitInfo&);
    ~UnitInfo();

    bool operator==(const UnitInfo& other) const;

    QString getName() const;
    void setName(const QString& name);

    QString getDescription() const;
    void setDescription(const QString& description);

    QString getLoadState() const;
    void setLoadState(const QString& loadState);

    QString getActiveState() const;
    void setActiveState(const QString& activeState);

    QString getSubState() const;
    void setSubState(const QString& subState);

    QString getFollowedBy() const;
    void setFollowedBy(const QString& followedBy);

    QString getUnitObjectPath() const;
    void setUnitObjectPath(const QString& unitObjectPath);

    quint32 getJobId() const;
    void setJobId(quint32 jobId);

    QString getJobType() const;
    void setJobType(const QString& jobType);

    QString getJobObjectPath() const;
    void setJobObjectPath(const QString& jobObjectPath);

    static void registerMetaType();

    friend QDebug& operator<<(QDebug& debug, const UnitInfo& unit);
    friend QDBusArgument& operator<<(QDBusArgument& argument, const UnitInfo& unit);
    friend QDataStream& operator<<(QDataStream& stream, const UnitInfo& unit);
    friend const QDBusArgument& operator>>(const QDBusArgument& argument, UnitInfo& unit);
    friend const QDataStream& operator>>(QDataStream& stream, UnitInfo& unit);

private:
    QExplicitlySharedDataPointer<UnitInfoData> data;
};

typedef QList<UnitInfo> UnitInfoList;

Q_DECLARE_METATYPE(UnitInfo)
Q_DECLARE_METATYPE(UnitInfoList)

QDBusArgument& operator<<(QDBusArgument& argument, const UnitInfoList& list);
const QDBusArgument& operator>>(const QDBusArgument& argument, UnitInfoList& list);

#endif  // UNIT_INFO_H
