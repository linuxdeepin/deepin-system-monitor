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
#include <DStyle>
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QtMath>

#include "compact_disk_monitor.h"
#include "constant.h"
#include "process/system_monitor.h"
#include "smooth_curve_generator.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE

using namespace Utils;

CompactDiskMonitor::CompactDiskMonitor(QWidget *parent)
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

    readSpeeds = new QList<qreal>();
    for (int i = 0; i < pointsNumber; i++) {
        readSpeeds->append(0);
    }

    writeSpeeds = new QList<qreal>();
    for (int i = 0; i < pointsNumber; i++) {
        writeSpeeds->append(0);
    }

    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        connect(sysmon, &SystemMonitor::diskStatInfoUpdated, this,
                &CompactDiskMonitor::updateStatus);
    }

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CompactDiskMonitor::changeFont);
}

CompactDiskMonitor::~CompactDiskMonitor()
{
    delete readSpeeds;
    delete writeSpeeds;
}

void CompactDiskMonitor::updateStatus(qreal tReadKbs, qreal tWriteKbs)
{
    totalReadKbs = tReadKbs;
    totalWriteKbs = tWriteKbs;

    // Init read path.
    readSpeeds->append(totalReadKbs);

    if (readSpeeds->size() > pointsNumber) {
        readSpeeds->pop_front();
    }

    double readMaxHeight = 0;
    for (int i = 0; i < readSpeeds->size(); i++) {
        if (readSpeeds->at(i) > readMaxHeight) {
            readMaxHeight = readSpeeds->at(i);
        }
    }

    // Init write path.
    writeSpeeds->append(totalWriteKbs);

    if (writeSpeeds->size() > pointsNumber) {
        writeSpeeds->pop_front();
    }

    double writeMaxHeight = 0;
    for (int i = 0; i < writeSpeeds->size(); i++) {
        if (writeSpeeds->at(i) > writeMaxHeight) {
            writeMaxHeight = writeSpeeds->at(i);
        }
    }

    qreal maxH = std::max(readMaxHeight, writeMaxHeight);
    int modReadMaxRenderH = readRenderMaxHeight - 2;
    int modWriteMaxRenderH = writeRenderMaxHeight - 2;

    QList<QPointF> readPoints;
    for (int i = 0; i < readSpeeds->size(); i++) {
        if (readMaxHeight < modReadMaxRenderH) {
            readPoints.append(QPointF(i * 5, readSpeeds->at(i)));
        } else {
            qreal scale = readSpeeds->at(i) * ulong(modReadMaxRenderH) / maxH;
            if (scale > 0 && scale < 0.5) {
                scale = 0.5;
            }
            readPoints.append(QPointF(i * 5, scale));
        }
    }

    readPath = SmoothCurveGenerator::generateSmoothCurve(readPoints);

    QList<QPointF> writePoints;
    for (int i = 0; i < writeSpeeds->size(); i++) {
        if (writeMaxHeight < modWriteMaxRenderH) {
            writePoints.append(QPointF(i * 5, writeSpeeds->at(i)));
        } else {
            qreal scale = writeSpeeds->at(i) * ulong(modWriteMaxRenderH) / maxH;
            if (scale > 0 && scale < 0.5) {
                scale = 0.5;
            }
            writePoints.append(QPointF(i * 5, scale));
        }
    }

    writePath = SmoothCurveGenerator::generateSmoothCurve(writePoints);

    update();
}

void CompactDiskMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
#ifndef THEME_FALLBACK_COLOR
    QColor tagColor = palette.color(DPalette::TextTitle);
#else
    QColor tagColor = palette.color(DPalette::Text);
#endif

    QColor statColor = palette.color(DPalette::TextTips);
    QColor frameColor = palette.color(DPalette::FrameBorder);

    // Draw disk summary.
    QFontMetrics fm(m_tagFont);
    QFontMetrics fmStat(m_statFont);

    QString rtag = DApplication::translate("Process.Graph.View", "Disk read");
    QString wtag = DApplication::translate("Process.Graph.View", "Disk write");
    QString rstat = QString("%1").arg(formatBandwidth(totalReadKbs));
    QString wstat = QString("%1").arg(formatBandwidth(totalWriteKbs));

    QRect rcol1(m_bulletSize * 2 + 2, 0, fm.size(Qt::TextSingleLine, rtag).width(), fm.height());
    QRect rcol2(rcol1.x() + rcol1.width() + margin, rcol1.y(),
                fmStat.size(Qt::TextSingleLine, rstat).width(), fmStat.height());
    QRect wcol1(rcol1.x(), rcol1.y() + rcol1.height(), fm.size(Qt::TextSingleLine, wtag).width(),
                fm.height());
    QRect wcol2(rcol2.x(), wcol1.y(), fmStat.size(Qt::TextSingleLine, wstat).width(), fm.height());
    QRect bulletRect1(0, rcol1.y() + qCeil((rcol1.height() - m_bulletSize) / 2.), m_bulletSize,
                      m_bulletSize);
    QRect bulletRect2(0, wcol1.y() + qCeil((wcol1.height() - m_bulletSize) / 2.), m_bulletSize,
                      m_bulletSize);

    QPainterPath path1, path2;
    path1.addEllipse(bulletRect1);
    path2.addEllipse(bulletRect2);
    painter.fillPath(path1, m_diskReadColor);
    painter.fillPath(path2, m_diskWriteColor);

    painter.setPen(QPen(tagColor));
    painter.setFont(m_tagFont);
    painter.drawText(rcol1, Qt::AlignLeft | Qt::AlignVCenter, rtag);
    painter.drawText(wcol1, Qt::AlignLeft | Qt::AlignVCenter, wtag);

    painter.setPen(QPen(statColor));
    painter.setFont(m_statFont);
    painter.drawText(rcol2, Qt::AlignLeft | Qt::AlignVCenter, rstat);
    painter.drawText(wcol2, Qt::AlignLeft | Qt::AlignVCenter, wstat);

    // Draw background grid.
    painter.setRenderHint(QPainter::Antialiasing, false);

    QPen framePen;
    int penSize = 1;
    framePen.setColor(frameColor);
    framePen.setWidth(penSize);
    painter.setPen(framePen);

    int gridX = rect().x() + penSize;
    int gridY = rect().y() + wcol1.y() + wcol1.height() + margin;
    int gridWidth =
        rect().width() - 3 - ((rect().width() - 3 - penSize) % (gridSize + penSize)) - penSize;
    int gridHeight = readRenderMaxHeight + writeRenderMaxHeight + 4 * penSize;

    QPainterPath framePath;
    QRect gridFrame(gridX, gridY, gridWidth, gridHeight);
    framePath.addRect(gridFrame);
    painter.drawPath(framePath);

    // Draw grid.
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 2;
    dashes << 2 << space;
    gridPen.setColor(frameColor);
    gridPen.setWidth(penSize);
    gridPen.setDashPattern(dashes);
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

    QPainterPath clip;
    clip.addRect(gridFrame);
    painter.setClipPath(clip);

    qreal diskCurveWidth = 1.2;

    painter.translate(gridFrame.x() + 2, gridFrame.y() + gridFrame.height() / 2 - 2);
    painter.scale(1, -1);
    painter.setPen(QPen(m_diskReadColor, diskCurveWidth));
    painter.drawPath(readPath);

    painter.translate(0, writeWaveformsRenderOffsetY);
    painter.scale(1, -1);
    painter.setPen(QPen(m_diskWriteColor, diskCurveWidth));
    painter.drawPath(writePath);

    setFixedHeight(gridFrame.y() + gridFrame.height() + penSize);
}

void CompactDiskMonitor::changeFont(const QFont &font)
{
    m_tagFont = font;
    m_tagFont.setPointSize(m_tagFont.pointSize() - 1);
    m_tagFont.setWeight(QFont::Medium);
    m_statFont = font;
    m_statFont.setPointSize(m_statFont.pointSize() - 1);
}
