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

#include "common/uevent_filter.h"
#include "wm_connection.h"
#include "wm_atom.h"
#include "wm_info.h"

#include <QByteArrayList>
#include <QThread>

#include <memory>

using namespace common::core;

namespace common {
namespace core {
class UEvent;
class UEventLoop;
} // namespace core
} // namespace common

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
    , public UEventFilter
{
    Q_OBJECT

public:
    enum {
        kListUpdateTimer
    };

    WMWindowList(QObject *parent = nullptr);
    ~WMWindowList() override = default;

    const QMap<uint64_t, QVector<uint>> getWindowIcon(pid_t pid) const;
    const QString getWindowTitle(pid_t pid) const;
    const QList<pid_t> getTrayProcessList() const;
    const QList<pid_t> getGuiProcessList() const;

    void scheduleUpdate(UEventLoop *loop, const struct timeval *interval);
    bool uevent(UEvent *event) override;

private:
    QList<WMWId> getTrayWindows() const;
    void updateWindowListCache();
    WMWindow getWindowInfo(WMWId winId);

private:
    std::multimap<pid_t, WMWindow> m_cache;
    WMConnection m_conn;
};

} // namespace wm
} // namespace core

#endif // WM_WINDOW_LIST_H
