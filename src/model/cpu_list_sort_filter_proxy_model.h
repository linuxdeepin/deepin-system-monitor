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
#ifndef CPU_LIST_SORT_FILTER_PROXY_MODEL_H
#define CPU_LIST_SORT_FILTER_PROXY_MODEL_H

#include <QSortFilterProxyModel>

class CPUListSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum SortKey {
        kSortByIndex = 0,
        kSortByUsage,

        kSortKeyMax
    };

    explicit CPUListSortFilterProxyModel(QObject *parent = nullptr);
    virtual ~CPUListSortFilterProxyModel() override = default;

    void setSortKey(enum SortKey key = kSortByIndex);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool filterAcceptsColumn(int column, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    enum SortKey m_sortKey;
};

inline void CPUListSortFilterProxyModel::setSortKey(CPUListSortFilterProxyModel::SortKey key)
{
    m_sortKey = key;
}

#endif // CPU_LIST_SORT_FILTER_PROXY_MODEL_H
