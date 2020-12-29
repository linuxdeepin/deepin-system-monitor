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
#ifndef PROCESS_ICON_CACHE_H
#define PROCESS_ICON_CACHE_H

#include <QCache>
#include <QThread>

namespace core {
namespace process {

class ProcessIcon;
class ProcessIconCache : QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ProcessIconCache)

public:
    static ProcessIconCache *instance();

    ProcessIcon *getProcessIcon(const QString &procName) const;
    void addProcessIcon(const QString &procName, ProcessIcon *procIcon);
    void removeProcessIcon(const QString &procName);
    bool contains(const QString &procName) const;
    void clear();
    void setMaxCost(int cost);

private:
    explicit ProcessIconCache(QObject *parent = nullptr);

private:
    QCache<QString, ProcessIcon> m_cache {1024};

    static ProcessIconCache *m_instance;
};

inline ProcessIconCache *ProcessIconCache::instance()
{
    if (!m_instance)
        m_instance = new ProcessIconCache(QThread::currentThread());

    return m_instance;
}

inline ProcessIcon *ProcessIconCache::getProcessIcon(const QString &procName) const
{
    return m_cache[procName];
}

inline void ProcessIconCache::addProcessIcon(const QString &procName, ProcessIcon *procIcon)
{
    m_cache.insert(procName, procIcon);
}

inline void ProcessIconCache::removeProcessIcon(const QString &procName)
{
    m_cache.remove(procName);
}

inline bool ProcessIconCache::contains(const QString &procName) const
{
    return m_cache.contains(procName);
}

inline void ProcessIconCache::clear()
{
    m_cache.clear();
}

inline void ProcessIconCache::setMaxCost(int cost)
{
    m_cache.setMaxCost(cost);
}

} // namespace process
} // namespace core

#endif // PROCESS_ICON_CACHE_H
