/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      wangchao <wangchao@uniontech.com>
* Maintainer:  wangchao <wangchao@uniontech.com>
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

#include "process/desktop_entry_cache.h"
#include "process/desktop_entry_cache_updater.h"

#include <QFileInfo>
#include <QDir>
#include <DDesktopEntry>
#include <QDebug>
#include <QHash>
#include <memory>

DCORE_USE_NAMESPACE

#include "ut_desktop_entry_cache.h"

Ut_DesktopEntryCache::Ut_DesktopEntryCache()
{
}

TEST(UT_DesktopEntryCache_DesktopEntryCache, UT_DesktopEntryCache_DesktopEntryCache_001)
{
    using namespace core::process;
    DesktopEntryCache *cache = new DesktopEntryCache();
    EXPECT_NE(cache, nullptr);
    delete cache;
}

TEST(UT_DesktopEntryCache_contains, UT_DesktopEntryCache_contains_001)
{
    using namespace core::process;
    DesktopEntryCache *cache = new DesktopEntryCache();
    DesktopEntry entry;
    QString key("test");

    cache->m_cache.insert(key, entry);

    EXPECT_EQ(cache->contains(key), true);
    delete cache;
}

TEST(UT_DesktopEntryCache_entry, UT_DesktopEntryCache_entry_001)
{
    using namespace core::process;
    DesktopEntryCache *cache = new DesktopEntryCache();
    QString key("test");

    const QString filePath("test.desktop");
    const QString name = "test1";
    const QString displayName = "DisplayTest1";
    const QString exec = QString("/usr/bin/%1").arg(name);
    const QString icon = "/usr/share/test1/icon";
    const QString startup = name;

    QFile file(filePath);

    // 创建desktop文件
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QByteArray data;
        file.write("[Desktop Entry]\n");
        file.write(QString("Name=%1\n").arg(name).toUtf8());
        file.write(QString("GenericName=%1\n").arg(displayName).toUtf8());
        file.write("Type=Application\n");
        file.write(QString("Exec=%1\n").arg(exec).toUtf8());
        file.write(QString("Icon=%1\n").arg(icon).toUtf8());
        file.write("Categories=Utility\n");
        file.write(QString("StartupWMClass=%1\n").arg(startup).toUtf8());
        file.close();
    }

    QFileInfo info(filePath);
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(info);

    cache->m_cache.insert(key, entry);

    EXPECT_EQ(cache->entry(key)->name, name);
    delete cache;

    if(file.exists()) {
        file.remove();
    }
}

TEST(UT_DesktopEntryCache_entryWithDesktopFile, UT_DesktopEntryCache_entryWithDesktopFile_001)
{
    using namespace core::process;
    DesktopEntryCache *cache = new DesktopEntryCache();
    QString key("test");

    const QString filePath("test.desktop");
    const QString name = "test1";
    const QString displayName = "DisplayTest1";
    const QString exec = QString("/usr/bin/%1").arg(name);
    const QString icon = "/usr/share/test1/icon";
    const QString startup = name;

    QFile file(filePath);

    // 创建desktop文件
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QByteArray data;
        file.write("[Desktop Entry]\n");
        file.write(QString("Name=%1\n").arg(name).toUtf8());
        file.write(QString("GenericName=%1\n").arg(displayName).toUtf8());
        file.write("Type=Application\n");
        file.write(QString("Exec=%1\n").arg(exec).toUtf8());
        file.write(QString("Icon=%1\n").arg(icon).toUtf8());
        file.write("Categories=Utility\n");
        file.write(QString("StartupWMClass=%1\n").arg(startup).toUtf8());
        file.close();
    }

    QFileInfo info(filePath);
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(info);

    cache->m_cache.insert(key, entry);

    EXPECT_EQ(cache->entryWithDesktopFile(filePath)->name, name);
    delete cache;

    if(file.exists()) {
        file.remove();
    }
}

TEST(UT_DesktopEntryCache_entryWithSubName, UT_DesktopEntryCache_entryWithSubName_001)
{
    using namespace core::process;
    DesktopEntryCache *cache = new DesktopEntryCache();
    QString key("test");

    const QString filePath("test.desktop");
    const QString name = "test1";
    const QString displayName = "DisplayTest1";
    const QString exec = QString("/usr/bin/%1").arg(name);
    const QString icon = "/usr/share/test1/icon";
    const QString startup = name;

    QFile file(filePath);

    // 创建desktop文件
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QByteArray data;
        file.write("[Desktop Entry]\n");
        file.write(QString("Name=%1\n").arg(name).toUtf8());
        file.write(QString("GenericName=%1\n").arg(displayName).toUtf8());
        file.write("Type=Application\n");
        file.write(QString("Exec=%1\n").arg(exec).toUtf8());
        file.write(QString("Icon=%1\n").arg(icon).toUtf8());
        file.write("Categories=Utility\n");
        file.write(QString("StartupWMClass=%1\n").arg(startup).toUtf8());
        file.close();
    }

    QFileInfo info(filePath);
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(info);

    cache->m_cache.insert(key, entry);

    EXPECT_EQ(cache->entryWithSubName(key)->name, name);
    delete cache;

    if(file.exists()) {
        file.remove();
    }
}

TEST(UT_DesktopEntryCache_updateCache, UT_DesktopEntryCache_updateCache_001)
{
    using namespace core::process;
    DesktopEntryCache *cache = new DesktopEntryCache();

    cache->updateCache();
    EXPECT_GT(cache->m_cache.size(), 0);
    delete cache;
}
