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

#ifndef PROCESS_SORT_FILTER_PROXY_MODEL_H
#define PROCESS_SORT_FILTER_PROXY_MODEL_H

#include <QSortFilterProxyModel>

/**
 * @brief Sort filter proxy model for process model
 */
class ProcessSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    /**
     * @brief Proxy model constructor
     * @param parent Parent object
     */
    explicit ProcessSortFilterProxyModel(QObject *parent = nullptr);

    /**
     * @brief Set search pattern
     * @param search Search pattern
     */
    void setSortFilterString(const QString &search);

    void setFilterType(int type);

protected:
    /**
     * @brief Filters the row of specified parent with given pattern
     * @param row The row to be filtered
     * @param parent Parent index of the row
     * @return Returns true if the item in the row indicated by the given row and parent should be included in the model; otherwise returns false
     */
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;

    /**
     * @brief Compare two items with the specified index
     * @param left Index of the item to be compared on left side
     * @param right Index of the item to be compared on right side
     * @return Returns true if the value of the item referred to by the given index left is less than the value of the item referred to by the given index right, otherwise returns false
     */
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    // Search pattern
    QString m_search {};
    // Pinyin represented as ascii string converted from chinese hanzi
    QString m_hanwords {};

    int m_fileterType = 0;
};

#endif  // PROCESS_SORT_FILTER_PROXY_MODEL_H
