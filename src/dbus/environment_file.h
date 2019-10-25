#ifndef ENVIRONMENT_FILE_H
#define ENVIRONMENT_FILE_H

#include <QList>
#include <QString>
#include <QtDBus>

class EnvironmentFile
{
public:
    EnvironmentFile();
    ~EnvironmentFile();

    static void registerMetaType();

    inline bool operator==(const EnvironmentFile& other) const
    {
        return envFile == other.envFile && flag == other.flag;
    }

    friend QDebug& operator<<(QDebug& debug, const EnvironmentFile& file);
    friend QDBusArgument& operator<<(QDBusArgument& argument, const EnvironmentFile& file);
    friend QDataStream& operator<<(QDataStream& stream, const EnvironmentFile& file);
    friend const QDBusArgument& operator>>(const QDBusArgument& argument, EnvironmentFile& file);
    friend const QDataStream& operator>>(QDataStream& stream, EnvironmentFile& file);

    QString envFile;
    bool flag;
};

typedef QList<EnvironmentFile> EnvironmentFileList;
Q_DECLARE_METATYPE(EnvironmentFile)
Q_DECLARE_METATYPE(EnvironmentFileList)

#endif  // ENVIRONMENT_FILE_H
