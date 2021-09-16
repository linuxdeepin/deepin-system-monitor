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
#ifndef PROCESS_NAME_H
#define PROCESS_NAME_H

#include <QString>
#include <QByteArrayList>

#include <memory>

namespace core {
namespace process {

class Process;

class ProcessName
{
public:
    explicit ProcessName();
    ~ProcessName() = default;

    void refreashProcessName(Process *proc);

    QString name() const;
    QString displayName() const;

    static QString normalizeProcessName(const QString &name, const QByteArrayList &cmdline);

private:
    QString getDisplayName(Process *proc);

private:
    QString m_name {};
    QString m_displayName {};
};

inline QString ProcessName::name() const
{
    return m_name;
}

inline QString ProcessName::displayName() const
{
    return m_displayName;
}

} // namespace process
} // namespace core

#endif // PROCESS_NAME_H
