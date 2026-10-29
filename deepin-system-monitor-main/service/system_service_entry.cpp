// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "system_service_entry.h"
#include "ddlog.h"

using namespace DDLog;

SystemServiceEntry::SystemServiceEntry()
    : data(new SystemServiceEntryData)
{
    // qCDebug(app) << "SystemServiceEntry default constructor called";
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
    // qCDebug(app) << "SystemServiceEntry constructor called with id:" << id;
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
    // qCDebug(app) << "SystemServiceEntry copy constructor called for id:" << rhs.getId();
}

SystemServiceEntry &SystemServiceEntry::operator=(const SystemServiceEntry &rhs)
{
    // qCDebug(app) << "SystemServiceEntry assignment operator called for id:" << rhs.getId();
    if (this != &rhs)
        data.operator = (rhs.data);
    return *this;
}

SystemServiceEntry::~SystemServiceEntry() {}
