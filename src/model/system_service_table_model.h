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
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SYSTEM_SERVICE_TABLE_MODEL_H
#define SYSTEM_SERVICE_TABLE_MODEL_H

#include "service/system_service_entry.h"

#include <QAbstractTableModel>
#include <QList>
#include <QHash>

constexpr const char *kSystemServiceName = QT_TRANSLATE_NOOP("Service.Table.Header", "Name");
constexpr const char *kSystemServiceLoadState = QT_TRANSLATE_NOOP("Service.Table.Header", "Load");
constexpr const char *kSystemServiceActiveState =
    QT_TRANSLATE_NOOP("Service.Table.Header", "Active");
//: sub state (running status)
constexpr const char *kSystemServiceSubState = QT_TRANSLATE_NOOP("Service.Table.Header", "Sub");
//: state
constexpr const char *kSystemServiceState = QT_TRANSLATE_NOOP("Service.Table.Header", "State");
//: service startup mode
constexpr const char *kSystemServiceStartupMode = QT_TRANSLATE_NOOP("Service.Table.Header", "Startup Type");
constexpr const char *kSystemServiceDescription =
    QT_TRANSLATE_NOOP("Service.Table.Header", "Description");
constexpr const char *kSystemServicePID = QT_TRANSLATE_NOOP("Service.Table.Header", "PID");

class SystemServiceEntry;

class SystemServiceTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum SystemServiceTableColumn {
        kSystemServiceNameColumn = 0,
        kSystemServiceLoadStateColumn,
        kSystemServiceActiveStateColumn,
        kSystemServiceSubStateColumn,
        kSystemServiceStateColumn,
        kSystemServiceDescriptionColumn,
        kSystemServicePIDColumn,
        kSystemServiceStartupModeColumn,

        kSystemServiceTableColumnCount
    };

    explicit SystemServiceTableModel(QObject *parent = nullptr);

    inline void reset()
    {
        beginRemoveRows({}, 0, m_svcList.size() - 1);
        m_svcList.clear();
        m_svcMap.clear();
        endRemoveRows();
    }

    void updateServiceEntry(const SystemServiceEntry &entry);

    inline QString getUnitFileState(const QModelIndex &index)
    {
        if (!index.isValid())
            return {};

        return m_svcMap[m_svcList[index.row()]].getState();
    }

    inline QString getUnitFileName(const QModelIndex &index)
    {
        if (!index.isValid())
            return {};

        return m_svcMap[m_svcList[index.row()]].getSName();
    }

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void fetchMore(const QModelIndex &parent) override;
    bool canFetchMore(const QModelIndex &parent) const override;

Q_SIGNALS:
    void currentRowChanged(int row);

private Q_SLOTS:
    void updateServiceList(const QList<SystemServiceEntry> &list);

private:
    // service name
    QList<QString>                      m_svcList   {};
    // name - entry
    QHash<QString, SystemServiceEntry>  m_svcMap    {};
    // current loaded items (into the model)
    int m_nr {};
};

#endif  // SYSTEM_SERVICE_TABLE_MODEL_H
