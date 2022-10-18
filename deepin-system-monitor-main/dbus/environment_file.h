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

#ifndef ENVIRONMENT_FILE_H
#define ENVIRONMENT_FILE_H

#include <QList>
#include <QString>
#include <QtDBus>

/**
 * @brief The EnvironmentFile class
 */
class EnvironmentFile
{
public:
    /**
     * @brief Default constructor
     */
    EnvironmentFile();
    /**
     * @brief Copy constructor
     */
    EnvironmentFile(const EnvironmentFile &);
    /**
     * @brief operator = Copy assignment
     * @return Copied version
     */
    EnvironmentFile &operator=(const EnvironmentFile &);
    ~EnvironmentFile();

    /**
     * @brief registerMetaType Register class to QT's metatype system
     */
    static void registerMetaType();

    /**
     * @brief operator == Compare with others
     * @param other Other EnvironmentFile object
     * @return true/false
     */
    inline bool operator==(const EnvironmentFile &other) const
    {
        return envFile == other.envFile && flag == other.flag;
    }

    /**
     * @brief operator << Print this object to debug stream
     * @param debug Debug stream
     * @param file EnvironmentFile object
     * @return Debug stream
     */
    friend QDebug &operator<<(QDebug &debug, const EnvironmentFile &file);
    /**
     * @brief operator << Print this object to DBus argument object
     * @param argument DBus argument object
     * @param file EnvironmentFile object
     * @return DBus argument object
     */
    friend QDBusArgument &operator<<(QDBusArgument &argument, const EnvironmentFile &file);
    /**
     * @brief operator << Print this object to data stream
     * @param stream Data stream
     * @param file EnvironmentFile object
     * @return Data stream
     */
    friend QDataStream &operator<<(QDataStream &stream, const EnvironmentFile &file);
    /**
     * @brief operator >> Read EnvironmentFile object from DBus argument object
     * @param argument DBus argument object
     * @param file EnvironmentFile object
     * @return DBus argument object
     */
    friend const QDBusArgument &operator>>(const QDBusArgument &argument, EnvironmentFile &file);
    /**
     * @brief operator >> Read EnvironmentFile object from data stream
     * @param stream Data stream
     * @param file EnvironmentFile object
     * @return Data stream
     */
    friend const QDataStream &operator>>(QDataStream &stream, EnvironmentFile &file);

    // environment file
    QString envFile {};
    // flag
    bool flag {};
};

typedef QList<EnvironmentFile> EnvironmentFileList;
Q_DECLARE_METATYPE(EnvironmentFile)
Q_DECLARE_METATYPE(EnvironmentFileList)

#endif  // ENVIRONMENT_FILE_H
