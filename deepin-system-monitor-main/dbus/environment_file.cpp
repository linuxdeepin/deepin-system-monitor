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
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "environment_file.h"

// Default constructor
EnvironmentFile::EnvironmentFile()
    : envFile()
    , flag(false)
{
}

// Copy constructor
EnvironmentFile::EnvironmentFile(const EnvironmentFile &rhs) :
    envFile(rhs.envFile),
    flag(rhs.flag)
{
}

// Copy assignment
EnvironmentFile &EnvironmentFile::operator=(const EnvironmentFile &rhs)
{
    if (this != &rhs) {
        envFile = rhs.envFile;
        flag = rhs.flag;
    }
    return *this;
}

// Destructor
EnvironmentFile::~EnvironmentFile() {}

// Register meta type
void EnvironmentFile::registerMetaType()
{
    qRegisterMetaType<EnvironmentFile>("EnvironmentFile");
    qDBusRegisterMetaType<EnvironmentFile>();
    qRegisterMetaType<EnvironmentFileList>("EnvironmentFileList");
    qDBusRegisterMetaType<EnvironmentFileList>();
}

// Print EnvironmentFile object to debug stream
QDebug &operator<<(QDebug &debug, const EnvironmentFile &file)
{
    debug << file.envFile << " flag: " << file.flag;
    return debug;
}

// Print EnvironmentFile object to DBus argument object
QDBusArgument &operator<<(QDBusArgument &argument, const EnvironmentFile &file)
{
    argument.beginStructure();
    argument << file.envFile << file.flag;
    argument.endStructure();
    return argument;
}

// Print EnvironmentFile object to data stream
QDataStream &operator<<(QDataStream &stream, const EnvironmentFile &file)
{
    stream << file.envFile << file.flag;
    return stream;
}

// Read EnvironmentFile object from DBus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, EnvironmentFile &file)
{
    argument.beginStructure();
    argument >> file.envFile >> file.flag;
    argument.endStructure();
    return argument;
}

// Read EnvironmentFile object from data stream
const QDataStream &operator>>(QDataStream &stream, EnvironmentFile &file)
{
    stream >> file.envFile >> file.flag;
    return stream;
}
