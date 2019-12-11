/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#include <DApplication>
#include <DApplicationHelper>
#include <DHiDPIHelper>
#include <DPalette>
#include <DStyleHelper>
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QtMath>

#include "compact_cpu_monitor.h"
#include "constant.h"
#include "process/system_monitor.h"
#include "smooth_curve_generator.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE

using namespace Utils;

CompactCpuMonitor::CompactCpuMonitor(QWidget *parent)
    : QWidget(parent)
{
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth - margin * 2);
    setFixedHeight(160);

    pointsNumber = int(statusBarMaxWidth / 5.4);

    numCPU = int(sysconf(_SC_NPROCESSORS_ONLN));

    for (int i = 0; i < numCPU; i++) {
        QList<double> cpuPercent;
        for (int j = 0; j < pointsNumber; j++) {
            cpuPercent.append(0);
        }

        cpuPercents.append(cpuPercent);
    }

    cpuColors << "#1094D8"
              << "#F7B300"
              << "#55D500"
              << "#C362FF"
              << "#FF2997"
              << "#00B4C7"
              << "#F8E71C"
              << "#FB1818"
              << "#8544FF"
              << "#00D7AB"
              << "#00D7AB"
              << "#FF00FF"
              << "#30BF03"
              << "#7E41F1"
              << "#2CA7F8"
              << "#A005CE";

    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        connect(sysmon, &SystemMonitor::cpuStatInfoUpdated, this, &CompactCpuMonitor::updateStatus);
    }

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CompactCpuMonitor::changeFont);
}

CompactCpuMonitor::~CompactCpuMonitor() {}

void CompactCpuMonitor::updateStatus(qreal cpuPercent, QVector<qreal> cPercents)
{
    totalCpuPercent = cpuPercent;

    for (int i = 0; i < cPercents.size(); i++) {
        QList<qreal> cpuPercent = cpuPercents[i];

        cpuPercent.append(cPercents[i]);

        if (cpuPercent.size() > pointsNumber) {
            cpuPercent.pop_front();
        }

        cpuPercents[i] = cpuPercent;

        //        QList<QPointF> readPoints;

        //        double readMaxHeight = 0;
        //        for (int i = 0; i < cpuPercent.size(); i++) {
        //            if (cpuPercent.at(i) > readMaxHeight) {
        //                readMaxHeight = cpuPercent.at(i);
        //            }
        //        }

        //        int modCPURenderMaxHeight = cpuRenderMaxHeight - 20;

        //        for (int i = 0; i < cpuPercent.size(); i++) {
        //            if (readMaxHeight < modCPURenderMaxHeight) {
        //                readPoints.append(QPointF(i * 5, cpuPercent.at(i)));
        //            } else {
        //                readPoints.append(
        //                    QPointF(i * 5, cpuPercent.at(i) * modCPURenderMaxHeight /
        //                    readMaxHeight));
        //            }
        //        }

        //        QPainterPath cpuPath = SmoothCurveGenerator::generateSmoothCurve(readPoints);
        //        if (cpuPaths.size() <= i) {
        //            cpuPaths.append(cpuPath);
        //        } else {
        //            cpuPaths[i] = cpuPath;
        //        }
    }

    update();
}

void CompactCpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    auto *dAppHelper = DApplicationHelper::instance();
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    // init colors
    auto palette = dAppHelper->applicationPalette();
    QColor frameColor = palette.color(DPalette::FrameBorder);

#ifndef THEME_FALLBACK_COLOR
    QColor cpuColor = palette.color(DPalette::TextTitle);
#else
    QColor cpuColor = palette.color(DPalette::Text);
#endif

    QColor statColor = palette.color(DPalette::TextTips);
    QColor sectionColor {"#0081FF"};

    int spacing = 4;

    // Draw cpu summary.
    QFontMetrics fm(m_cpuFont);
    QFontMetrics fmStat(m_statFont);

    QString cpuText = DApplication::translate("Process.Graph.View", "CPU");
    QString cpuStatText = QString::number(totalCpuPercent, 'f', 1).append('%');

    QRect cpuRect(pointerRadius * 2 + spacing - 2, 0, fm.width(cpuText), fm.height());
    QRect sectionRect(0, cpuRect.y() + qCeil((cpuRect.height() - pointerRadius) / 2.),
                      pointerRadius, pointerRadius);
    QRect statRect(cpuRect.x() + cpuRect.width() + spacing, cpuRect.y(), fmStat.width(cpuStatText),
                   fmStat.height());

    // draw section
    painter.setPen(sectionColor);
    QPainterPath path;
    path.addRoundedRect(sectionRect, pointerRadius, pointerRadius);
    painter.fillPath(path, sectionColor);

    // draw cpu title
    painter.setPen(cpuColor);
    painter.setFont(m_cpuFont);
    painter.drawText(cpuRect, Qt::AlignLeft | Qt::AlignVCenter, cpuText);

    // draw cpu stat
    painter.setPen(statColor);
    painter.setFont(m_statFont);
    painter.drawText(statRect, Qt::AlignLeft | Qt::AlignVCenter, cpuStatText);

    // draw grid
    QPen framePen;
    int penSize = 1;
    framePen.setColor(frameColor);
    framePen.setWidth(penSize);
    painter.setPen(framePen);

    int gridX = rect().x() + penSize;
    int gridY = cpuRect.y() + cpuRect.height() + margin;
    int gridWidth =
        rect().width() - 3 - ((rect().width() - 3 - penSize) % (gridSize + penSize)) - penSize;
    int gridHeight = cpuRenderMaxHeight + 8 * penSize;

    painter.setRenderHint(QPainter::Antialiasing, false);
    QPainterPath framePath;
    QRect gridFrame(gridX, gridY, gridWidth, gridHeight);
    framePath.addRect(gridFrame);
    painter.drawPath(framePath);

    // Draw grid.
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 2;
    dashes << 2 << space;
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

    //    painter.translate(gridFrame.x() + 2 * penSize,
    //                      gridFrame.y() + gridFrame.height() - gridSize - penSize);
    painter.translate(gridFrame.x(), gridFrame.y());
    //    painter.scale(1, -1);

    //    qreal devicePixelRatio = qApp->devicePixelRatio();
    //    qreal diskCurveWidth = 1.2;
    //    if (devicePixelRatio > 1) {
    //        diskCurveWidth = 2;
    //    }

    //    for (int i = cpuPaths.size() - 1; i >= 0; i--) {
    //        int colorIndex;
    //        if (cpuPaths.size() > cpuColors.size()) {
    //            colorIndex = i % cpuColors.size();
    //        } else {
    //            colorIndex = i;
    //        }

    //        painter.setPen(QPen(cpuColors[colorIndex], diskCurveWidth));
    //        painter.drawPath(cpuPaths[i]);
    //    }

    //===========
    //
    //
    //
    //
    //
    //
    //
    //
    qreal offsetX = gridFrame.width();
    qreal sampleWidth = gridFrame.width() * 1.0 / pointsNumber;
    qreal deltaX = (gridFrame.width() - penSize * 2) * 1.0 / (pointsNumber - 3);
    // enum cpu
    for (int i = cpuPercents.size() - 1; i >= 0; i--) {
        // set stroke color
        QColor c = cpuColors[i % cpuColors.size()];
        painter.setPen(QPen(c, penSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        QPainterPath path;
        // move to the first point
        //        path.moveTo()

        for (int j = 1; j < pointsNumber; j++) {
        }
        painter.drawPath(path);
    }
}

void CompactCpuMonitor::changeFont(const QFont &font)
{
    m_cpuFont = font;
    m_cpuFont.setWeight(QFont::Medium);
    m_cpuFont.setPointSize(m_cpuFont.pointSize() - 1);
    m_statFont = font;
    m_statFont.setPointSize(m_statFont.pointSize() - 1);
}
