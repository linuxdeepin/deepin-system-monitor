// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "compact_gpu_monitor.h"

#include "smooth_curve_generator.h"
#include "common/common.h"
#include "model/gpu_info_model.h"

#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif
#include <DPalette>
#include <DStyleHelper>

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QMouseEvent>

DWIDGET_USE_NAMESPACE
using namespace common;
using namespace DDLog;

CompactGpuMonitor::CompactGpuMonitor(QWidget *parent)
    : QWidget(parent)
{
    qCDebug(app) << "CompactGpuMonitor constructor";
    int statusBarMaxWidth = common::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth);
    setFixedHeight(160);

    pointsNumber = int(statusBarMaxWidth / 10);

    gpuPercents = new QList<qreal>();
    for (int i = 0; i < pointsNumber; i++) {
        gpuPercents->append(0);
    }

    m_gpuInfoModel = GPUInfoModel::instance();
    connect(m_gpuInfoModel, &GPUInfoModel::modelUpdated, this, &CompactGpuMonitor::updateStatus);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CompactGpuMonitor::changeFont);
}

CompactGpuMonitor::~CompactGpuMonitor()
{
    delete gpuPercents;
}

void CompactGpuMonitor::updateStatus()
{
    qCDebug(app) << "CompactGpuMonitor updateStatus";
    m_gpuPercent = m_gpuInfoModel->gpuAllUtilization();

    gpuPercents->append(m_gpuPercent);

    if (gpuPercents->size() > pointsNumber) {
        gpuPercents->pop_front();
    }

    QList<QPointF> points;
    for (int i = 0; i < gpuPercents->size(); i++) {
        qreal val = gpuPercents->at(i);
        if (val > renderMaxHeight)
            val = renderMaxHeight;
        points.append(QPointF(i * 5, val));
    }

    gpuPath = SmoothCurveGenerator::generateSmoothCurve(points);

    update();
}

void CompactGpuMonitor::changeFont(const QFont &font)
{
    m_tagFont = font;
    m_tagFont.setWeight(QFont::Medium);
    m_tagFont.setPointSizeF(m_tagFont.pointSizeF() - 1);
    m_statFont = font;
    m_statFont.setPointSizeF(m_statFont.pointSizeF() - 1);
}

void CompactGpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif

    auto palette = dAppHelper->applicationPalette();
    QColor frameColor = palette.color(DPalette::TextTips);
    frameColor.setAlphaF(0.3);

#ifndef THEME_FALLBACK_COLOR
    QColor tagColor = palette.color(DPalette::TextTitle);
#else
    QColor tagColor = palette.color(DPalette::Text);
#endif

    QColor statColor = palette.color(DPalette::TextTips);
    QColor sectionColor = m_gpuColor;

    int spacing = 4;

    // Draw tag line
    QFontMetrics fm(m_tagFont);
    QFontMetrics fmStat(m_statFont);

    QString gpuText = DApplication::translate("Process.Graph.View", "GPU");
    QString gpuStatText = QString::number(m_gpuPercent, 'f', 1).append('%');

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QRect tagRect(m_bulletSize * 2 + spacing - 2, 0, fm.width(gpuText), fm.height() + 4);
#else
    QRect tagRect(m_bulletSize * 2 + spacing - 2, 0, fm.horizontalAdvance(gpuText), fm.height() + 4);
#endif
    QRect sectionRect(0, tagRect.y() + qCeil((tagRect.height() - m_bulletSize) / 2.),
                      m_bulletSize, m_bulletSize);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QRect statRect(tagRect.x() + tagRect.width() + spacing, tagRect.y(), fmStat.width(gpuStatText),
                   fmStat.height() + 4);
#else
    QRect statRect(tagRect.x() + tagRect.width() + spacing, tagRect.y(), fmStat.horizontalAdvance(gpuStatText),
                   fmStat.height() + 4);
#endif

    // draw section bullet
    painter.setPen(sectionColor);
    QPainterPath path;
    path.addRoundedRect(sectionRect, m_bulletSize, m_bulletSize);
    painter.fillPath(path, sectionColor);

    // draw GPU label
    painter.setPen(tagColor);
    painter.setFont(m_tagFont);
    painter.drawText(tagRect, Qt::AlignLeft | Qt::AlignVCenter, gpuText);

    // draw GPU stat
    painter.setPen(statColor);
    painter.setFont(m_statFont);
    painter.drawText(statRect, Qt::AlignLeft | Qt::AlignVCenter, gpuStatText);

    // draw grid frame
    QPen framePen;
    int penSize = 1;
    framePen.setColor(frameColor);
    framePen.setWidth(penSize);
    painter.setPen(framePen);

    int gridSize = 10;
    int gridX = rect().x() + penSize;
    int gridY = tagRect.y() + tagRect.height() + 10;
    int gridWidth =
        rect().width() - 3 - ((rect().width() - 3 - penSize) % (gridSize + penSize)) - penSize;
    int gridHeight = renderMaxHeight + 8 * penSize;

    painter.setRenderHint(QPainter::Antialiasing, false);
    QPainterPath framePath;
    QRect gridFrame(gridX, gridY, gridWidth, gridHeight);
    framePath.addRect(gridFrame);
    painter.drawPath(framePath);

    // Draw grid lines
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 2;
    dashes << 2 << space;
    gridPen.setDashPattern(dashes);
    gridPen.setColor(frameColor);
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

    painter.setRenderHint(QPainter::Antialiasing, true);

    // Clip to grid and draw curve
    QPainterPath clip;
    clip.addRect(gridFrame);
    painter.setClipPath(clip);

    painter.translate(gridFrame.x() + penSize, gridFrame.y() + gridFrame.height() - penSize);
    painter.scale(1, -1);

    qreal strokeWidth = 1.2;
    painter.setPen(QPen(m_gpuColor, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(gpuPath);

    setFixedHeight(gridFrame.y() + gridFrame.height() + penSize);
}

void CompactGpuMonitor::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
        emit clicked("MSG_GPU");
}

void CompactGpuMonitor::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    return;
}
