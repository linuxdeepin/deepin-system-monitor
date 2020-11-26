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
    void setBackground(const QColor& color);
    void clearBackground();
    void setLeftTextColor(const QColor& color);
    void setRightTextColor(const QColor& color);
    void setLeftTextTooltip(const QString& strText);
    void setRightTextTooltip(const QString& strText);
    void setRadius(int radius);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QRectF boundingRect() const override;
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const override;
    void setGeometry(const QRectF &geom) override;

private:
    void setRightTextPos();
    void setLeftTextPos();
    void getPainterPath(QPainterPath &path);
private:
    QRect  m_rect;
    QColor m_lineColor = Qt::gray;
    QColor m_backColor;
    QGraphicsTextItem  *m_textLeftItem;
    QGraphicsTextItem  *m_textRightItem;
    int m_leftMargin = 0;
    int m_rightMargin = 0;
    RectType m_rectType;
    bool isHaveBackground = false;
    int m_iRadius = 10;
};

#endif // GRAPHICS_STAT_GRID_ITEM_H
