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
#include "graphics_stat_grid_widget.h"
#include "graphics_stat_grid_item.h"
#include <QGraphicsGridLayout>
#include <QPainter>
#include <QDebug>

GraphicsStatGridWidget::GraphicsStatGridWidget(int row,int column,int radius,QGraphicsWidget *parent) : QGraphicsWidget(parent)
{
    m_irow = row; // 设置行数
    m_icolumn = column; // 设置列数
    m_iRadius = radius; // 设置圆角大小
    this->setPos(0,0); // 初始化位置
    QGraphicsGridLayout *grid = new QGraphicsGridLayout;
    for(int i =0; i < row; ++i)
    {
        for(int si =0; si < column; ++si)
        {
            GraphicsStatGridItem * item = nullptr;
            if(si == 0 && i == 0)
            {
                 item = new GraphicsStatGridItem(QRect(0,0,200,30),"统信软件","你好",GraphicsStatGridItem::None); // 左上
            }
            else if(i == 0 && si == column - 1)
            {
                 item = new GraphicsStatGridItem(QRect(0,0,200,30),"统信软件","你好",GraphicsStatGridItem::None); // 右上
            }
            else if(i == row-1 && si == 0)
            {
                 item = new GraphicsStatGridItem(QRect(0,0,200,30),"统信软件","你好",GraphicsStatGridItem::None); // 左下
            }
            else if(i == row-1 && si == column - 1)
            {
                 item = new GraphicsStatGridItem(QRect(0,0,200,30),"统信软件","你好",GraphicsStatGridItem::None);// 右下
            }
            else
            {
                item = new GraphicsStatGridItem(QRect(0,0,200,30),"统信软件","你好",GraphicsStatGridItem::None); // 普通item
            }

            if(i%2 == 1)
            {
                item->setBackground(/*QColor::fromRgb(247,247,247)*/Qt::lightGray); // 双数行的话 设置背景
            }
           // grid->setSpacing(-13);
            item->setRadius(m_iRadius); // 设置有圆角Item的圆角大小
            grid->addItem(item, i, si); // 将item放到布局器中
        }
    }
    grid->setContentsMargins(1,1,1,1); // 设置margin
    grid->setSpacing(0);
    grid->setHorizontalSpacing(1);
    setLayout(grid);


 }
// 设置背景颜色
void GraphicsStatGridWidget::setColor(const QColor& color)
{
    m_color = color;
}
// 设置圆角弧度
void GraphicsStatGridWidget::setRadius(int radius)
{
    m_iRadius = radius;
}

// 画图
 void GraphicsStatGridWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
 {
  //  QPen framePen;
   // framePen.setWidth(1);
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    path.addRoundedRect(this->contentsRect(),10,10);
    painter->setClipPath(path);

    painter->fillPath(path, QBrush(m_color)); // 填充path 得到一个填充的圆角矩形


    QGraphicsWidget::paint(painter,option,widget);
 }
 int GraphicsStatGridWidget::row() // 获得行
 {
      return m_irow;
 }
 int GraphicsStatGridWidget::column() //　获得列
 {
      return m_icolumn;
 }

