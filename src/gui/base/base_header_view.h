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

#ifndef BASE_HEADER_VIEW_H
#define BASE_HEADER_VIEW_H

#include <DHeaderView>

DWIDGET_USE_NAMESPACE

class QModelIndex;

/**
 * @brief Customized HeaderView
 */
class BaseHeaderView : public DHeaderView
{
    Q_OBJECT

public:
    /**
     * @brief BaseHeaderView constructor
     * @param orientation Header orientation
     * @param parent Parent Widget
     */
    explicit BaseHeaderView(Qt::Orientation orientation, QWidget *parent = nullptr);

    /**
     * @brief sizeHint Hint size of this widget
     * @return Hint size
     */
    QSize sizeHint() const override;

protected:
    /**
     * @brief paintEvent Paint event handler for header view
     * @param e Paint event object
     */
    void paintEvent(QPaintEvent *e) override;
    /**
     * @brief eventFilter Event filter for header view
     * @param obj Object being watched
     * @param ev Event to be filtered
     * @return true: Event being handled & no further processing; false: event should be propagated further
     */
    bool eventFilter(QObject *obj, QEvent *ev) override;

    /**
     * @brief focusInEvent
     */
    void focusInEvent(QFocusEvent *event) override;

private:
    int m_focusReason = Qt::TabFocusReason;
};

#endif  // BASE_HEADER_VIEW_H
