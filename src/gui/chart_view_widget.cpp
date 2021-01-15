/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     leiyu <leiyu@uniontech.com>
*
* Maintainer: leiyu <leiyu@uniontech.com>
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
#include "chart_view_widget.h"

#include <QPainter>
#include <DApplicationHelper>
#include <DApplication>

DWIDGET_USE_NAMESPACE
const int allDatacount = 30;
ChartViewWidget::ChartViewWidget(QWidget *parent) : QWidget(parent)
{
    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &ChartViewWidget::changeFont);
}

void ChartViewWidget::setDataColor(const QColor &color)
{
    m_dataColor = color;
}

void ChartViewWidget::addData(qreal data)
{
    m_listData << data;
    if (m_listData.size() > allDatacount + 10)
        m_listData.pop_front();
}

void ChartViewWidget::changeFont(const QFont &font)
{
    m_textfont = font;
    m_textfont.setPointSize(m_textfont.pointSize() - 2);
}

void ChartViewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    drawBackPixmap();
}

void ChartViewWidget::getPainterPathByData(const QList<qreal> &listData, QPainterPath &path)
{
    qreal offsetX = 0;
    qreal distance = m_chartRect.width() * 1.0 / allDatacount;
    int dataCount = listData.size();
    int startIndex = qMax(0, dataCount - allDatacount - 5);

    path.moveTo(offsetX, -m_chartRect.height() *listData[0]);
    for (int i = dataCount - 1;  i > startIndex; i--) {
        QPointF sp(offsetX, -m_chartRect.height() *listData[i]);
        QPointF ep(offsetX - distance, -m_chartRect.height() * listData[i - 1]);
        offsetX -= distance;

        QPointF c1 = QPointF((sp.x() + ep.x()) / 2.0, sp.y());
        QPointF c2 = QPointF((sp.x() + ep.x()) / 2.0, ep.y());
        path.cubicTo(c1, c2, ep);
    }

}

void ChartViewWidget::drawData(QPainter *painter)
{
    if (m_listData.size() <= 0)
        return;

    painter->setClipRect(m_chartRect);

    QPainterPath path;
    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(m_dataColor, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->translate(m_chartRect.bottomRight());

    getPainterPathByData(m_listData, path);
    painter->drawPath(path);
}

void ChartViewWidget::drawBackPixmap()
{
    m_backPixmap = QPixmap(this->size());
    m_backPixmap.fill(Qt::transparent);

    QPainter painter(&m_backPixmap);

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    QColor frameColor = palette.color(DPalette::TextTips);
    frameColor.setAlphaF(0.3);

    QPen framePen;
    int penSize = 1;
    framePen.setColor(frameColor);
    framePen.setWidth(penSize);
    painter.setPen(framePen);

    painter.setFont(m_textfont);
    painter.drawText(0, 0, this->width(), painter.fontMetrics().height(), Qt::AlignRight | Qt::AlignVCenter, "100%");

    int gridX = penSize;
    int gridY = penSize + painter.fontMetrics().height();
    int gridWidth = this->width() - 2 * penSize;
    int gridHeight = this->height() - 2 * gridY;

    QRect bottomTextRect(0, this->height() - painter.fontMetrics().height(), this->width(), painter.fontMetrics().height());
    painter.drawText(bottomTextRect, Qt::AlignRight | Qt::AlignVCenter, "0");
    painter.drawText(bottomTextRect, Qt::AlignLeft | Qt::AlignVCenter, tr("60 seconds"));

    painter.setRenderHint(QPainter::Antialiasing, false);
    QPainterPath framePath;
    m_chartRect = QRect(gridX, gridY, gridWidth, gridHeight);
    framePath.addRect(m_chartRect);
    painter.drawPath(framePath);

    // Draw grid.
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 2;
    dashes << space << space;
    gridPen.setDashPattern(dashes);
    gridPen.setColor(frameColor);
    painter.setPen(gridPen);

    int gridLineX = gridX;
    while (gridLineX + gridSize + penSize < gridX + gridWidth) {
        gridLineX += gridSize + penSize;
        painter.drawLine(gridLineX, gridY + 1, gridLineX, gridY + gridHeight - 1);
    }
    int gridLineY = gridY;
    while (gridLineY + gridSize + penSize < gridY + gridHeight) {
        gridLineY += gridSize + penSize;
        painter.drawLine(gridX + 1, gridLineY, gridX + gridWidth - 1, gridLineY);
    }
}

void ChartViewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_backPixmap);
    drawData(&painter);
}
