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
#ifndef GRAPHICS_STAT_GRID_ITEM_H
#define GRAPHICS_STAT_GRID_ITEM_H

#include <QGraphicsItem>
#include <QGraphicsLayoutItem>

/**
 * @brief Base class for stat view (summary or detail view)
 */
class GraphicsStatGridItem : public QGraphicsItem
    , public QGraphicsLayoutItem
{
public:
    typedef enum rectType {
        None,
        Top_Left,
        Top_Right,
        Bottom_Left,
        Bottom_Right

    }RectType;
    GraphicsStatGridItem(const QRect& rect,
                         const QString& leftText,
                         const QString& rightText,
                         RectType rt = None);
    GraphicsStatGridItem();
public:
    /**
     * @brief init item
     * @param leftText show left text
     * @param rightText show right text
     */
    void init( const QString& leftText,const QString& rightText);
    /**
     * @brief set frame color
     * @param color frame color
     */
    void setLineColor(const QColor& color);
    /**
     * @brief set right text
     * @param right text
     */
    void setRightText(const QString& strText);
    /**
     * @brief set left Margin
     * @param left Margin
     */
    void setLeftMargin(int n);
    /**
     * @brief set right Margin
     * @param right Margin
     */
    void setRightMargin(int n);
     /**
     * @brief set background color
     * @param background color
     */
    void setBackground(const QColor& color);
       /**
     * @brief clear background
     */
    void clearBackground();
     /**
     * @brief set left text color
     * @param left text color
     */
    void setLeftTextColor(const QColor& color);
      /**
     * @brief set right text color
     * @param right text color
     */
    void setRightTextColor(const QColor& color);
      /**
     * @brief set left text tool tip
     * @param show tooltip text
     */
    void setLeftTextTooltip(const QString& strText);
     /**
     * @brief set right text tool tip
     * @param show tooltip text
     */
    void setRightTextTooltip(const QString& strText);
     /**
     * @brief set Radius
     * @param Radius value
     */
    void setRadius(int radius);

protected:
    // repaint
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    // get rect
    QRectF boundingRect() const override;
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const override;
    void setGeometry(const QRectF &geom) override;

private:
     /**
     * @brief set right text pos
     * @param
     */
    void setRightTextPos();
    /**
     * @brief set left text pos
     * @param
     */
    void setLeftTextPos();
     /**
     * @brief get painter path
     * @param result
     */
    void getPainterPath(QPainterPath &path);
private:
    QRect  m_rect; // 当前item的rect
    QColor m_lineColor = Qt::gray;// 边框颜色
    QColor m_backColor; // 背景色
    QGraphicsTextItem  *m_textLeftItem; // 左侧文字的item
    QGraphicsTextItem  *m_textRightItem; // 右侧文字的item
    int m_leftMargin = 0; // 左侧文字的margin
    int m_rightMargin = 0; // 右侧的margin
    RectType m_rectType; // item类型
    bool isHaveBackground = false; // 是否有背景
    int m_iRadius = 10; // 圆角大小
};

#endif // GRAPHICS_STAT_GRID_ITEM_H
