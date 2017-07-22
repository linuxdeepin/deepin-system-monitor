/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
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

#include "constant.h"
#include "memory_monitor.h"
#include "dthememanager.h"
#include "utils.h"
#include <QDebug>
#include <QPainter>
#include <QtMath>

DWIDGET_USE_NAMESPACE

using namespace Utils;

MemoryMonitor::MemoryMonitor(QWidget *parent) : QWidget(parent)
{
    iconDarkImage = QImage(Utils::getQrcPath("icon_memory_dark.png"));
    iconLightImage = QImage(Utils::getQrcPath("icon_memory_light.png"));
    
    initTheme();
    
    connect(DThemeManager::instance(), &DThemeManager::themeChanged, this, &MemoryMonitor::changeTheme);
    
    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth);
    ringCenterPointerX = statusBarMaxWidth - 90;

    usedMemory = 0;
    totalMemory = 0;
    usedSwap = 0;
    totalSwap = 0;


    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(render()));
    timer->start();

    setFixedHeight(120);
}

MemoryMonitor::~MemoryMonitor()
{
}

void MemoryMonitor::initTheme()
{
    if (DThemeManager::instance()->theme() == "light") {
        textColor = "#303030";
        numberColor = "#000000";
        summaryColor = "#505050";
        
        memoryForegroundColor = "#FF2997";
        memoryForegroundOpacity = 1;
        memoryBackgroundColor = "#000000";
        memoryBackgroundOpacity = 0.05;

        swapForegroundColor = "#00B4C7";
        swapForegroundOpacity = 1;
        swapBackgroundColor = "#000000";
        swapBackgroundOpacity = 0.05;
        
        iconImage = iconLightImage;
    } else {
        textColor = "#ffffff";
        numberColor = "#D4D4D4";
        summaryColor = "#909090";
        
        memoryForegroundColor = "#FF2997";
        memoryForegroundOpacity = 1;
        memoryBackgroundColor = "#FF2997";
        memoryBackgroundOpacity = 0.1;

        swapForegroundColor = "#00B4C7";
        swapForegroundOpacity = 1;
        swapBackgroundColor = "#00B4C7";
        swapBackgroundOpacity = 0.1;
        
        iconImage = iconDarkImage;
    }
}

void MemoryMonitor::changeTheme(QString )
{
    initTheme();
}

void MemoryMonitor::render()
{
    if (animationIndex < animationFrames) {
        animationIndex++;

        repaint();
    } else {
        timer->stop();
    }
}

void MemoryMonitor::updateStatus(long uMemory, long tMemory, long uSwap, long tSwap)
{
    if ((uMemory != usedMemory) ||
        (tMemory != totalMemory) ||
        (uSwap != usedSwap) || 
        (tSwap != totalSwap)
        ) {
        prevUsedMemory = usedMemory;
        prevUsedSwap = usedSwap;

        usedMemory = uMemory;
        totalMemory = tMemory;
        usedSwap = uSwap;
        totalSwap = tSwap;

        animationIndex = 0;
        timer->start(30);
    }
}

QPointF MemoryMonitor::getEndPointerCoordinate(double percent, int r)
{
    int angle = 360 - (270 * percent);

    double sinValue = qSin((angle / 360.0) * 2 * M_PI);
    double cosValue = qCos((angle / 360.0) * 2 * M_PI);

    int pointerX = rect().x() + ringCenterPointerX + static_cast<int>(r * cosValue) + static_cast<int>(pointerRadius * sinValue);
    int pointerY = rect().y() + ringCenterPointerY - static_cast<int>(r * sinValue) + static_cast<int>(pointerRadius * cosValue);

    // I don't why this need adjust 1 pixel, it's weird.
    if (angle > 270) {
        pointerY += 1;
    }

    return QPointF(pointerX, pointerY);
}

void MemoryMonitor::paintEvent(QPaintEvent *)
{
    // Init.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    double memoryPercent = (prevUsedMemory + easeInOut(animationIndex / animationFrames) * (usedMemory - prevUsedMemory)) * 1.0 / totalMemory;
    double swapPercent;
    if (totalSwap == 0) {
        swapPercent = 0;
    } else {
        swapPercent = (prevUsedSwap + easeInOut(animationIndex / animationFrames) * (usedSwap - prevUsedSwap)) * 1.0 / totalSwap;
    }

    // Draw icon.
    painter.drawImage(QPoint(iconRenderOffsetX, iconRenderOffsetY), iconImage);

    // Draw title.
    QFont font = painter.font() ;
    font.setPointSize(titleRenderSize);
    font.setWeight(QFont::Light);
    painter.setFont(font);
    painter.setPen(QPen(QColor(textColor)));
    painter.drawText(QRect(rect().x() + titleRenderOffsetX, rect().y(), rect().width() - titleRenderOffsetX, rect().height()), Qt::AlignLeft | Qt::AlignTop, tr("Memory"));

    // Draw memory summary.
    setFontSize(painter, memoryRenderSize);
    QFontMetrics fm = painter.fontMetrics();

    QString memoryTitle = QString("%1 (%2%)").arg(tr("Memory")).arg(QString::number(memoryPercent * 100, 'f', 1));
    QString memoryContent = QString("%1/%2").arg(formatByteCount(usedMemory)).arg(formatByteCount(totalMemory));
    QString swapTitle = "";
    QString swapContent = "";
    if (totalSwap == 0) {
        swapTitle = QString("%1 (%2)").arg(tr("Swap")).arg(tr("Not enabled"));
        swapContent = "";
    } else {
        swapTitle = QString("%1 (%2%)").arg(tr("Swap")).arg(QString::number(swapPercent * 100, 'f', 1));
        swapContent = QString("%2/%3").arg(formatByteCount(usedSwap)).arg(formatByteCount(totalSwap));
    }
    
    painter.setPen(QPen(QColor(memoryColor)));
    painter.setBrush(QBrush(QColor(memoryColor)));
    painter.drawEllipse(QPointF(rect().x() + pointerRenderPaddingX, rect().y() + memoryRenderPaddingY + pointerRenderPaddingY), pointerRadius, pointerRadius);

    setFontSize(painter, memoryRenderSize);
    painter.setPen(QPen(QColor(summaryColor)));
    painter.drawText(QRect(rect().x() + memoryRenderPaddingX,
                           rect().y() + memoryRenderPaddingY,
                           fm.width(memoryTitle),
                           rect().height()),
                     Qt::AlignLeft | Qt::AlignTop,
                     memoryTitle);

    setFontSize(painter, memoryRenderSize);
    painter.setPen(QPen(QColor(summaryColor)));
    painter.drawText(QRect(rect().x() + memoryRenderPaddingX,
                           rect().y() + memoryRenderPaddingY + lineHeight,
                           fm.width(memoryContent),
                           rect().height()),
                     Qt::AlignLeft | Qt::AlignTop,
                     memoryContent);

    // Draw swap summary.
    painter.setPen(QPen(QColor(swapColor)));
    painter.setBrush(QBrush(QColor(swapColor)));
    painter.drawEllipse(QPointF(rect().x() + pointerRenderPaddingX, rect().y() + swapRenderPaddingY + pointerRenderPaddingY), pointerRadius, pointerRadius);

    setFontSize(painter, swapRenderSize);
    painter.setPen(QPen(QColor(summaryColor)));
    painter.drawText(QRect(rect().x() + swapRenderPaddingX,
                           rect().y() + swapRenderPaddingY,
                           fm.width(swapTitle),
                           rect().height()),
                     Qt::AlignLeft | Qt::AlignTop,
                     swapTitle);

    setFontSize(painter, swapRenderSize);
    painter.setPen(QPen(QColor(summaryColor)));
    painter.drawText(QRect(rect().x() + swapRenderPaddingX,
                           rect().y() + swapRenderPaddingY + lineHeight,
                           fm.width(swapContent),
                           rect().height()),
                     Qt::AlignLeft | Qt::AlignTop,
                     swapContent);

    // Draw memory ring.
    drawLoadingRing(
        painter,
        rect().x() + ringCenterPointerX,
        rect().y() + ringCenterPointerY,
        outsideRingRadius,
        ringWidth,
        270,
        270,
        memoryForegroundColor, memoryForegroundOpacity,
        memoryBackgroundColor, memoryBackgroundOpacity,
        memoryPercent
        );

    // Draw swap ring.
    drawLoadingRing(
        painter,
        rect().x() + ringCenterPointerX,
        rect().y() + ringCenterPointerY,
        insideRingRadius,
        ringWidth,
        270,
        270,
        swapForegroundColor, swapForegroundOpacity,
        swapBackgroundColor, swapBackgroundOpacity,
        swapPercent
        );


    // Draw percent text.
    setFontSize(painter, memoryPercentRenderSize);
    painter.setPen(QPen(QColor(numberColor)));
    painter.drawText(QRect(rect().x() + ringCenterPointerX - insideRingRadius, rect().y() + ringCenterPointerY - insideRingRadius, insideRingRadius * 2, insideRingRadius * 2),
                     Qt::AlignCenter,
                     QString("%1%").arg(QString::number(memoryPercent * 100, 'f', 1)));
}

