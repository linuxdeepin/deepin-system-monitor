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

#include <DDesktopEntry>

#include <QMutex>
#include <QHashFunctions>
#include <QTimer>
#include <QDir>
#include <QHash>

#define DESKTOP_ENTRY_PATH "/usr/share/applications"
#define DESKTOP_ENTRY_SUFFIX ".desktop"

using Container = std::unordered_map<QString, DesktopEntry>;
using Cache = std::unique_ptr<Container>;

static const int kUpdateInterval = 1000 * 300; // 5 minutes interval

#ifndef QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH
namespace std {
template<>
struct hash<QString> {
    std::size_t operator()(const QString &s) const noexcept
    {
        return size_t(qHash(s));
    }
};
} // namespace std
#endif

DCORE_USE_NAMESPACE

class DesktopEntryCacheUpdater : public QObject
{
    Q_OBJECT

public:
    DesktopEntryCacheUpdater()
    {
        updateCache();
    }

    ~DesktopEntryCacheUpdater() = default;

public Q_SLOTS:
    void activateScheduler()
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(kUpdateInterval);
        connect(m_timer, &QTimer::timeout, this, &DesktopEntryCacheUpdater::updateCache);
        m_timer->start();
    }

    void updateCache()
    {
        Cache cache(new Container());

        auto fileInfoList = QDir(DESKTOP_ENTRY_PATH).entryInfoList(QDir::Files);
        for (auto &fileInfo : fileInfoList) {
            if (!fileInfo.fileName().endsWith(DESKTOP_ENTRY_SUFFIX))
                continue;

            auto entry = createEntry(fileInfo);
            if (entry) {
                (*cache)[fileInfo.fileName()] = entry;
                (*cache)[entry->name] = entry;
            }
        }

        m_mutex.lock();
        m_cache.reset();
        m_cache = std::move(cache);
        m_mutex.unlock();

        Q_EMIT cacheUpdated();
    }

private:
    DesktopEntry createEntry(const QFileInfo &fileInfo)
    {
        if (!fileInfo.exists()) {
            return {};
        }

        DesktopEntry entry(new struct desktop_entry());
        DDesktopEntry desktop(fileInfo.filePath());

        entry->displayName = desktop.ddeDisplayName();
        auto icon = desktop.stringValue("Icon");
        if (!icon.isEmpty()) {
            entry->icon = QIcon::fromTheme(icon);
        }

        auto tryExec = desktop.stringValue("TryExec");
        auto exec = desktop.stringValue("Exec");
        if (!tryExec.isEmpty()) {
            entry->exec = tryExec.split(' ');
            entry->name = QFileInfo(entry->exec[0]).baseName();
        } else if (!exec.isEmpty()) {
            entry->exec = exec.split(' ');
            entry->name = QFileInfo(entry->exec[0]).baseName();
        } else {
            entry->name = desktop.name();
        }

        auto wmclass = desktop.stringValue("StartupWMClass");
        if (!wmclass.isEmpty()) {
            entry->startup_wm_class = wmclass;
            entry->name = wmclass;
        }

        return entry;
    }

Q_SIGNALS:
    void cacheUpdated();

private:
    std::mutex m_mutex;
    Cache m_cache;
    QTimer *m_timer;

    friend class DesktopEntryCache;
};

DesktopEntryCache::DesktopEntryCache(QObject *parent)
    : QObject(parent)
{
    m_cacheUpdater = new DesktopEntryCacheUpdater();
    // initial cache fillup
    m_cache = std::move(m_cacheUpdater->m_cache);
    m_cacheUpdater->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_cacheUpdater, &QObject::deleteLater);
    connect(&m_thread, &QThread::started, m_cacheUpdater, &DesktopEntryCacheUpdater::activateScheduler);
    connect(m_cacheUpdater, &DesktopEntryCacheUpdater::cacheUpdated, this, [=]() {
        std::lock_guard<std::mutex> lock(m_cacheUpdater->m_mutex);
        m_cache = std::move(m_cacheUpdater->m_cache);
    });
    m_thread.start();
}

DesktopEntryCache::~DesktopEntryCache()
{
    m_thread.quit();
    m_thread.wait();
}

DesktopEntry DesktopEntryCache::entry(const QString &key) const
{
    if (m_cache) {
        auto iter = m_cache->find(key);
        if (iter != m_cache->end())
            return iter->second;
    }
    return {};
}

DesktopEntry DesktopEntryCache::entryWithSubKey(const QString &subKey) const
{
    if (m_cache) {
        for (auto iter : (*m_cache)) {
            if (iter.first.contains(subKey))
                return iter.second;
        }
    }

    return {};
}

DesktopEntry DesktopEntryCache::createCachedEntry(const QString &desktopPath)
{
    QFileInfo fileInfo(desktopPath);
    if (!fileInfo.exists() || !m_cache)
        return {};

    auto entry = m_cacheUpdater->createEntry(fileInfo);
    if (entry) {
        (*m_cache)[entry->name] = entry;
        (*m_cache)[fileInfo.fileName()] = entry;
    }
    return entry;
}

#include "desktop_entry_cache.moc"
