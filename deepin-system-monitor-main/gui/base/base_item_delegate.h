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

#ifndef BASE_ITEM_DELEGATE_H
#define BASE_ITEM_DELEGATE_H

#include <QStyledItemDelegate>

class QModelIndex;
class QPainter;
class QStyleOptionViewItem;

/**
 * @brief Base item delegate for treeview
 */
class BaseItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    /**
     * @brief Default constructor
     * @param parent Parent object
     */
    explicit BaseItemDelegate(QObject *parent = nullptr);

    /**
     * @brief paint Paint method for this delegate
     * @param painter Painter object
     * @param option Style option for this delegate
     * @param index Current index
     */
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    /**
     * @brief createEditor Returns the editor to be used for editing the data item with the given index.
     * @param parent Parent widget
     * @param option Style option
     * @param index Current index
     * @return New editor instance
     */
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    /**
     * @brief sizeHint Size hint for this delegate
     * @param option Style option for this delegate
     * @param index Current index
     * @return Hint size
     */
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    /**
     * @brief helpEvent Function to handle help events
     * @param e Help event
     * @param view View that generated this help event
     * @param option Style option
     * @param index Current index
     * @return Returns true if the delegate can handle the event; otherwise returns false.
     */
    bool helpEvent(QHelpEvent *e,
                   QAbstractItemView *view,
                   const QStyleOptionViewItem &option,
                   const QModelIndex &index) override;

protected:
    /**
     * @brief initStyleOption
     * @param option Option to be modified
     * @param index Index to get model data
     */
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

#endif  // BASE_ITEM_DELEGATE_H
