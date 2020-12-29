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
#include "desktop_entry_cache_updater.h"
#include "common/uevent_loop.h"

#include <DDesktopEntry>

#include <QDir>

#define DESKTOP_ENTRY_PATH "/usr/share/applications"
#define DESKTOP_ENTRY_SUFFIX ".desktop"

DCORE_USE_NAMESPACE

namespace core {
namespace process {

DesktopEntryCacheUpdater::DesktopEntryCacheUpdater(QObject *parent)
    : QObject(parent)
{
}

DesktopEntry DesktopEntryCacheUpdater::createEntry(const QFileInfo &fileInfo)
{
    if (!fileInfo.exists()) {
        return {};
    }

    DDesktopEntry dde(fileInfo.filePath());
    if (dde.status() != DDesktopEntry::NoError)
        return {};

    DesktopEntry entry(new struct desktop_entry_t());

    // display name
    entry->displayName = dde.ddeDisplayName();
    // icon name
    auto icon = dde.stringValue("Icon");
    if (!icon.isEmpty()) {
        entry->icon = icon;
    }

    // exec & name
    auto tryExec = dde.stringValue("TryExec");
    auto exec = dde.stringValue("Exec");
    if (!tryExec.isEmpty()) {
        entry->exec = tryExec.split(' ');
        entry->name = QFileInfo(entry->exec[0]).baseName();
    } else if (!exec.isEmpty()) {
        entry->exec = exec.split(' ');
        entry->name = QFileInfo(entry->exec[0]).baseName();
    } else {
        entry->name = dde.name();
    }

    // startup wm class & name
    auto wmclass = dde.stringValue("StartupWMClass");
    if (!wmclass.isEmpty()) {
        entry->startup_wm_class = wmclass;
        entry->name = wmclass;
    }

    return entry;
}

void DesktopEntryCacheUpdater::scheduleUpdate(UEventLoop *loop, const timeval *interval)
{
    loop->installTimerEventFilter(kCacheUpdateTimer, true, UEvent::kEventPriorityLow, interval, this);
}

bool DesktopEntryCacheUpdater::uevent(UEvent *event)
{
    if (!event)
        return false;

    if (event->type == UEvent::kEventTypeTimer) {
        auto *ev = dynamic_cast<UTimerEvent *>(event);
        if (ev->timerId == kCacheUpdateTimer) {
            updateCache();

            return true;
        }
    }

    return false;
}

void DesktopEntryCacheUpdater::updateCache()
{
    m_cache.clear();

    auto fileInfoList = QDir(DESKTOP_ENTRY_PATH).entryInfoList(QDir::Files);
    for (auto &fileInfo : fileInfoList) {
        if (!fileInfo.fileName().endsWith(DESKTOP_ENTRY_SUFFIX))
            continue;

        auto entry = DesktopEntryCacheUpdater::createEntry(fileInfo);
        if (entry) {
            m_cache[fileInfo.fileName()] = entry;
            m_cache[entry->name] = entry;
        }
    }

    emit cacheUpdated();
}

} // namespace process
} // namespace core
