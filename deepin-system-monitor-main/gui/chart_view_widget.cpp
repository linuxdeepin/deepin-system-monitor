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
#include "common/common.h"

#include <QPainter>
#include <DApplicationHelper>
#include <DApplication>

using namespace common::format;

DWIDGET_USE_NAMESPACE
const int allDatacount = 30;
ChartViewWidget::ChartViewWidget(QWidget *parent) : QWidget(parent)
{
    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &ChartViewWidget::changeFont);

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &ChartViewWidget::changeTheme);
}

void ChartViewWidget::changeTheme()
{
    drawBackPixmap();
    update();
}

void ChartViewWidget::setSpeedAxis(bool speed)
{
    m_speedAxis = speed;
    setAxisTitle(formatUnit(qMax(m_maxData1, m_maxData2), B, 1, true));
}

void ChartViewWidget::setData1Color(const QColor &color)
{
    m_data1Color = color;
}

void ChartViewWidget::addData1(const QVariant &data)
{
    m_listData1 << data;
    if (m_listData1.size() > allDatacount + 1) {
        m_listData1.pop_front();
    }

    const QVariant &maxdata = *std::max_element(m_listData1.begin(), m_listData1.end());
    if (maxdata.toLongLong() > 0 && maxdata != m_maxData1) {
        m_maxData1 = QVariant(maxdata.toLongLong() * 1.1);
        m_maxData = qMax(m_maxData1, m_maxData2);

        if (m_speedAxis)
            setAxisTitle(formatUnit(m_maxData, B, 1, true));
    } else {
        // when the data hold the zero num,we should set the chart max value as 0
        if (m_speedAxis)
            setAxisTitle(formatUnit(0, B, 1, true));
    }
}

void ChartViewWidget::setData2Color(const QColor &color)
{
    m_data2Color = color;
}

void ChartViewWidget::addData2(const QVariant &data)
{
    m_listData2 << data;
    if (m_listData2.size() > allDatacount + 1) {
        m_listData2.pop_front();
    }

    const QVariant &maxdata = *std::max_element(m_listData2.begin(), m_listData2.end());
    if (maxdata.toLongLong() > 0 && maxdata != m_maxData2) {
        m_maxData2 = QVariant(maxdata.toLongLong() * 1.1);
        m_maxData = qMax(m_maxData1, m_maxData2);

        if (m_speedAxis)
            setAxisTitle(formatUnit(m_maxData, B, 1, true));
    } else {
        // when the data hold the zero num,we should set the chart max value as 0
        if (m_speedAxis)
            setAxisTitle(formatUnit(0, B, 1, true));
    }
}

void ChartViewWidget::setAxisTitle(const QString &text)
{
    m_axisTitle = text;
    update();
}

void ChartViewWidget::changeFont(const QFont &font)
{
    m_textfont = font;
    m_textfont.setPointSizeF(m_textfont.pointSizeF() - 2);
}

void ChartViewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    drawBackPixmap();
}

void ChartViewWidget::getPainterPathByData(const QList<QVariant> &listData, QPainterPath &path, QVariant maxYvalue)
{
    qreal offsetX = 0;
    qreal distance = m_chartRect.width() * 1.0 / allDatacount;
    int dataCount = listData.size();
    int startIndex = qMax(0, dataCount - allDatacount - 1);

    qlonglong maxL = maxYvalue.toLongLong();
    const QVariant &lastData = listData.last();
    if (lastData.canConvert(QMetaType::Double))
        path.moveTo(offsetX, -m_chartRect.height() * lastData.toDouble() * 1.0 / maxL);
    else
        path.moveTo(offsetX, -m_chartRect.height() * lastData.toLongLong() * 1.0 / maxL);

    for (int i = dataCount - 1;  i > startIndex; i--) {
        QPointF sp;
        QPointF ep;

        if (listData[i].canConvert(QMetaType::Double)) {
            sp = QPointF(offsetX, -m_chartRect.height() * listData[i].toDouble() / maxL);
            ep = QPointF(offsetX - distance, -m_chartRect.height() * listData[i - 1].toDouble() / maxL);
        } else {
            sp = QPointF(offsetX, -m_chartRect.height() * listData[i].toLongLong() / maxL);
            ep = QPointF(offsetX - distance, -m_chartRect.height() * listData[i - 1].toLongLong() / maxL);
        }

        offsetX -= distance;

        QPointF c1 = QPointF((sp.x() + ep.x()) / 2.0, sp.y());
        QPointF c2 = QPointF((sp.x() + ep.x()) / 2.0, ep.y());
        path.cubicTo(c1, c2, ep);
    }

}

void ChartViewWidget::drawData1(QPainter *painter)
{
    if (m_listData1.size() <= 0)
        return;

    painter->save();
    painter->setClipRect(m_chartRect.adjusted(1, -1, 1, 1));

    QPainterPath path;
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(m_data1Color, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->translate(m_chartRect.bottomRight() + QPoint(1, 1));

    getPainterPathByData(m_listData1, path, m_maxData);
    painter->drawPath(path);
    painter->restore();
}

void ChartViewWidget::drawData2(QPainter *painter)
{
    if (m_listData2.size() <= 0)
        return;

    painter->save();
    painter->setClipRect(m_chartRect.adjusted(1, -1, 1, 1));

    QPainterPath path;
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(m_data2Color, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->translate(m_chartRect.bottomRight() + QPoint(1, 1));

    getPainterPathByData(m_listData2, path, m_maxData);
    painter->drawPath(path);
    painter->restore();
}

void ChartViewWidget::drawBackPixmap()
{
    if (this->width() == 0 || this->height() == 0)
        return;

    m_backPixmap = QPixmap(this->size());
    m_backPixmap.fill(Qt::transparent);

    QPainter painter(&m_backPixmap);

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    QColor frameColor = palette.color(DPalette::TextTips);
    frameColor.setAlphaF(0.3);

    int penSize = 1;
    painter.setPen(QPen(frameColor, penSize));
    painter.setBrush(palette.color(QPalette::Base));
    painter.setFont(m_textfont);

    int gridX = penSize;
    int gridY = penSize + painter.fontMetrics().height();
    int gridWidth = this->width() - 2 * penSize;
    int gridHeight = this->height() - 2 * gridY;

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

void ChartViewWidget::drawAxisText(QPainter *painter)
{
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    painter->setPen(palette.color(DPalette::TextTips));
    painter->setFont(m_textfont);
    painter->drawText(0, 0, this->width(), painter->fontMetrics().height(), Qt::AlignRight | Qt::AlignVCenter, m_axisTitle);

    QRect bottomTextRect(0, this->height() - painter->fontMetrics().height(), this->width(), painter->fontMetrics().height());
    painter->drawText(bottomTextRect, Qt::AlignRight | Qt::AlignVCenter, "0");
    painter->drawText(bottomTextRect, Qt::AlignLeft | Qt::AlignVCenter, tr("60 seconds"));
}

void ChartViewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
    painter.drawPixmap(0, 0, m_backPixmap);

    drawData1(&painter);
    drawData2(&painter);
    drawAxisText(&painter);
}
