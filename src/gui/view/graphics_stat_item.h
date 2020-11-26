/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:   tenghuan <tenghuan@uniontech.com>
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
#ifndef GRAPHICS_STAT_ITEM_H
#define GRAPHICS_STAT_ITEM_H

#include <QGraphicsItem>

/**
 * @brief Base class for stat graphics item
 */
class GraphicsStatItem : public QGraphicsItem
{

public:
    // show view type
    typedef  enum ShowType {
        Memery,
        Cpu,
        NetWork,
        Disk
    }ShowType;

     GraphicsStatItem(QRect rect,ShowType type= Memery);
     GraphicsStatItem(int x,int y,int width,int heigth,ShowType type= Memery);
     GraphicsStatItem();
     /**
      * @brief paint view
      * @param
      */
     void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
     QRectF boundingRect() const override;
     /**
      * @brief set grid color
      * @param set the color
      */
     void setGridColor(const QColor& color);
     /**
      * @brief set grid size
      * @param size value
      */
     void setGridSize(int size);
     void setData(qreal data);
     void setData1(qreal data);
     void setDataColor(const QColor& color);
     void setDataColor1(const QColor& color);
     void getPainterPathByData(const QList<qreal>& list,QPainterPath &path);

     bool setOffsetX(int xSize);

private:
     /**
      * @brief  draw gird
      * @param
      */
     void drawGrid(QPainter *painter);
     /**
      * @brief init data list
      * @param
      */
     void initDataList();
     /**
      * @brief draw data curve
      * @param
      */
     void drawData(QPainter *painter);
     void init();
private:
     QRect m_rect;  //   当前的画图区域
     QColor m_gridColor = Qt::gray;  //  网格的颜色
     int m_gridSize = 10; //  网格的大小
     ShowType m_showType; //  当前界面的类型
     QList<qreal> m_listData; // 第一条曲线的绘制数据
     QList<qreal> m_listData1; // 第二条曲线的绘制数据
     QColor m_dataColor; //  第一条曲线的颜色
     QColor m_dataColor1; // 第二条曲线的颜色
     int m_iDrawDataCount = 1; //  当前界面显示的曲线条数 暂时最大支持2条
     int  m_offsetX = 5;   //  x轴点分布大小

};

#endif // GRAPHICS_STAT_ITEM_H
