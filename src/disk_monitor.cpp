/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      Wang Yong <wangyong@deepin.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "disk_monitor.h"

#include "process/system_monitor.h"
#include "process/stats_collector.h"
#include "smooth_curve_generator.h"
#include "gui/ui_common.h"
#include "constant.h"
#include "utils.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DHiDPIHelper>
#include <DPalette>

#include <QApplication>
#include <QDebug>
#include <QPainter>

DWIDGET_USE_NAMESPACE
using namespace Utils;

DiskMonitor::DiskMonitor(QWidget *parent)
    : QWidget(parent),
      iconDarkImage(iconPathFromQrc("dark/icon_disk_dark.svg")),
      iconLightImage(iconPathFromQrc("light/icon_disk_light.svg"))
{
    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth);
    setFixedHeight(190);

    pointsNumber = int(statusBarMaxWidth / 5.4);

    readSpeeds = new QList<qreal>();
    for (int i = 0; i < pointsNumber; i++) {
        readSpeeds->append(0);
    }

    writeSpeeds = new QList<qreal>();
    for (int i = 0; i < pointsNumber; i++) {
        writeSpeeds->append(0);
    }

    auto *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &DiskMonitor::changeTheme);

    auto *smo = SystemMonitor::instance();
    Q_ASSERT(smo != nullptr);
    connect(smo->jobInstance(), &StatsCollector::diskStatInfoUpdated,
            this, &DiskMonitor::updateStatus);
}

DiskMonitor::~DiskMonitor()
{
    delete readSpeeds;
    delete writeSpeeds;
}

void DiskMonitor::changeTheme(DApplicationHelper::ColorType themeType)
{
    switch (themeType) {
    case DApplicationHelper::LightType:
        iconImage = iconLightImage;
        break;
    case DApplicationHelper::DarkType:
        iconImage = iconDarkImage;
        break;
    default:
        break;
    }
}

void DiskMonitor::updateStatus(qreal tReadKbs, qreal tWriteKbs)
{
    totalReadBps = tReadKbs;
    totalWriteBps = tWriteKbs;

    // Init read path.
    readSpeeds->append(totalReadBps);

    if (readSpeeds->size() > pointsNumber) {
        readSpeeds->pop_front();
    }

    QList<QPointF> readPoints;

    double readMaxHeight = 0;
    for (int i = 0; i < readSpeeds->size(); i++) {
        if (readSpeeds->at(i) > readMaxHeight) {
            readMaxHeight = readSpeeds->at(i);
        }
    }

    for (int i = 0; i < readSpeeds->size(); i++) {
        if (readMaxHeight < readRenderMaxHeight) {
            readPoints.append(QPointF(i * 5, readSpeeds->at(i)));
        } else {
            readPoints.append(
                QPointF(i * 5, readSpeeds->at(i) * readRenderMaxHeight / readMaxHeight));
        }
    }

    readPath = SmoothCurveGenerator::generateSmoothCurve(readPoints);

    // Init write path.
    writeSpeeds->append(totalWriteBps);

    if (writeSpeeds->size() > pointsNumber) {
        writeSpeeds->pop_front();
    }

    QList<QPointF> writePoints;

    double writeMaxHeight = 0;
    for (int i = 0; i < writeSpeeds->size(); i++) {
        if (writeSpeeds->at(i) > writeMaxHeight) {
            writeMaxHeight = writeSpeeds->at(i);
        }
    }

    for (int i = 0; i < writeSpeeds->size(); i++) {
        if (writeMaxHeight < writeRenderMaxHeight) {
            writePoints.append(QPointF(i * 5, writeSpeeds->at(i)));
        } else {
            writePoints.append(
                QPointF(i * 5, writeSpeeds->at(i) * writeRenderMaxHeight / writeMaxHeight));
        }
    }

    writePath = SmoothCurveGenerator::generateSmoothCurve(writePoints);

    repaint();
}

void DiskMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    // TODO: change color
    textColor = palette.color(DPalette::Text);
    summaryColor = palette.color(DPalette::Text);

    // Draw icon.
    painter.drawPixmap(QPoint(iconRenderOffsetX, iconRenderOffsetY), iconImage);

    // Draw title.
    QFont font = painter.font();
    font.setPointSize(titleRenderSize);
    font.setWeight(QFont::Light);
    painter.setFont(font);
    painter.setPen(QPen(textColor));
    painter.drawText(QRect(rect().x() + titleRenderOffsetX, rect().y(),
                           rect().width() - titleRenderOffsetX, rect().height()),
                     Qt::AlignLeft | Qt::AlignTop,
                     DApplication::translate("Process.Graph.View", "Disk"));

    // Draw background grid.
    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen framePen;
    painter.setOpacity(0.1);
    framePen.setColor(textColor);
    framePen.setWidth(1);
    painter.setPen(framePen);

    int penSize = 1;
    int gridX = rect().x() + penSize;
    int gridY = rect().y() + gridRenderOffsetY + gridPaddingTop;
    int gridWidth = rect().width() - gridPaddingRight - penSize * 2;
    int gridHeight = readRenderMaxHeight + writeRenderMaxHeight + waveformRenderPadding;

    QPainterPath framePath;
    framePath.addRect(QRect(gridX, gridY, gridWidth, gridHeight));
    painter.drawPath(framePath);

    // Draw grid.
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 3;
    dashes << 5 << space;
    painter.setOpacity(0.05);
    gridPen.setColor(textColor);
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

    // Draw disk summary.
    setFontSize(painter, readRenderSize);
    QFontMetrics fm = painter.fontMetrics();

    QString readTitle = QString("%1 %2")
                        .arg(DApplication::translate("Process.Graph.View", "Disk read"))
                        .arg(formatUnit(totalReadBps, B, 1, true));
    QString writeTitle = QString("%1 %2")
                         .arg(DApplication::translate("Process.Graph.View", "Disk write"))
                         .arg(formatUnit(totalWriteBps, B, 1, true));

    painter.setOpacity(1);
    painter.setPen(QPen(readColor));
    painter.setBrush(QBrush(readColor));
    painter.drawEllipse(QPointF(rect().x() + pointerRenderPaddingX,
                                rect().y() + readRenderPaddingY + pointerRenderPaddingY),
                        pointerRadius, pointerRadius);

    setFontSize(painter, readRenderSize);
    painter.setPen(QPen(summaryColor));
    painter.drawText(QRect(rect().x() + readRenderPaddingX, rect().y() + readRenderPaddingY,
                           fm.size(Qt::TextSingleLine, readTitle).width(), rect().height()),
                     Qt::AlignLeft | Qt::AlignTop, readTitle);

    painter.setPen(QPen(writeColor));
    painter.setBrush(QBrush(writeColor));
    painter.drawEllipse(QPointF(rect().x() + pointerRenderPaddingX,
                                rect().y() + writeRenderPaddingY + pointerRenderPaddingY),
                        pointerRadius, pointerRadius);

    setFontSize(painter, writeRenderSize);
    painter.setPen(QPen(summaryColor));
    painter.drawText(QRect(rect().x() + writeRenderPaddingX, rect().y() + writeRenderPaddingY,
                           fm.size(Qt::TextSingleLine, writeTitle).width(), rect().height()),
                     Qt::AlignLeft | Qt::AlignTop, writeTitle);

    painter.translate((rect().width() - pointsNumber * 5) / 2 - 7,
                      readWaveformsRenderOffsetY + gridPaddingTop);
    painter.scale(1, -1);

    qreal diskCurveWidth = 1.2;

    painter.setPen(QPen(readColor, diskCurveWidth));
    painter.setBrush(QBrush());
    painter.drawPath(readPath);

    painter.translate(0, writeWaveformsRenderOffsetY);
    painter.scale(1, -1);

    painter.setPen(QPen(writeColor, diskCurveWidth));
    painter.setBrush(QBrush());
    painter.drawPath(writePath);
}
