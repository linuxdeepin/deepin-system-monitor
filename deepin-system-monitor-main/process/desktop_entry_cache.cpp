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
#include "desktop_entry_cache.h"

#include "desktop_entry_cache_updater.h"

#include <QFileInfo>
#include <QDir>
#include <DDesktopEntry>
#include <QDebug>

DCORE_USE_NAMESPACE

namespace core {
namespace process {

#define DESKTOP_ENTRY_PATH "/usr/share/applications"
#define DESKTOP_ENTRY_SUFFIX ".desktop"

DesktopEntryCache::DesktopEntryCache()
{

}

const DesktopEntry DesktopEntryCache::entryWithDesktopFile(const QString &desktopFile)
{
    QFileInfo fileInfo(desktopFile);
    if (!fileInfo.exists())
        return {};

    auto entry = DesktopEntryCacheUpdater::createEntry(fileInfo);
    if (entry) {
        m_cache[fileInfo.fileName()] = entry;
        m_cache[entry->name] = entry;
    }
    return entry;
}

void DesktopEntryCache::updateCache()
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
}

} // namespace process
} // namespace core
