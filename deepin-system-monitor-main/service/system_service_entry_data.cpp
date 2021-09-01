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

#include "system_service_entry_data.h"

SystemServiceEntryData::SystemServiceEntryData() {}

SystemServiceEntryData::SystemServiceEntryData(const SystemServiceEntryData &rhs)
    : QSharedData(rhs)
    , m_id(rhs.m_id)
    , m_sname(rhs.m_sname)
    , m_loadState(rhs.m_loadState)
    , m_activeState(rhs.m_activeState)
    , m_subState(rhs.m_subState)
    , m_state(rhs.m_state)
    , m_startupType(rhs.m_startupType)
    , m_unitObjectPath(rhs.m_unitObjectPath)
    , m_description(rhs.m_description)
    , m_mainPID(rhs.m_mainPID)
    , m_canReload(rhs.m_canReload)
    , m_canStart(rhs.m_canStart)
    , m_canStop(rhs.m_canStop)
{
}

SystemServiceEntryData &SystemServiceEntryData::operator=(const SystemServiceEntryData &rhs)
{
    if (this != &rhs) {
        m_id.operator = (rhs.m_id);
        m_sname.operator = (rhs.m_sname);
        m_loadState.operator = (rhs.m_loadState);
        m_activeState.operator = (rhs.m_activeState);
        m_subState.operator = (rhs.m_subState);
        m_state.operator = (rhs.m_state);
        m_startupType.operator = (rhs.m_startupType);
        m_unitObjectPath = rhs.m_unitObjectPath;
        m_description.operator = (rhs.m_description);
        m_mainPID = rhs.m_mainPID;
        m_canReload = rhs.m_canReload;
        m_canStart = rhs.m_canStart;
        m_canStop = rhs.m_canStop;
    }
    return *this;
}

SystemServiceEntryData::~SystemServiceEntryData() {}
