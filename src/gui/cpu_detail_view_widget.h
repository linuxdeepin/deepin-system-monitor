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
#ifndef CPU_DETAIL_VIEW_WIDGET_H
#define CPU_DETAIL_VIEW_WIDGET_H

#include <QWidget>

class QTableWidget;
/**
 * @brief CPU detail view widget
 */
class CPUDetailViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CPUDetailViewWidget(QWidget *parent = nullptr);

private:
    QWidget      *m_graphicsTable;
    QTableWidget *m_textTable;
};
//显示cpu的核心的折线图表
class CPUCoreItem : public QWidget
{
    Q_OBJECT
public:
    CPUCoreItem(QWidget *parent): QWidget(parent) {}

    void paintEvent(QPaintEvent *event);

};
#endif // CPU_DETAIL_VIEW_WIDGET_H
