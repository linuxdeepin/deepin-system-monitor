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
#ifndef DESKTOP_ENTRY_CACHE_H
#define DESKTOP_ENTRY_CACHE_H

#include <QHash>

#include <memory>

namespace core {
namespace process {

struct desktop_entry_t {
    QString name;
    QString displayName;
    QStringList exec;
    QString icon;
    QString startup_wm_class;
};
using DesktopEntry = std::shared_ptr<struct desktop_entry_t>;

class DesktopEntryCache
{
public:
    explicit DesktopEntryCache();
    virtual ~DesktopEntryCache() = default;

    bool contains(const QString &name) const;
    const DesktopEntry entry(const QString &name) const;
    const DesktopEntry entryWithDesktopFile(const QString &desktopFile);
    const DesktopEntry entryWithSubName(const QString &subName) const;

    void updateCache();


private:
    QHash<QString, DesktopEntry> m_cache;
};

inline bool DesktopEntryCache::contains(const QString &name) const
{
    return m_cache.contains(name);
}

inline const DesktopEntry DesktopEntryCache::entry(const QString &name) const
{
    return m_cache[name];
}

inline const DesktopEntry DesktopEntryCache::entryWithSubName(const QString &subName) const
{
    for (auto &key : m_cache.keys()) {
        if (key.contains(subName))
            return m_cache[key];
    }
    return {};
}

} // namespace process
} // namespace core

#endif // DESKTOP_ENTRY_CACHE_H
