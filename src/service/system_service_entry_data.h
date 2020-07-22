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

#ifndef SYSTEM_SERVICE_ENTRY_DATA_H
#define SYSTEM_SERVICE_ENTRY_DATA_H

#include <QSharedData>
#include <QString>

class SystemServiceEntry;

class SystemServiceEntryData : public QSharedData
{
public:
    SystemServiceEntryData();
    SystemServiceEntryData(const SystemServiceEntryData &);
    SystemServiceEntryData &operator=(const SystemServiceEntryData &);
    ~SystemServiceEntryData();

    friend class SystemServiceEntry;

private:
    // name =>
    // org.freedesktop.systemd1->/org/freedesktop/systemd1->org.freedesktop.systemd1.Manager->ListUnitFiles
    // with .service suffix
    QString m_id {};
    // service name: some entries may contain aliases, which will produce duplicate items with same pid, thats not we want!
    QString m_sname {};
    // LOAD: (loaded | not-found | stub | bad-setting | error | merged | masked) =>
    // org.freedesktop.systemd1->%objpath%->org.freedesktop.systemd1.Unit->LoadState
    QString m_loadState {};
    // ACTIVE: (active | inactive | reloading | failed | activating | deactivating) =>
    // org.freedesktop.systemd1->%objpath%->org.freedesktop.systemd1.Unit->ActiveState
    QString m_activeState {};
    // running status SUB: (running | exited | dead | startpre | start | start-post | reload | stop
    // | stop-watchdog | stop-sigterm | stop-sigkill | stop-post | final-sigterm | final-sigkill |
    // failed | auto-restart) =>
    // org.freedesktop.systemd1->%objpath%->org.freedesktop.systemd1.Unit->SubState
    QString m_subState {};
    // STATE: (enabled | disabled | masked | static | generated | transient | indirect) =>
    // org.freedesktop.systemd1->/org/freedesktop/systemd1->org.freedesktop.systemd1.Manager->ListUnitFiles
    QString m_state {};
    // Startup type
    QString m_startupType {};

    // unit object path
    QString m_unitObjectPath {};

    // DESCRIPTION
    QString m_description {};  // org.freedesktop.systemd1.Unit
    // PID
    quint32 m_mainPID {0};  // org.freedesktop.systemd1.Service

    bool m_canReload {false};  // org.freedesktop.systemd1.Unit
    bool m_canStart {false};   // org.freedesktop.systemd1.Unit
    bool m_canStop {false};    // org.freedesktop.systemd1.Unit
};

#endif  // SYSTEM_SERVICE_ENTRY_DATA_H
