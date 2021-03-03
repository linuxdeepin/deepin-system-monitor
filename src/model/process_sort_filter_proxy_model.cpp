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

#include "process_sort_filter_proxy_model.h"

#include "process_table_model.h"

#include "common/collator.h"
#include "common/han_latin.h"

#include <QCollator>
#include <QDebug>
#include <QLocale>

// proxy model constructor
ProcessSortFilterProxyModel::ProcessSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

// set search pattern
void ProcessSortFilterProxyModel::setSortFilterString(const QString &search)
{
    m_search = search;

    // in chinese locale, we convert hanzi to pinyin words to help filter out processes named with pinyin
    if (QLocale::system().language() == QLocale::Chinese) {
        m_hanwords = util::common::convHanToLatin(search);
        if (m_hanwords.isEmpty()) {
            m_capwords = QString {};
        } else {
            QStringList wlist = m_hanwords.split(QRegExp("\\s+"));

            // take each initial character of pinyin, and concate each as a whole string
            for (const QString &w : wlist) {
                if (w.isEmpty())
                    continue;

                m_capwords += w[0];
            }
        }
    }

    // set search pattern & do the filter
    setFilterRegExp(QRegExp(search, Qt::CaseInsensitive));
}

// filters the row of specified parent with given pattern
bool ProcessSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    // name, displayName, pid, user, pinyin
    QModelIndex name = sourceModel()->index(row, ProcessTableModel::kProcessNameColumn, parent);
    QModelIndex pid = sourceModel()->index(row, ProcessTableModel::kProcessPIDColumn, parent);
    QModelIndex user = sourceModel()->index(row, ProcessTableModel::kProcessUserColumn, parent);

    bool rc = false;
    // display name or name matches pattern
    if (name.isValid()) {
        rc |= sourceModel()->data(name).toString().contains(filterRegExp());
        rc |= sourceModel()->data(name, Qt::UserRole).toString().contains(filterRegExp());
        if (QLocale::system().language() == QLocale::Chinese) {
            // pinyin matches pattern
            rc |= sourceModel()->data(name, Qt::UserRole).toString().contains(m_hanwords);
        }
        //        rc |= sourceModel()->data(name).toString().contains(m_capwords);
    }
    // pid matches pattern
    if (pid.isValid())
        rc |= sourceModel()->data(pid).toString().contains(filterRegExp());
    // user name matches pattern
    if (user.isValid())
        rc |= sourceModel()->data(user).toString().contains(filterRegExp());

    return rc;
}

// filters the column of specified parent with given pattern
bool ProcessSortFilterProxyModel::filterAcceptsColumn(int column, const QModelIndex &parent) const
{
    // no column filters here
    return QSortFilterProxyModel::filterAcceptsColumn(column, parent);
}

// compare two items with the specified index
bool ProcessSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    int sortcolumn = sortColumn();
    switch (sortcolumn) {
    case ProcessTableModel::kProcessNameColumn: {
        const QString &lhs = left.data(Qt::DisplayRole).toString();
        const QString &rhs = right.data(Qt::DisplayRole).toString();

        bool lstartHz = Utils::startWithHanzi(lhs);
        bool rstartHz = Utils::startWithHanzi(rhs);
        if (!lstartHz && rstartHz)
            return true;

        if (lstartHz && !rstartHz)
            return false;

        int rc = lhs.localeAwareCompare(rhs);

        if (rc == 0) {
            return left.sibling(left.row(), ProcessTableModel::kProcessCPUColumn)
                   .data(Qt::UserRole) <
                   right.sibling(right.row(), ProcessTableModel::kProcessCPUColumn)
                   .data(Qt::UserRole);
        } else
            return rc < 0;
    }
    case ProcessTableModel::kProcessUserColumn: {
        const QString &lhs = left.data(Qt::DisplayRole).toString();
        const QString &rhs = right.data(Qt::DisplayRole).toString();
        return lhs.localeAwareCompare(rhs) < 0;
    }
    case ProcessTableModel::kProcessMemoryColumn: {
        const QVariant &lmem = left.data(Qt::UserRole);
        const QVariant &rmem = right.data(Qt::UserRole);

        // compare memory usage first, then by cpu time
        if (lmem == rmem) {
            return left.sibling(left.row(), ProcessTableModel::kProcessCPUColumn)
                   .data(Qt::UserRole) <
                   right.sibling(right.row(), ProcessTableModel::kProcessCPUColumn)
                   .data(Qt::UserRole);
        } else {
            return lmem < rmem;
        }
    }
    case ProcessTableModel::kProcessCPUColumn: {
        const QVariant &lcpu = left.data(Qt::UserRole);
        const QVariant &rcpu = right.data(Qt::UserRole);

        const QVariant &lmem = left.sibling(left.row(), ProcessTableModel::kProcessMemoryColumn).data(Qt::UserRole);
        const QVariant &rmem = right.sibling(right.row(), ProcessTableModel::kProcessMemoryColumn).data(Qt::UserRole);

        // compare cpu time first, then by memory usage
        if (qFuzzyCompare(lcpu.toReal(), rcpu.toReal())) {
            return lmem < rmem;
        } else {
            return lcpu < rcpu;
        }
    }
    case ProcessTableModel::kProcessUploadColumn: {
        qreal lkbs, rkbs;
        lkbs = left.data(Qt::UserRole).toReal();
        rkbs = right.data(Qt::UserRole).toReal();

        // compare upload speed first, then by total send bytes
        if (qFuzzyCompare(lkbs, rkbs)) {
            return left.data(Qt::UserRole + 1).toULongLong() < right.data(Qt::UserRole + 1).toULongLong();
        } else {
            return lkbs < rkbs;
        }
    }
    case ProcessTableModel::kProcessDownloadColumn: {
        qreal lkbs = left.data(Qt::UserRole).toReal();
        qreal rkbs = right.data(Qt::UserRole).toReal();

        // compare download speed first, then by total download bytes
        if (qFuzzyCompare(lkbs, rkbs)) {
            return left.data(Qt::UserRole + 1).toULongLong() < right.data(Qt::UserRole + 1).toULongLong();
        } else {
            return lkbs < rkbs;
        }
    }
    case ProcessTableModel::kProcessPIDColumn: {
        // compare pid
        return left.data(Qt::UserRole) < right.data(Qt::UserRole);
    }
    case ProcessTableModel::kProcessDiskReadColumn: {
        // compare disk read speed
        return left.data(Qt::UserRole) < right.data(Qt::UserRole);
    }
    case ProcessTableModel::kProcessDiskWriteColumn: {
        // compare disk write speed
        return left.data(Qt::UserRole) < right.data(Qt::UserRole);
    }
    case ProcessTableModel::kProcessNiceColumn: {
        // higher priority has negative number
        return !(left.data(Qt::UserRole) < right.data(Qt::UserRole));
    }
    case ProcessTableModel::kProcessPriorityColumn: {
        // compare priority with nice value instead of nice display name
        return !(left.sibling(left.row(), ProcessTableModel::kProcessNiceColumn).data(Qt::UserRole) < right.sibling(right.row(), ProcessTableModel::kProcessNiceColumn).data(Qt::UserRole));
    }
    default:
        break;
    }

    return QSortFilterProxyModel::lessThan(left, right);
}
