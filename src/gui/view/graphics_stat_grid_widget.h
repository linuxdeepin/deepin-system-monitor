/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     tenghuan <tenghuan@uniontech.com>
*
* Maintainer: tenghuan <tenghuan@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef GRAPHICS_STAT_GRID_WIDGET_H
#define GRAPHICS_STAT_GRID_WIDGET_H

#include <QGraphicsWidget>

class GraphicsStatGridWidget: public QGraphicsWidget {
    Q_OBJECT

public:
    GraphicsStatGridWidget(int row = 0,int column = 0,int radius = 10,QGraphicsWidget *parent = nullptr);
    int row(); // 获得当前行数
    int column(); // 获得当前列数
    void setRadius(int radius); // 设置圆角大小
    void setColor(const QColor& color); // 设置背景颜色

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
private:
    int m_irow; // 行数
    int m_icolumn;// 列数
    int m_iRadius = 10;// 圆角大小
    QColor m_color = Qt::green;// 背景色
};

#endif // GRAPHICS_STAT_GRID_WIDGET_H
