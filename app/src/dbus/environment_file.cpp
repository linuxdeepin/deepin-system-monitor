#include "environment_file.h"

EnvironmentFile::EnvironmentFile()
    : envFile()
    , flag(false)
{
}

EnvironmentFile::~EnvironmentFile() {}

// static
void EnvironmentFile::registerMetaType()
{
    qRegisterMetaType<EnvironmentFile>("EnvironmentFile");
    qDBusRegisterMetaType<EnvironmentFile>();
    qRegisterMetaType<EnvironmentFileList>("EnvironmentFileList");
    qDBusRegisterMetaType<EnvironmentFileList>();
}

QDebug& operator<<(QDebug& debug, const EnvironmentFile& file)
{
    debug << file.envFile << " flag: " << file.flag;
    return debug;
}

QDBusArgument& operator<<(QDBusArgument& argument, const EnvironmentFile& file)
{
    argument.beginStructure();
    argument << file.envFile << file.flag;
    argument.endStructure();
    return argument;
}

QDataStream& operator<<(QDataStream& stream, const EnvironmentFile& file)
{
    stream << file.envFile << file.flag;
    return stream;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, EnvironmentFile& file)
{
    argument.beginStructure();
    argument >> file.envFile >> file.flag;
    argument.endStructure();
    return argument;
}

const QDataStream& operator>>(QDataStream& stream, EnvironmentFile& file)
{
    stream >> file.envFile >> file.flag;
    return stream;
}
