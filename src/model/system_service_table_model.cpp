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

#include "system_service_table_model.h"

#include "service/service_manager.h"
#include "utils.h"

#include <DApplication>

#include <QColor>
#include <QDebug>
#include <QFont>
#include <QFontMetrics>

DWIDGET_USE_NAMESPACE
using namespace Utils;

SystemServiceTableModel::SystemServiceTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    auto *mgr = ServiceManager::instance();
    Q_ASSERT(mgr);
    connect(mgr, &ServiceManager::serviceListUpdated, this, &SystemServiceTableModel::updateServiceList);
    connect(mgr, &ServiceManager::serviceStatusUpdated, this, &SystemServiceTableModel::updateServiceEntry);
}

void SystemServiceTableModel::updateServiceEntry(const SystemServiceEntry &entry)
{
    auto sname = entry.getSName();
    if (m_svcMap.contains(sname)) {
        for (auto row = 0; row < m_svcList.size(); row++) {
            if (m_svcList[row] == sname) {
                m_svcMap[sname] = entry;
                Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
                break;
            }
        }
    } else {
        auto row = m_svcList.size();
        beginInsertRows({}, row, row);
        m_svcList << sname;
        m_svcMap[sname] = entry;
        endInsertRows();
        Q_EMIT currentRowChanged(row);
    }
}

QVariant SystemServiceTableModel::data(const QModelIndex &index, int role) const
{
    QString buf {};
    auto row = index.row();

    if (!index.isValid() || !(row >= 0 && row < m_svcList.size())) {
        return {};
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case kSystemServiceNameColumn:
            return m_svcMap[m_svcList[row]].getSName();
        case kSystemServiceLoadStateColumn: {
            buf = m_svcMap[m_svcList[row]].getLoadState();
            return DApplication::translate("DBus.Unit.Load.State", buf.toUtf8());
        }
        case kSystemServiceActiveStateColumn: {
            buf = m_svcMap[m_svcList[row]].getActiveState();
            return DApplication::translate("DBus.Unit.Active.State", buf.toUtf8());
        }
        case kSystemServiceSubStateColumn:
            return m_svcMap[m_svcList[row]].getSubState();
        case kSystemServiceStateColumn: {
            buf = m_svcMap[m_svcList[row]].getState();
            return DApplication::translate("DBus.Unit.State", buf.toUtf8());
        }
        case kSystemServiceStartupModeColumn: {
            auto stype = m_svcMap[m_svcList[row]].getStartupType();
            return DApplication::translate("DBus.Unit.Startup.Mode", stype.toUtf8());
        }
        case kSystemServiceDescriptionColumn:
            return m_svcMap[m_svcList[row]].getDescription();
        case kSystemServicePIDColumn: {
            auto pid = m_svcMap[m_svcList[row]].getMainPID();
            auto va = (pid == 0) ? QVariant() : QVariant(pid);
            return va;
        }
        default:
            break;
        }
    } else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    }
    return {};
}

int SystemServiceTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_nr;
}
int SystemServiceTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return kSystemServiceTableColumnCount;
}
QVariant SystemServiceTableModel::headerData(int section, Qt::Orientation orientation,
                                             int role) const
{
    if (role == Qt::DisplayRole) {
        switch (section) {
        case kSystemServiceNameColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceName);
        case kSystemServiceLoadStateColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceLoadState);
        case kSystemServiceActiveStateColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceActiveState);
        case kSystemServiceSubStateColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceSubState);
        case kSystemServiceStateColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceState);
        case kSystemServiceDescriptionColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceDescription);
        case kSystemServicePIDColumn:
            return DApplication::translate("Service.Table.Header", kSystemServicePID);
        case kSystemServiceStartupModeColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceStartupMode);
        default:
            break;
        }
    } else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::InitialSortOrderRole) {
        return QVariant::fromValue(Qt::DescendingOrder);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags SystemServiceTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void SystemServiceTableModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid())
        return;

    int left = m_svcList.size() - m_nr;
    int more = qMin(100, left);

    if (more <= 0)
        return;

    beginInsertRows(QModelIndex(), m_nr, m_nr + more - 1);
    m_nr += more;
    endInsertRows();
}

bool SystemServiceTableModel::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid())
        return false;

    return (m_nr < m_svcList.size());
}

void SystemServiceTableModel::updateServiceList(const QList<SystemServiceEntry> &list)
{
    beginResetModel();
    m_svcList.clear();
    m_svcMap.clear();
    m_nr = 0;
    for (auto &ent : list) {
        m_svcList << ent.getSName();
        m_svcMap[ent.getSName()] = ent;
    }
    endResetModel();
}
