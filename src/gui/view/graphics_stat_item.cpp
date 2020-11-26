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
#include "graphics_stat_item.h"
#include <QPainter>

GraphicsStatItem::GraphicsStatItem(QRect rect,ShowType type) :m_rect(rect) {
    m_showType = type;
    // init view type
    init();
}

GraphicsStatItem::GraphicsStatItem()
{
    m_showType = Memery;
    m_rect = QRect(0,0,280,100);
    init();
}

GraphicsStatItem::GraphicsStatItem(int x,int y, int width,int height,ShowType type) : m_rect(x,y,width,height) {
    m_showType = type;
    // init view type
    init();
}
// 初始化界面属性
void GraphicsStatItem::init() {
    m_gridColor = QColor::fromRgb(220,220,200);
    //  根据界面类型 设置界面要显示的默认属性
    switch (m_showType) {
    case Memery: { //  内存界面
            m_iDrawDataCount = 1;
            m_dataColor = QColor::fromRgb(106,215,135);
        }
        break;
    case  Cpu: { //  CPU
             m_iDrawDataCount = 1;
             m_dataColor = QColor::fromRgb(0,129,255);
        }
        break;
    case NetWork: { // 网络
            m_iDrawDataCount = 2;
            m_dataColor = QColor::fromRgb(0,78,239);
            m_dataColor1 = QColor::fromRgb(225,67,0);
        }
        break;
    case Disk: { // 硬盘
            m_iDrawDataCount = 2;
            m_dataColor = QColor::fromRgb(106,215,135);
            m_dataColor1 = QColor::fromRgb(143,136,255);
        }
        break;
    }
    // 初始化data list
    initDataList();
}

QRectF GraphicsStatItem::boundingRect() const {
    int penWidth=1;
    return QRectF(m_rect.adjusted(-penWidth,-penWidth,penWidth,penWidth));
}
 // 设置第一条曲线的数据
 void GraphicsStatItem::setData(qreal data) {
    m_listData.append(data);
 }
// 设置第二条曲线的数据
 void GraphicsStatItem::setData1(qreal data) {
    if(m_iDrawDataCount == 1) return;
    m_listData1.append(data);
 }
//  初始化data list
 void GraphicsStatItem::initDataList() {
     // m_rect.width()/m_offsetX  是 用来计算当前rect的X轴可以绘多少个点
    for(int i =0; i < m_rect.width()/m_offsetX; ++i) {
        m_listData.append(0);
    }
    if(m_iDrawDataCount == 2) {
        for(int i =0; i < m_rect.width()/m_offsetX; ++i) {
            m_listData1.append(0);
        }
    }
 }
 // 设置当前rect的x轴的点间距
 bool GraphicsStatItem::setOffsetX(int xSize)
 {
    // 控制最小值是5
    if(xSize < 5) return false;
    m_offsetX = xSize;
    return true;
 }
 //  设置第一条曲线的颜色
 void GraphicsStatItem::setDataColor(const QColor& color) {
    m_dataColor = color;
 }
 // 设置第二条曲线的颜色
 void GraphicsStatItem::setDataColor1(const QColor& color) {
    m_dataColor1 = color;
 }
  // 设置网格颜色
 void GraphicsStatItem::setGridColor(const QColor &color) {
    m_gridColor = color;
 }
 // 设置网格大小
 void GraphicsStatItem::setGridSize(int size) {
     m_gridSize = size;
 }

 // 根据data 获取painterpath
 void GraphicsStatItem::getPainterPathByData(const QList<qreal>& listData,QPainterPath &path) {
     qreal j = 0;
     for(int i = listData.size() - (m_rect.width() / m_offsetX);  i < listData.size() -1; ++i) {
         // 开始点
         QPointF sp(j,0 -(m_rect.height() *listData[i]));
         // 结束点
         QPointF ep(j+=m_offsetX,0 -(m_rect.height() *listData[i+1]));
         QPointF c1 = QPointF((sp.x() + ep.x()) / 2, sp.y());
         QPointF c2 = QPointF((sp.x() + ep.x()) / 2, ep.y());
         if(i == listData.size() - (m_rect.width() / m_offsetX)) {
             path.moveTo(sp);
         }
         path.cubicTo(c1, c2, ep);
     }
 }
   // 绘制曲线
 void GraphicsStatItem::drawData(QPainter *painter) {
    QPen framePen;
    framePen.setWidth(1);
    framePen.setColor(m_dataColor);
    painter->setPen(framePen);

    painter->translate(m_rect.x(), m_rect.height()+m_rect.y());
    QPainterPath path;
    getPainterPathByData(m_listData,path);
    painter->drawPath(path);
    if(m_iDrawDataCount == 2) {
        //  设置第二条曲线的pen
         framePen.setColor(m_dataColor1);
         painter->setPen(framePen);
         QPainterPath path1;
         getPainterPathByData(m_listData1,path1);
         painter->drawPath(path1);
    }
    painter->setRenderHint(QPainter::Antialiasing, true);
 }
 // 绘制网格
 void GraphicsStatItem::drawGrid(QPainter *painter) {
     int penSize = 1;
     int gridX = m_rect.x() + penSize;
     int gridY = m_rect.y() -1;
     int gridWidth = m_rect.width() -1 - penSize;
     int gridHeight = m_rect.height();

     QPen gridPen;
     QVector<qreal> dashes;
     // 虚线
     qreal space = 2;
     dashes << 2 << space;
     gridPen.setDashPattern(dashes);
     gridPen.setColor(m_gridColor);
     painter->setPen(gridPen);
     // 网格
     int gridLineX = m_rect.x() + 1;
     while (gridLineX + m_gridSize + penSize < gridX + gridWidth) {
         gridLineX += m_gridSize + penSize;
         painter->drawLine(gridLineX, gridY + 1, gridLineX, gridY + gridHeight - 1);
     }
     int gridLineY = gridY;
     while (gridLineY + m_gridSize + penSize < gridY + gridHeight) {
         gridLineY += m_gridSize + penSize;
         painter->drawLine(gridX + 1, gridLineY, gridX + gridWidth - 1, gridLineY);
     }
      painter->setRenderHint(QPainter::Antialiasing, true);
 }
// 重绘
void GraphicsStatItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    QPen framePen;
    framePen.setWidth(1); // 设置画笔宽度
    painter->setRenderHint(QPainter::Antialiasing, true);
    framePen.setColor(m_gridColor); // 设置网格颜色
    painter->setPen(framePen); // 设置画笔
    painter->drawRect(m_rect);
    drawGrid(painter); // 画网格
    drawData(painter); // 画曲线
}

