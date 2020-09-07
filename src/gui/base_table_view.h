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

#ifndef BASE_TABLE_VIEW_H
#define BASE_TABLE_VIEW_H

#include <DTreeView>

DWIDGET_USE_NAMESPACE

class BaseItemDelegate;
class BaseHeaderView;
class QModelIndex;

class BaseTableView : public DTreeView
{
    Q_OBJECT

public:
    BaseTableView(DWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void drawRow(QPainter *painter, const QStyleOptionViewItem &options,
                 const QModelIndex &index) const override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
    bool viewportEvent(QEvent *event) override;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;

private:
    BaseItemDelegate *m_itemDelegate {nullptr};
    BaseHeaderView *m_headerView {nullptr};

    QModelIndex m_hover;
    QModelIndex m_pressed;
};

#endif  // BASE_TABLE_VIEW_H
