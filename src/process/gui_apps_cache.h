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
#ifndef GUI_APPS_CACHE_H
#define GUI_APPS_CACHE_H

#include <QList>

namespace core {
namespace process {

class GuiAppsCache
{
public:
    explicit GuiAppsCache();
    virtual ~GuiAppsCache() = default;

    bool isGuiApp(pid_t pid) const;
    void updateCache(const QList<pid_t> &cache);

private:
    QList<pid_t> m_cache;
};

inline bool GuiAppsCache::isGuiApp(pid_t pid) const
{
    return m_cache.contains(pid);
}

inline void GuiAppsCache::updateCache(const QList<pid_t> &cache)
{
    m_cache = cache;
}

} // namespace process
} // namespace core

#endif // GUI_APPS_CACHE_H
