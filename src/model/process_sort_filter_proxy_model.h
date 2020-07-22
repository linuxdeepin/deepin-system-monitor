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

#ifndef PROCESS_SORT_FILTER_PROXY_MODEL_H
#define PROCESS_SORT_FILTER_PROXY_MODEL_H

#include <QSortFilterProxyModel>

class ProcessSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ProcessSortFilterProxyModel(QObject *parent = nullptr);

    void setSortFilterString(const QString &search);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool filterAcceptsColumn(int column, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString m_search {};
    QString m_hanwords {};
    QString m_capwords {};
};

#endif  // PROCESS_SORT_FILTER_PROXY_MODEL_H
