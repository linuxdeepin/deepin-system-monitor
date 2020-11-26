/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  tenghuan <tenghuan@uniontech.com>
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
#include <QPen>
#include <QPainter>
#include "graphics_stat_grid_item.h"
#include <QDebug>


GraphicsStatGridItem::GraphicsStatGridItem(const QRect& rect,
                                           const QString& leftText,
                                           const QString& rightText,RectType rt)
{
    setGraphicsItem(this);
    m_rectType = rt;
    m_backColor = QColor::fromRgb(255,255,255);
    m_rect = rect;
    init(leftText,rightText);
}
// 构造函数
GraphicsStatGridItem::GraphicsStatGridItem()
{
     setGraphicsItem(this);
     m_rectType = None;
     m_backColor = QColor::fromRgb(255,255,255);
     m_rect = QRect(0,0,200,30);
     init("","");
}
// 初始化文字
void GraphicsStatGridItem::init( const QString& leftText, const QString& rightText)
{
    m_textLeftItem  = new QGraphicsTextItem(leftText,this);
    m_textRightItem = new QGraphicsTextItem(rightText,this);
    setRightTextPos();
}
// 设置左侧文字的颜色
void GraphicsStatGridItem::setLeftTextColor(const QColor& color)
{
    m_textLeftItem->setDefaultTextColor(color);
}
// 设置右侧文字的颜色
void GraphicsStatGridItem::setRightTextColor(const QColor& color)
{
    m_textRightItem->setDefaultTextColor(color);
}
// 设置左侧textitem的tooltip
void GraphicsStatGridItem::setLeftTextTooltip(const QString& strText)
{
    m_textLeftItem->setToolTip(strText);
}
// 设置右侧textitem的tooltip
void GraphicsStatGridItem::setRightTextTooltip(const QString& strText)
{
    m_textRightItem->setToolTip(strText);
}
// 设置边框颜色
void GraphicsStatGridItem::setLineColor(const QColor& color)
{
    m_lineColor = color;
}
// 设置item右侧的文字
void GraphicsStatGridItem::setRightText(const QString& strText)
{
    m_textRightItem->setPlainText(strText);
    setRightTextPos();
}
// 设置item左面文字的margin
void GraphicsStatGridItem::setLeftMargin(int n)
{
    m_leftMargin = n;
    setLeftTextPos();
}
// 设置右侧文字的位置
void GraphicsStatGridItem::setRightTextPos()
{
    QFontMetrics fontMetrics(m_textRightItem->font());
    int nFontWidth = fontMetrics.width(m_textRightItem->toPlainText());
    m_textRightItem->setPos(m_textRightItem->pos().x() + m_rect.width() - nFontWidth - 10 - m_rightMargin,m_textRightItem->pos().y());
}
// 设置左侧文字的位置
void GraphicsStatGridItem::setLeftTextPos()
{
    m_textLeftItem->setPos(m_textLeftItem->pos().x() + m_leftMargin,m_textLeftItem->pos().y());
}
// 设置item右面文字的margin
void GraphicsStatGridItem::setRightMargin(int n)
{
    m_rightMargin = n;
    setRightTextPos();
}
// 设置圆角的大小
void GraphicsStatGridItem::setRadius(int radius)
{
    m_iRadius = radius;
}
//
void GraphicsStatGridItem::setGeometry(const QRectF &geom)
 {
     prepareGeometryChange();
     QGraphicsLayoutItem::setGeometry(geom);
     setPos(geom.topLeft());
 }

// 获得要画的PainterPath
void GraphicsStatGridItem::getPainterPath(QPainterPath &path)
{
    int cornerSize = m_iRadius * 2;       //调节圆角的大小
//    int offset = 0;
//    if(isHaveBackground)
//        offset = 1;
    switch (m_rectType) {
    case None: //  普通的item
    {
        path.moveTo(m_rect.bottomRight()); // path起点
        path.lineTo(m_rect.topRight());
        path.lineTo(m_rect.topLeft() );
        path.lineTo(m_rect.bottomLeft());
        path.lineTo(m_rect.bottomRight());
    }
        break;
    case Top_Left: // 左上
    {
        path.moveTo(m_rect.bottomRight()+QPoint(1,0));// path起点
        path.lineTo(m_rect.topRight()+QPoint(1,0));
        path.lineTo(m_rect.topLeft() + QPointF(m_iRadius, 0));
        path.arcTo(QRectF(QPointF(m_rect.topLeft()), QSize(m_iRadius * 2, m_iRadius * 2)), 90, 90);
        path.lineTo(m_rect.bottomLeft()+QPoint(0,1));
        path.lineTo(m_rect.bottomRight()+QPoint(0,1));
    }
        break;
    case Top_Right:// 右上
    {
        path.moveTo(m_rect.left(), m_rect.top());// path起点
        path.lineTo(m_rect.left(), m_rect.bottom()+1);
        path.lineTo(m_rect.right()+1, m_rect.bottom()+1);
        path.lineTo(m_rect.right()+1, m_rect.top()+m_iRadius);
        path.arcTo(m_rect.right() +1- cornerSize, m_rect.top(), cornerSize, cornerSize, 0, 90);
        path.lineTo(m_rect.left(), m_rect.top());
    }
        break;
    case Bottom_Left:// 左下
    {
        path.moveTo(m_rect.left(), m_rect.top());// path起点
        path.lineTo(m_rect.left(), m_rect.bottom() - m_iRadius);
        path.arcTo(m_rect.left(), m_rect.bottom() +1- cornerSize, cornerSize, cornerSize, 180, 90);
        path.lineTo(m_rect.right()+1, m_rect.bottom()+1);
        path.lineTo(m_rect.right()+1, m_rect.top());
        path.lineTo(m_rect.left(), m_rect.top());
    }
        break;
    case Bottom_Right:// 右下
    {
        path.moveTo(m_rect.left(), m_rect.top());// path起点
        path.lineTo(m_rect.left(), m_rect.bottom()+1);
        path.lineTo(m_rect.right() - m_iRadius, m_rect.bottom()+1);
        path.arcTo(m_rect.right()+1 - cornerSize, m_rect.bottom() +1- cornerSize, cornerSize, cornerSize, 270, 90);
        path.lineTo(m_rect.right()+1, m_rect.top() );
        path.lineTo(m_rect.left(), m_rect.top());
    }
        break;
    }
}
// 设置背景颜色
void GraphicsStatGridItem::setBackground(const QColor& color)
{
    isHaveBackground = true;
    m_backColor = color;
}

// 是否画背景
void GraphicsStatGridItem::clearBackground()
{
    isHaveBackground = false;
}
// 重绘
void GraphicsStatGridItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPen framePen;
    framePen.setWidth(1);
    painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    //framePen.setColor(m_lineColor);
   // painter->setPen(framePen);
   // painter->setClipRect(m_rect);
   // painter->fillRect(m_rect,QBrush(m_backColor));
    QPainterPath path;
    getPainterPath(path);// 获得要画的图形的PainterPath
    // 判断item类型
    if(m_rectType != None)
    {
        painter->fillPath(path,QBrush(m_backColor)); // 填充path
    }
    else
    {
        painter->fillRect(m_rect,QBrush(m_backColor));// 填充矩形区域
    }
    // painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
}
// 获得矩形区域
QRectF GraphicsStatGridItem::boundingRect() const
{
    return QRectF(m_rect.adjusted(0,0,0,0));
}

QSizeF GraphicsStatGridItem::sizeHint(Qt::SizeHint, const QSizeF & ) const // 自适应大小
{
    return boundingRect().size();
}
