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

#include <QIcon>
#include <QThread>

#include <memory>
#include <unordered_map>

struct desktop_entry {
    QString name;
    QString displayName;
    QStringList exec;
    QIcon icon;
    QString startup_wm_class;
};
using DesktopEntry = std::shared_ptr<struct desktop_entry>;

class DesktopEntryCacheUpdater;
class DesktopEntryCache : public QObject
{
    Q_OBJECT

public:
    explicit DesktopEntryCache(QObject *parent = nullptr);
    ~DesktopEntryCache();

    DesktopEntry entry(const QString &key) const;
    DesktopEntry entryWithSubKey(const QString &subKey) const;
    DesktopEntry createCachedEntry(const QString &desktopPath);

private:
    using Cache = std::unique_ptr<std::unordered_map<QString, DesktopEntry>>;
    Cache m_cache;

    QThread m_thread;
    DesktopEntryCacheUpdater *m_cacheUpdater {};
};

#endif // DESKTOP_ENTRY_CACHE_H
