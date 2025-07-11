// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "chart_view_widget.h"
#include "common/common.h"
#include "ddlog.h"

#include <QPainter>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif
#include <DApplication>
#include <DFontSizeManager>

using namespace common::format;
using namespace DDLog;

DWIDGET_USE_NAMESPACE
const int allDatacount = 30;
ChartViewWidget::ChartViewWidget(ChartViewTypes types, QWidget *parent) : QWidget(parent), m_viewType(types)
{
    qCDebug(app) << "ChartViewWidget constructor, type:" << m_viewType;
    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &ChartViewWidget::changeFont);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &ChartViewWidget::changeTheme);
#else
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ChartViewWidget::changeTheme);
#endif
}

void ChartViewWidget::changeTheme()
{
    qCDebug(app) << "ChartViewWidget::changeTheme";
    drawBackPixmap();
    update();
}

void ChartViewWidget::setSpeedAxis(bool speed)
{
    qCDebug(app) << "ChartViewWidget::setSpeedAxis:" << speed;
    m_speedAxis = speed;
    if (m_viewType == BLOCK_CHART || m_viewType == MEM_CHART)
        setAxisTitle(formatUnit_memory_disk(qMax(m_maxData1.toLongLong(), m_maxData2.toLongLong()), B, 1, true));
    else
        setAxisTitle(formatUnit_net(qMax(m_maxData1.toLongLong(), m_maxData2.toLongLong()), B, 1, true));
}

void ChartViewWidget::setData1Color(const QColor &color)
{
    qCDebug(app) << "ChartViewWidget::setData1Color";
    m_data1Color = color;
}

void ChartViewWidget::addData1(const QVariant &data)
{
    // qCDebug(app) << "ChartViewWidget::addData1";
    m_listData1 << data;
    if (m_listData1.size() > allDatacount + 1) {
        qCDebug(app) << "Data1 list is full, pop one";
        m_listData1.pop_front();
    }

    auto maxElement = std::max_element(m_listData1.begin(), m_listData1.end(),
        [](const QVariant &a, const QVariant &b) {
            return a.toLongLong() < b.toLongLong();
        });
    const QVariant &maxdata = *maxElement;
    if (maxdata.toLongLong() > 0 && maxdata != m_maxData1) {
        qCDebug(app) << "Updating max data 1";
        m_maxData1 = QVariant(maxdata.toLongLong() * 1.1);
        m_maxData = qMax(m_maxData1.toLongLong(), m_maxData2.toLongLong());

        // 这边需要通过当前的图标界面类型去区分, 内存和磁盘统一处理
        if (m_speedAxis) {
            if (m_viewType == BLOCK_CHART || m_viewType == MEM_CHART)
                setAxisTitle(formatUnit_memory_disk(m_maxData, B, 1, true));
            else
                setAxisTitle(formatUnit_net(m_maxData, B, 1, true));
        }

    } else {
        qCDebug(app) << "Data 1 is zero, setting axis to zero";
        // when the data hold the zero num,we should set the chart max value as 0
        if (m_speedAxis) {
            if (m_viewType == BLOCK_CHART || m_viewType == MEM_CHART)
                setAxisTitle(formatUnit_memory_disk(0, B, 1, true));
            else
                setAxisTitle(formatUnit_net(0, B, 1, true));
        }
    }
}

void ChartViewWidget::setData2Color(const QColor &color)
{
    qCDebug(app) << "ChartViewWidget::setData2Color";
    m_data2Color = color;
}

void ChartViewWidget::addData2(const QVariant &data)
{
    m_listData2 << data;
    if (m_listData2.size() > allDatacount + 1) {
        qCDebug(app) << "Data2 list is full, pop one";
        m_listData2.pop_front();
    }

    auto maxElement = std::max_element(m_listData2.begin(), m_listData2.end(),
        [](const QVariant &a, const QVariant &b) {
            return a.toDouble() < b.toDouble();
        });
    const QVariant &maxdata = *maxElement;
    if (maxdata.toLongLong() > 0 && maxdata != m_maxData2) {
        qCDebug(app) << "Updating max data 2";
        m_maxData2 = QVariant(maxdata.toLongLong() * 1.1);
        m_maxData = qMax(m_maxData1.toLongLong(), m_maxData2.toLongLong());

        if (m_speedAxis) {
            if (m_viewType == BLOCK_CHART || m_viewType == MEM_CHART)
                setAxisTitle(formatUnit_memory_disk(m_maxData, B, 1, true));
            else
                setAxisTitle(formatUnit_net(m_maxData, B, 1, true));
        } else {
            qCDebug(app) << "Data 2 is zero, setting axis to zero";
            // when the data hold the zero num,we should set the chart max value as 0
            if (m_speedAxis) {
                if (m_viewType == BLOCK_CHART || m_viewType == MEM_CHART)
                    setAxisTitle(formatUnit_memory_disk(0, B, 1, true));
                else
                    setAxisTitle(formatUnit_net(0, B, 1, true));
            }
        }

    }
}

void ChartViewWidget::setAxisTitle(const QString &text)
{
    qCDebug(app) << "ChartViewWidget::setAxisTitle:" << text;
    m_axisTitle = text;
    update();
}

void ChartViewWidget::changeFont(const QFont &font)
{
    qCDebug(app) << "ChartViewWidget::changeFont";
    Q_UNUSED(font)
    m_textfont = DFontSizeManager::instance()->get(DFontSizeManager::T8);
}

void ChartViewWidget::resizeEvent(QResizeEvent *event)
{
    // qCDebug(app) << "ChartViewWidget::resizeEvent";
    QWidget::resizeEvent(event);
    drawBackPixmap();
}

void ChartViewWidget::getPainterPathByData(const QList<QVariant> &listData, QPainterPath &path, QVariant maxYvalue)
{
    // qCDebug(app) << "ChartViewWidget::getPainterPathByData";
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
    // qCDebug(app) << "ChartViewWidget::drawData1";
    if (m_listData1.size() <= 0) {
        qCDebug(app) << "No data for drawData1";
        return;
    }

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
    // qCDebug(app) << "ChartViewWidget::drawData2";
    if (m_listData2.size() <= 0) {
        qCDebug(app) << "No data for drawData2";
        return;
    }

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
    // qCDebug(app) << "ChartViewWidget::drawBackPixmap";
    if (this->width() == 0 || this->height() == 0) {
        qCDebug(app) << "Cannot draw back pixmap, invalid widget size";
        return;
    }

    m_backPixmap = QPixmap(this->size());
    m_backPixmap.fill(Qt::transparent);

    QPainter painter(&m_backPixmap);

    // init colors
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif
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
    //set to 0 lead to line with always 1px
    gridPen.setWidth(0);
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
    // qCDebug(app) << "ChartViewWidget::drawAxisText";
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif
    auto palette = dAppHelper->applicationPalette();
    QColor color = palette.color(DPalette::ToolTipText);
    color.setAlphaF(0.3);
    painter->setPen(color);
    painter->setFont(m_textfont);
    painter->drawText(0, 0, this->width(), painter->fontMetrics().height(), Qt::AlignRight | Qt::AlignVCenter, m_axisTitle);

    QRect bottomTextRect(0, this->height() - painter->fontMetrics().height(), this->width(), painter->fontMetrics().height());
    painter->drawText(bottomTextRect, Qt::AlignRight | Qt::AlignVCenter, "0");
    painter->drawText(bottomTextRect, Qt::AlignLeft | Qt::AlignVCenter, tr("60 seconds"));
}

void ChartViewWidget::paintEvent(QPaintEvent *event)
{
    // qCDebug(app) << "ChartViewWidget::paintEvent";
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
    painter.drawPixmap(0, 0, m_backPixmap);

    drawData1(&painter);
    drawData2(&painter);
    drawAxisText(&painter);
}
