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
#ifndef DESKTOP_ENTRY_CACHE_UPDATER_H
#define DESKTOP_ENTRY_CACHE_UPDATER_H

#include "common/uevent_filter.h"
#include "desktop_entry_cache.h"

using namespace common::core;

namespace common {
namespace core {
class UEventLoop;
}
} // namespace common

namespace core {
namespace process {

class DesktopEntryCacheUpdater : public QObject
    , public UEventFilter
{
    Q_OBJECT

public:
    enum {
        kCacheUpdateTimer = 0
    };

    explicit DesktopEntryCacheUpdater(QObject *parent = nullptr);
    ~DesktopEntryCacheUpdater() override = default;

    static DesktopEntry createEntry(const QFileInfo &fileInfo);

    void scheduleUpdate(UEventLoop *loop, const struct timeval *interval);
    bool uevent(UEvent *event) override;

signals:
    void cacheUpdated();

private:
    void updateCache();

private:
    QHash<QString, DesktopEntry> m_cache;

    friend class DesktopEntryCache;
};

} // namespace process
} // namespace core

#endif // DESKTOP_ENTRY_CACHE_UPDATER_H
