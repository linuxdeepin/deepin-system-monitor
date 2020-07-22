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

#ifndef ENVIRONMENT_FILE_H
#define ENVIRONMENT_FILE_H

#include <QList>
#include <QString>
#include <QtDBus>

class EnvironmentFile
{
public:
    EnvironmentFile();
    EnvironmentFile(const EnvironmentFile &);
    EnvironmentFile &operator=(const EnvironmentFile &);
    ~EnvironmentFile();

    static void registerMetaType();

    inline bool operator==(const EnvironmentFile &other) const
    {
        return envFile == other.envFile && flag == other.flag;
    }

    friend QDebug &operator<<(QDebug &debug, const EnvironmentFile &file);
    friend QDBusArgument &operator<<(QDBusArgument &argument, const EnvironmentFile &file);
    friend QDataStream &operator<<(QDataStream &stream, const EnvironmentFile &file);
    friend const QDBusArgument &operator>>(const QDBusArgument &argument, EnvironmentFile &file);
    friend const QDataStream &operator>>(QDataStream &stream, EnvironmentFile &file);

    QString envFile {};
    bool flag {};
};

typedef QList<EnvironmentFile> EnvironmentFileList;
Q_DECLARE_METATYPE(EnvironmentFile)
Q_DECLARE_METATYPE(EnvironmentFileList)

#endif  // ENVIRONMENT_FILE_H
