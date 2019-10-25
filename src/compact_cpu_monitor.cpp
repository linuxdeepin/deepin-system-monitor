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

#include "compact_cpu_monitor.h"
#include <DApplicationHelper>
#include <DHiDPIHelper>
#include <DPalette>
#include <DStyleHelper>
#include <QApplication>
#include <QDebug>
#include <QPainter>

#include "constant.h"
#include "dthememanager.h"
#include "smooth_curve_generator.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE

using namespace Utils;

CompactCpuMonitor::CompactCpuMonitor(QWidget *parent)
    : QWidget(parent)
{
    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth);
    setFixedHeight(160);

    pointsNumber = int(statusBarMaxWidth / 5.4);

    numCPU = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));

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
}

CompactCpuMonitor::~CompactCpuMonitor() {}

void CompactCpuMonitor::updateStatus(double cpuPercent, std::vector<double> cPercents)
{
    totalCpuPercent = cpuPercent;

    for (unsigned int i = 0; i < cPercents.size(); i++) {
        QList<double> cpuPercent = cpuPercents[i];

        cpuPercent.append(cPercents[i]);

        if (cpuPercent.size() > pointsNumber) {
            cpuPercent.pop_front();
        }

        cpuPercents[i] = cpuPercent;

        QList<QPointF> readPoints;

        double readMaxHeight = 0;
        for (int i = 0; i < cpuPercent.size(); i++) {
            if (cpuPercent.at(i) > readMaxHeight) {
                readMaxHeight = cpuPercent.at(i);
            }
        }

        for (int i = 0; i < cpuPercent.size(); i++) {
            if (readMaxHeight < cpuRenderMaxHeight) {
                readPoints.append(QPointF(i * 5, cpuPercent.at(i)));
            } else {
                readPoints.append(
                    QPointF(i * 5, cpuPercent.at(i) * cpuRenderMaxHeight / readMaxHeight));
            }
        }

        QPainterPath cpuPath = SmoothCurveGenerator::generateSmoothCurve(readPoints);
        if ((unsigned int)cpuPaths.size() <= i) {
            cpuPaths.append(cpuPath);
        } else {
            cpuPaths[i] = cpuPath;
        }
    }

    repaint();
}

void CompactCpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    // TODO: change color
    QColor textColor = palette.color(DPalette::Text);
    QColor cpuColor = palette.color(DPalette::Text);
    QColor summaryColor = palette.color(DPalette::Text);

    // Draw cpu summary.
    setFontSize(painter, cpuTextRenderSize);
    QFontMetrics fm = painter.fontMetrics();

    QString readTitle =
        QString("%1 %2%").arg(tr("CPU")).arg(QString::number(totalCpuPercent, 'f', 1));

    painter.setOpacity(1);
    painter.setPen(QPen(textColor));
    painter.setBrush(QBrush(QColor(cpuColor)));
    painter.drawEllipse(QPointF(rect().x() + pointerRenderPaddingX,
                                rect().y() + cpuRenderPaddingY + pointerRenderPaddingY),
                        pointerRadius, pointerRadius);

    setFontSize(painter, cpuTextRenderSize);
    painter.setPen(QPen(QColor(summaryColor)));
    painter.drawText(QRect(rect().x() + cpuRenderPaddingX, rect().y() + cpuRenderPaddingY,
                           fm.width(readTitle), rect().height()),
                     Qt::AlignLeft | Qt::AlignTop, readTitle);

    // Draw background grid.
    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen framePen;
    painter.setOpacity(0.1);
    framePen.setColor(QColor(textColor));
    framePen.setWidth(1);
    painter.setPen(framePen);

    int penSize = 1;
    int gridX = rect().x() + penSize;
    int gridY = rect().y() + gridRenderOffsetY + gridPaddingTop;
    int gridWidth = rect().width() - gridPaddingRight - penSize * 2;
    int gridHeight = cpuRenderMaxHeight + waveformRenderPadding;

    QPainterPath framePath;
    painter.setBrush(QBrush());  // clear brush to got transparent background
    framePath.addRect(QRect(gridX, gridY, gridWidth, gridHeight));
    painter.drawPath(framePath);

    // Draw grid.
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 3;
    dashes << 5 << space;
    painter.setOpacity(0.05);
    gridPen.setColor(QColor(textColor));
    gridPen.setWidth(1);
    gridPen.setDashPattern(dashes);
    painter.setPen(gridPen);

    int gridLineX = gridX;
    while (gridLineX < gridX + gridWidth - gridSize) {
        gridLineX += gridSize;
        painter.drawLine(gridLineX, gridY + 1, gridLineX, gridY + gridHeight - 1);
    }
    int gridLineY = gridY;
    while (gridLineY < gridY + gridHeight - gridSize) {
        gridLineY += gridSize;
        painter.drawLine(gridX + 1, gridLineY, gridX + gridWidth - 1, gridLineY);
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setOpacity(1);
    painter.translate((rect().width() - pointsNumber * 5) / 2 - 7,
                      cpuWaveformsRenderOffsetY + gridPaddingTop);
    painter.scale(1, -1);

    qreal devicePixelRatio = qApp->devicePixelRatio();
    qreal diskCurveWidth = 1.2;
    if (devicePixelRatio > 1) {
        diskCurveWidth = 2;
    }

    for (int i = cpuPaths.size() - 1; i >= 0; i--) {
        int colorIndex;
        if (cpuPaths.size() > cpuColors.size()) {
            colorIndex = i % cpuColors.size();
        } else {
            colorIndex = i;
        }

        painter.setPen(QPen(QColor(cpuColors[colorIndex]), diskCurveWidth));
        painter.setBrush(QBrush());
        painter.drawPath(cpuPaths[i]);
    }
}
