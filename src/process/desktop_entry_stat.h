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

#ifndef DESKTOP_ENTRY_STAT_H
#define DESKTOP_ENTRY_STAT_H

#include <QObject>
#include <QHash>
#include <QIcon>

class QTimer;

/**
 * @brief Desktop entry struct
 */
struct desktop_entry {
    QString name; // entry name (localized name)
    QString displayName; // entry display name
    QStringList exec; // entry exec
    QIcon icon; // entry icon
    QString startup_wm_class; // entry startup wm class
};

// Desktop entry
using DesktopEntry = QSharedPointer<struct desktop_entry>;
// Desktop entry cache
using DesktopEntryCache = QHash<QString, DesktopEntry>;

/**
 * @brief Desktop entry cache
 */
class DesktopEntryStat : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Entry cache constructor
     * @param parent Parent object
     */
    explicit DesktopEntryStat(QObject *parent = nullptr);
    /**
     * @brief Create new desktop entry
     * @param desktopFile Desktop entry file path
     * @return Desktop entry created
     */
    static DesktopEntry createDesktopEntry(const QString &desktopFile);

Q_SIGNALS:
    /**
     * @brief Desktop entry cache updated signal
     * @param cache Refreshed desktop entry cache
     */
    void cacheUpdated(DesktopEntryCache cache);

public Q_SLOTS:
    /**
     * @brief Start cache update timer
     */
    void start();
    /**
     * @brief Refresh entry cache
     */
    void refresh();

private:
    // Cache update timer
    QTimer *m_timer {};
};

#endif // DESKTOP_ENTRY_STAT_H
