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
#ifndef WM_WINDOW_LIST_H
#define WM_WINDOW_LIST_H

#include "wm_connection.h"
#include "wm_atom.h"
#include "wm_info.h"

namespace core {
namespace process {
class Process;
}
}

namespace core {
namespace wm {

union size_u {
    struct size_t {
        uint w;
        uint h;
    } s;
    uint64_t k;
};

/**
 * @brief The WMWindowList class
 */
class WMWindowList : public QObject
{
    Q_OBJECT

public:
    explicit WMWindowList(QObject *parent = nullptr);
    ~WMWindowList() override = default;

    int getAppCount();

    QImage getWindowIcon(pid_t pid) const;
    QString getWindowTitle(pid_t pid) const;

    bool isTrayApp(pid_t pid) const;
    bool isGuiApp(pid_t pid) const;
    bool isDesktopEntryApp(pid_t pid) const;
    void addDesktopEntryApp(core::process::Process *proc);

    void removeDesktopEntryApp(pid_t pid);
    void updateWindowListCache();

private:
    QList<WMWId> getTrayWindows() const;
    WMWindow getWindowInfo(WMWId winId);
    pid_t getWindowPid(WMWId window) const;
    QStringList getWindowType(WMWId winId) const;
    QByteArray getAtomName(xcb_connection_t *conn, xcb_atom_t atom) const;

private:
    std::map<pid_t, WMWindow> m_guiAppcache;
    std::map<pid_t, WMWindow> m_trayAppcache;

    QList<pid_t> m_desktopEntryCache;
    WMConnection m_conn;
};

} // namespace wm
} // namespace core

#endif // WM_WINDOW_LIST_H
