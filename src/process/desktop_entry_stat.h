/*
 * Copyright (C) 2019 ~ 2019 Union Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@uniontech.com>
 *
 * Maintainer: zccrs <zhangjide@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef DESKTOP_ENTRY_STAT_H
#define DESKTOP_ENTRY_STAT_H

#include <QHash>
#include <QTimer>
#include <QIcon>

struct desktop_entry {
    QString     name;
    QString     displayName;
    QStringList exec;
    QIcon       icon;
    QString     startup_wm_class;
};

using DesktopEntry = QSharedPointer<struct desktop_entry>;
using DesktopEntryCache = QHash<QString, DesktopEntry>;

class DesktopEntryStat : public QObject
{
    Q_OBJECT
public:
    explicit DesktopEntryStat(QObject *parent = nullptr);
    static DesktopEntry createDesktopEntry(const QString &desktopFile);

Q_SIGNALS:
    void cacheUpdated(DesktopEntryCache cache);

public Q_SLOTS:
    void start();
    void refresh();

private:
    QTimer *m_timer {};
};

#endif // DESKTOP_ENTRY_STAT_H
