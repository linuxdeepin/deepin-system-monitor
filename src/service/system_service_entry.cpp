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

#include "system_service_entry.h"

SystemServiceEntry::SystemServiceEntry()
    : data(new SystemServiceEntryData)
{
}
SystemServiceEntry::SystemServiceEntry(const QString &id,
                                       const QString &sname,
                                       const QString &loadState,
                                       const QString &activeState,
                                       const QString &subState,
                                       const QString &state,
                                       const QString &startupType,
                                       const QString &unitObjectPath,
                                       const QString &description,
                                       quint32 mainPID,
                                       bool canReload,
                                       bool canStart,
                                       bool canStop)
    : data(new SystemServiceEntryData)
{
    setId(id);
    setSName(sname);
    setLoadState(loadState);
    setActiveState(activeState);
    setSubState(subState);
    setState(state);
    setStartupType(startupType);
    setUnitObjectPath(unitObjectPath);
    setDescription(description);
    setMainPID(mainPID);
    setCanReload(canReload);
    setCanStart(canStart);
    setCanStop(canStop);
}

SystemServiceEntry::SystemServiceEntry(const SystemServiceEntry &rhs)
    : data(rhs.data)
{
}

SystemServiceEntry &SystemServiceEntry::operator=(const SystemServiceEntry &rhs)
{
    if (this != &rhs)
        data.operator = (rhs.data);
    return *this;
}

SystemServiceEntry::~SystemServiceEntry() {}
