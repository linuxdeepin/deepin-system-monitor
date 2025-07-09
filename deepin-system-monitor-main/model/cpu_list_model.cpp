// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cpu_list_model.h"
#include "cpu_stat_model.h"
#include "ddlog.h"

using namespace DDLog;

CPUListModel::CPUListModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_cpuInfoDB {}
    , m_statModelDB {}
    , m_nr {0}
{
    qCDebug(app) << "CPUListModel constructor";
}

int CPUListModel::rowCount(const QModelIndex &) const
{
    // qCDebug(app) << "CPUListModel rowCount:" << m_statModelDB.count();
    return m_statModelDB.count();
}

int CPUListModel::columnCount(const QModelIndex &) const
{
    // qCDebug(app) << "CPUListModel columnCount:" << kPropCount;
    return kPropCount;
}

QVariant CPUListModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid()) {
        qCDebug(app) << "CPUListModel::data invalid index";
        return {};
    }

    int row = idx.row();
    if (row < 0 || row >= m_statModelDB.count()) {
        qCDebug(app) << "CPUListModel::data row out of bounds";
        return {};
    }

    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole) {
        // qCDebug(app) << "CPUListModel::data Display/Accessible role for row" << row << "col"
        //              << idx.column();
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
        // qCDebug(app) << "CPUListModel::data Value role for row" << row << "col" << idx.column();
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
    qCDebug(app) << "CPUListModel::fetchMore";
    if (parent.isValid()) {
        qCDebug(app) << "parent is valid, returning";
        return;
    }

    // fetch at most 32 items at a time
    int left = m_statModelDB.count() - m_nr;
    int more = qMin(32, left);

    if (more <= 0) {
        qCDebug(app) << "no more items to fetch";
        return;
    }

    qCDebug(app) << "fetching" << more << "more items";
    beginInsertRows(QModelIndex(), m_nr, m_nr + more - 1);
    m_nr += more;
    endInsertRows();
}

bool CPUListModel::canFetchMore(const QModelIndex &parent) const
{
    qCDebug(app) << "CPUListModel::canFetchMore";
    if (parent.isValid())
        return false;

    return (m_nr < m_statModelDB.count());
}
