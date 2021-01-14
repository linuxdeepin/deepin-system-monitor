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
ChartViewWidget::ChartViewWidget(QWidget *parent) : QWidget(parent)
{
    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &ChartViewWidget::changeFont);
}

void ChartViewWidget::changeFont(const QFont &font)
{
    m_textfont = font;
    m_textfont.setPointSize(m_textfont.pointSize() - 2);
}

void ChartViewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    QColor frameColor = palette.color(DPalette::TextTips);
    frameColor.setAlphaF(0.5);

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
    QRect gridFrame(gridX, gridY, gridWidth, gridHeight);
    framePath.addRect(gridFrame);
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

    painter.setRenderHint(QPainter::Antialiasing, true);
}
