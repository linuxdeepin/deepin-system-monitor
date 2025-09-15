// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "desktop_entry_cache.h"
#include "ddlog.h"

#include "desktop_entry_cache_updater.h"

#include <QFileInfo>
#include <QDir>
#include <DDesktopEntry>
#include <QDebug>

DCORE_USE_NAMESPACE
using namespace DDLog;

namespace core {
namespace process {

#define DESKTOP_ENTRY_PATH "/usr/share/applications"
#define DESKTOP_ENTRY_SUFFIX ".desktop"

DesktopEntryCache::DesktopEntryCache()
{
    qCDebug(app) << "DesktopEntryCache created";
}

const DesktopEntry DesktopEntryCache::entryWithDesktopFile(const QString &desktopFile)
{
    qCDebug(app) << "Getting entry for desktop file:" << desktopFile;
    QFileInfo fileInfo(desktopFile);
    if (!fileInfo.exists()) {
        qCDebug(app) << "Desktop file does not exist:" << desktopFile;
        return {};
    }

    auto entry = DesktopEntryCacheUpdater::createEntry(fileInfo);
    if (entry) {
        qCDebug(app) << "Entry created, updating cache for:" << fileInfo.fileName();
        m_cache[fileInfo.fileName()] = entry;
        m_cache[entry->name] = entry;
    }
    return entry;
}

QHash<QString, DesktopEntry> DesktopEntryCache::getCache()
{
    // qCDebug(app) << "Getting cache";
    return m_cache;
}

void DesktopEntryCache::updateCache()
{
    qCDebug(app) << "Updating cache";
    m_cache.clear();

    QString xdgDataDirPath(getenv("XDG_DATA_DIRS"));
    QString xdgDataHomePath(getenv("XDG_DATA_HOME"));
    // Add XDG_DATA_HOME to XDG_DATA_DIRS
    if (!xdgDataHomePath.isEmpty()) {
        if (!xdgDataDirPath.isEmpty()) {
            xdgDataDirPath += ":" + xdgDataHomePath;
        } else {
            xdgDataDirPath = xdgDataHomePath;
        }
        qCDebug(app) << "XDG_DATA_DIRS:" << xdgDataDirPath;
    }

    if (xdgDataDirPath.isEmpty()) {
        qCDebug(app) << "XDG_DATA_DIRS is empty, using default path";
        auto fileInfoList = QDir(DESKTOP_ENTRY_PATH).entryInfoList(QDir::Files);
        for (auto &fileInfo : fileInfoList) {
            entryWithDesktopFile(fileInfo.filePath());
        }
    } else {
        qCDebug(app) << "XDG_DATA_DIRS is set, using it:" << xdgDataDirPath;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QStringList xdgDataDirPaths = xdgDataDirPath.split(":", QString::SkipEmptyParts);
#else
        QStringList xdgDataDirPaths = xdgDataDirPath.split(":", Qt::SkipEmptyParts);
#endif
        for (auto path : xdgDataDirPaths) {
            auto fileInfoList = QDir(path.trimmed() + "/applications").entryInfoList(QDir::Files);
            for (auto &fileInfo : fileInfoList) {
                entryWithDesktopFile(fileInfo.filePath());
            }
        }
    }
}

} // namespace process
} // namespace core
