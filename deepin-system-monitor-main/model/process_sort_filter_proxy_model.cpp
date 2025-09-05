// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process_sort_filter_proxy_model.h"
#include "ddlog.h"
#include "process/process_db.h"
#include "process_table_model.h"
#include "common/han_latin.h"
#include "common/common.h"

#include <QCollator>
#include <QDebug>
#include <QLocale>

// proxy model constructor
ProcessSortFilterProxyModel::ProcessSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    qCDebug(app) << "ProcessSortFilterProxyModel created";
}

// set search pattern
void ProcessSortFilterProxyModel::setSortFilterString(const QString &search)
{
    qCDebug(app) << "Set sort filter string:" << search;
    m_search = search;

    // in chinese locale, we convert hanzi to pinyin words to help filter out processes named with pinyin
    if (QLocale::system().language() == QLocale::Chinese) {
        qCDebug(app) << "Chinese locale, converting Hanzi to Latin";
        m_hanwords = util::common::convHanToLatin(search);
    }

    // set search pattern & do the filter
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    setFilterRegExp(QRegExp(search, Qt::CaseInsensitive));
#else
    setFilterRegularExpression(QRegularExpression(search, QRegularExpression::CaseInsensitiveOption));
#endif
}

void ProcessSortFilterProxyModel::setFilterType(int type)
{
    qCDebug(app) << "Set filter type:" << type;
    m_fileterType = type;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    setFilterRegExp(filterRegExp());
#else
    setFilterRegularExpression(filterRegularExpression());
#endif
}

// filters the row of specified parent with given pattern
bool ProcessSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    if (!sourceModel() || row < 0 || row >= sourceModel()->rowCount(parent)) {
        return false;
    }
    // qCDebug(app) << "Filtering row" << row;
    bool filter = false;
    const QModelIndex &pid = sourceModel()->index(row, ProcessTableModel::kProcessPIDColumn, parent);
    int apptype = pid.data(Qt::UserRole + 3).toInt();
    if (m_fileterType == kNoFilter) {
        qCDebug(app) << "No filter applied";
        filter = true;
    } else if (m_fileterType == kFilterCurrentUser && (apptype == kFilterApps || apptype == kFilterCurrentUser)) {
        qCDebug(app) << "Filter for current user";
        filter = true;
    } else if (m_fileterType == kFilterApps && apptype == kFilterApps) {
        qCDebug(app) << "Filter for applications";
        filter = true;
    }

    if (!filter) {
        qCDebug(app) << "Row" << row << "filtered out by type";
        return false;
    }

    bool rc = false;
    const QModelIndex &name = sourceModel()->index(row, ProcessTableModel::kProcessNameColumn, parent);
    // display name or name matches pattern
    if (name.isValid()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        rc |= name.data().toString().contains(filterRegExp());
#else
        rc |= name.data().toString().contains(filterRegularExpression());
#endif
        if (rc) {
            qCDebug(app) << "Filter accepted by name:" << name.data().toString();
            return rc;
        }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        rc |= name.data(Qt::UserRole).toString().contains(filterRegExp());
#else
        rc |= name.data(Qt::UserRole).toString().contains(filterRegularExpression());
#endif
        if (rc) {
            qCDebug(app) << "Filter accepted by user role name:" << name.data(Qt::UserRole).toString();
            return rc;
        }

        if (QLocale::system().language() == QLocale::Chinese) {
            qCDebug(app) << "Filter checking pinyin match";
            // pinyin matches pattern
            rc |= name.data(Qt::UserRole).toString().contains(m_hanwords);
            if (rc) {
                qCDebug(app) << "Filter accepted by pinyin match";
                return rc;
            }
        }
    }

    // pid matches pattern
    if (pid.isValid()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        rc |= pid.data().toString().contains(filterRegExp());
#else
        rc |= pid.data().toString().contains(filterRegularExpression());
#endif
    }
    if (rc) {
        qCDebug(app) << "Filter accepted by PID:" << pid.data().toString();
        return rc;
    }

    // user name matches pattern
    const QModelIndex &user = sourceModel()->index(row, ProcessTableModel::kProcessUserColumn, parent);
    if (user.isValid()) {   
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        rc |= user.data().toString().contains(filterRegExp());
#else
        rc |= user.data().toString().contains(filterRegularExpression());
#endif
        if (rc) qCDebug(app) << "Filter accepted by user name:" << user.data().toString();
    }

    return rc;
}

// compare two items with the specified index
bool ProcessSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (!left.isValid() || !right.isValid() || !left.model() || !right.model()) {
        return false;
    }
    qCDebug(app) << "lessThan sorting on column:" << sortColumn();
    int sortcolumn = sortColumn();
    switch (sortcolumn) {
    case ProcessTableModel::kProcessNameColumn: {
        qCDebug(app) << "Sorting by process name";
        const QString &lhs = left.data(Qt::DisplayRole).toString();
        const QString &rhs = right.data(Qt::DisplayRole).toString();

        bool lstartHz = common::startWithHanzi(lhs);
        bool rstartHz = common::startWithHanzi(rhs);
        if (!lstartHz && rstartHz) {
            qCDebug(app) << "lhs does not start with Hanzi, rhs does, lhs is less";
            return true;
        }

        if (lstartHz && !rstartHz) {
            qCDebug(app) << "lhs starts with Hanzi, rhs does not, rhs is less";
            return false;
        }

        int rc = lhs.localeAwareCompare(rhs);

        if (rc == 0) {
            qCDebug(app) << "Process names are equal, comparing by CPU";
            return left.sibling(left.row(), ProcessTableModel::kProcessCPUColumn)
                   .data(Qt::UserRole).toDouble() <
                   right.sibling(right.row(), ProcessTableModel::kProcessCPUColumn)
                   .data(Qt::UserRole).toDouble();
        } else
            return rc < 0;
    }
    case ProcessTableModel::kProcessUserColumn: {
        qCDebug(app) << "Sorting by user name";
        const QString &lhs = left.data(Qt::DisplayRole).toString();
        const QString &rhs = right.data(Qt::DisplayRole).toString();
        return lhs.localeAwareCompare(rhs) < 0;
    }
    case ProcessTableModel::kProcessMemoryColumn:
    case ProcessTableModel::kProcessShareMemoryColumn:
    case ProcessTableModel::kProcessVTRMemoryColumn: {
        qCDebug(app) << "Sorting by memory";
        const QVariant &lmem = left.data(Qt::UserRole);
        const QVariant &rmem = right.data(Qt::UserRole);

        // compare memory usage first, then by cpu time
        if (lmem == rmem) {
            qCDebug(app) << "Memory is equal, comparing by CPU";
            return left.sibling(left.row(), ProcessTableModel::kProcessCPUColumn)
                   .data(Qt::UserRole).toDouble() <
                   right.sibling(right.row(), ProcessTableModel::kProcessCPUColumn)
                   .data(Qt::UserRole).toDouble();
        } else {
            return lmem.toULongLong() < rmem.toULongLong();
        }
    }
    case ProcessTableModel::kProcessCPUColumn: {
        qCDebug(app) << "Sorting by CPU";
        const QVariant &lcpu = left.data(Qt::UserRole);
        const QVariant &rcpu = right.data(Qt::UserRole);

        const QVariant &lmem = left.sibling(left.row(), ProcessTableModel::kProcessMemoryColumn).data(Qt::UserRole);
        const QVariant &rmem = right.sibling(right.row(), ProcessTableModel::kProcessMemoryColumn).data(Qt::UserRole);

        // compare cpu time first, then by memory usage
        if (qFuzzyCompare(lcpu.toReal(), rcpu.toReal())) {
            qCDebug(app) << "CPU is equal, comparing by memory";
            return lmem.toULongLong() < rmem.toULongLong();
        } else {
            return lcpu.toDouble() < rcpu.toDouble();
        }
    }
    case ProcessTableModel::kProcessUploadColumn: {
        qCDebug(app) << "Sorting by upload speed";
        qreal lkbs, rkbs;
        lkbs = left.data(Qt::UserRole).toReal();
        rkbs = right.data(Qt::UserRole).toReal();

        // compare upload speed first, then by total send bytes
        if (qFuzzyCompare(lkbs, rkbs)) {
            qCDebug(app) << "Upload speed is equal, comparing by total sent bytes";
            return left.data(Qt::UserRole + 1).toULongLong() < right.data(Qt::UserRole + 1).toULongLong();
        } else {
            return lkbs < rkbs;
        }
    }
    case ProcessTableModel::kProcessDownloadColumn: {
        qCDebug(app) << "Sorting by download speed";
        qreal lkbs = left.data(Qt::UserRole).toReal();
        qreal rkbs = right.data(Qt::UserRole).toReal();

        // compare download speed first, then by total download bytes
        if (qFuzzyCompare(lkbs, rkbs)) {
            qCDebug(app) << "Download speed is equal, comparing by total download bytes";
            return left.data(Qt::UserRole + 1).toULongLong() < right.data(Qt::UserRole + 1).toULongLong();
        } else {
            return lkbs < rkbs;
        }
    }
    case ProcessTableModel::kProcessPIDColumn: {
        qCDebug(app) << "Sorting by PID";
        // compare pid
        return left.data(Qt::UserRole).toInt() < right.data(Qt::UserRole).toInt();
    }
    case ProcessTableModel::kProcessDiskReadColumn: {
        qCDebug(app) << "Sorting by disk read speed";
        // compare disk read speed
        return left.data(Qt::UserRole).toDouble() < right.data(Qt::UserRole).toDouble();
    }
    case ProcessTableModel::kProcessDiskWriteColumn: {
        qCDebug(app) << "Sorting by disk write speed";
        // compare disk write speed
        return left.data(Qt::UserRole).toDouble() < right.data(Qt::UserRole).toDouble();
    }
    case ProcessTableModel::kProcessNiceColumn: {
        qCDebug(app) << "Sorting by nice value";
        // higher priority has negative number
        return !(left.data(Qt::UserRole).toInt() < right.data(Qt::UserRole).toInt());
    }
    case ProcessTableModel::kProcessPriorityColumn: {
        qCDebug(app) << "Sorting by priority";
        // compare priority with nice value instead of nice display name
        return !(left.sibling(left.row(), ProcessTableModel::kProcessNiceColumn).data(Qt::UserRole).toInt() < right.sibling(right.row(), ProcessTableModel::kProcessNiceColumn).data(Qt::UserRole).toInt());
    }
    default:
        qCDebug(app) << "Default sort";
        break;
    }

    return QSortFilterProxyModel::lessThan(left, right);
}
