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

#include "desktop_entry_stat.h"

#include <DDesktopEntry>

#include <QDebug>
#include <QTimer>
#include <QFileInfo>

#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#define DESKTOP_ENTRY_PATH "/usr/share/applications"

DCORE_USE_NAMESPACE

static const int kUpdateInterval = 1000 * 300; // 5 minutes interval

// error debug output utility
auto print_err = [](decltype(errno) e, const QString &msg)
{
    qDebug() << QString("Error: [%1] %2, ").arg(e).arg(strerror(e)) << msg;
};

// constructor
DesktopEntryStat::DesktopEntryStat(QObject *parent) : QObject(parent)
{
    m_timer = nullptr;
}

// create new desktop entry
DesktopEntry DesktopEntryStat::createDesktopEntry(const QString &desktopFile)
{
    if (desktopFile.isEmpty()) {
        return {};
    }

    // desktop entry instance
    DDesktopEntry entry(desktopFile);
    auto re = DesktopEntry(new desktop_entry {});

    // display name
    re->displayName = entry.ddeDisplayName();
    // icon
    auto icon = entry.stringValue("Icon");
    // use default icon if nonexists
    if (!icon.isEmpty()) {
        re->icon = QIcon::fromTheme(icon);
    }

    // use tryexec & exec, replace app name with exec name if available
    auto tryExec = entry.stringValue("TryExec");
    auto exec = entry.stringValue("Exec");
    if (!tryExec.isEmpty()) {
        re->exec = tryExec.split(' ');
        re->name = QFileInfo(re->exec[0]).baseName();
    } else if (!exec.isEmpty()) {
        re->exec = exec.split(' ');
        re->name = QFileInfo(re->exec[0]).baseName();
    } else {
        re->name = entry.name();
    }

    // startup wm class
    auto wmclass = entry.stringValue("StartupWMClass");
    if (!wmclass.isEmpty() && !QString::compare(wmclass, QFileInfo(desktopFile).fileName(), Qt::CaseInsensitive)) {
        re->startup_wm_class = wmclass;
        re->name = wmclass;
    }

    return re;
}

// start cache update timer
void DesktopEntryStat::start()
{
    if(m_timer) {
        delete m_timer;
        m_timer = nullptr;
    }
    m_timer = new QTimer{};
    // update interval
    m_timer->setInterval(kUpdateInterval);
    connect(m_timer, &QTimer::timeout, this, &DesktopEntryStat::refresh);
    refresh();
    m_timer->start();
}

// timer refresh slot
void DesktopEntryStat::refresh()
{
    DesktopEntryCache cache {};
    DIR *dir {};
    struct dirent *dp;

    // open /usr/share/applications dir
    errno = 0;
    dir = opendir(DESKTOP_ENTRY_PATH);
    if (!dir) {
        print_err(errno, QString("open %1 failed").arg(DESKTOP_ENTRY_PATH));
        return;
    }

    // iterate /usr/share/applications dir, process each file ends with '.desktop'
    errno = 0;
    while ((dp = readdir(dir))) {
        if (dp->d_type == DT_REG) {
            QString name(dp->d_name);

            if (!name.endsWith(".desktop"))
                continue;

            auto path = QString("%1/%2").arg(DESKTOP_ENTRY_PATH).arg(name);

            // create new entry
            auto e = DesktopEntryStat::createDesktopEntry(path);
            // ignore duplicated instace
            if (cache.contains(e->name) && !QString::compare(e->name, name, Qt::CaseInsensitive)) {
                continue;
            } else {
                cache[e->name] = e;
            }
        }
    }
    // failed found one
    if (errno && !cache.size()) {
        print_err(errno, QString("read %1 failed").arg(DESKTOP_ENTRY_PATH));
    }
    closedir(dir);

    // emit cache updated signal
    if (cache.size() > 0) {
        Q_EMIT cacheUpdated(cache);
    }
}
