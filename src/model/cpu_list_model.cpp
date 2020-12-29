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
#include "cpu_list_model.h"

CPUListModel::CPUListModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_cpuInfoDB {}
    , m_statModelDB {}
    , m_nr {0}
{
}

int CPUListModel::rowCount(const QModelIndex &) const
{
    return m_statModelDB.count();
}

int CPUListModel::columnCount(const QModelIndex &) const
{
    return kPropCount;
}

QVariant CPUListModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid())
        return {};

    int row = idx.row();
    if (row < 0 || row >= m_statModelDB.count())
        return {};

    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole) {
        switch (idx.column()) {
        case kPropCPUIndex: {
            return m_cpuInfoDB[row].logicalIndex();
        }
        case kPropCPUUsage: {
            auto cpu = m_cpuInfoDB[row];
            auto stat = m_statModelDB[cpu.logicalName()];
            // only most recent sample cared here
            auto total = stat->data(index(0, 0), CPUStatModel::kStatTotalUsage).toULongLong();
            auto idle = stat->data(index(0, 0), CPUStatModel::kStatIdleUsage).toULongLong();
            return QString("%1%2").arg((total - idle) * 1. / total * 100).arg('%');
        }
        default:
            break;
        } // ::switch
    } // ::if(displayRole || accessibleRole)

    else if (role == kValueRole) {
        switch (idx.column()) {
        case kPropCPUUsage: {
            auto cpu = m_cpuInfoDB[row];
            auto stat = m_statModelDB[cpu.logicalName()];
            // only most recent sample cared here
            auto total = stat->data(index(0, 0), CPUStatModel::kStatTotalUsage).toULongLong();
            auto idle = stat->data(index(0, 0), CPUStatModel::kStatIdleUsage).toULongLong();
            return (total - idle) * 1. / total;
        }
        default:
            break;
        } // ::switch

    } // ::if(valueRole)

    return {};
}

void CPUListModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid())
        return;

    // fetch at most 32 items at a time
    int left = m_statModelDB.count() - m_nr;
    int more = qMin(32, left);

    if (more <= 0)
        return;

    beginInsertRows(QModelIndex(), m_nr, m_nr + more - 1);
    m_nr += more;
    endInsertRows();
}

bool CPUListModel::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid())
        return false;

    return (m_nr < m_statModelDB.count());
}
