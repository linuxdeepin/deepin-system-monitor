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

#ifndef MemoryMONITOR_H
#define MemoryMONITOR_H

#include <DApplicationHelper>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

DWIDGET_USE_NAMESPACE

class MemoryMonitor : public QWidget
{
    Q_OBJECT

public:
    MemoryMonitor(QWidget *parent = nullptr);
    ~MemoryMonitor();

public slots:
    void changeTheme(DApplicationHelper::ColorType themeType);
    void render();
    void updateStatus(long uMemory, long tMemory, long uSwap, long tSwap);

protected:
    QPointF getEndPointerCoordinate(double percent, int r);
    void paintEvent(QPaintEvent *event);

    QPixmap iconImage;
    QPixmap iconDarkImage;
    QPixmap iconLightImage;

    QColor memoryBackgroundColor;
    QColor memoryColor {"#FF2997"};
    QColor memoryForegroundColor;
    QColor numberColor;
    QColor summaryColor;
    QColor swapBackgroundColor;
    QColor swapColor {"#00B4C7"};
    QColor swapForegroundColor;
    QColor textColor;

    QTimer *timer;
    QVBoxLayout *layout;
    double animationFrames = 20;
    double memoryBackgroundOpacity;
    double memoryForegroundOpacity;
    double swapBackgroundOpacity;
    double swapForegroundOpacity;
    int animationIndex = 0;
    int iconRenderOffsetX = -5;
    int iconRenderOffsetY = 10;
    int insideRingRadius = 53;
    int lineHeight = 16;
    int memoryPercentRenderSize = 13;
    int memoryRenderPaddingX = 13;
    int memoryRenderPaddingY = 50;
    int memoryRenderSize = 9;
    int outsideRingRadius = 60;
    int pointerRadius = 3;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 9;
    int ringCenterPointerX;
    int ringCenterPointerY = 60;
    int ringWidth = 6;
    int swapRenderPaddingX = 13;
    int swapRenderPaddingY = 90;
    int swapRenderSize = 9;
    int textPadding = 12;
    int titleRenderOffsetX = 20;
    int titleRenderSize = 20;
    long prevUsedMemory = 0;
    long prevUsedSwap = 0;
    long totalMemory = 0;
    long totalSwap = 0;
    long usedMemory = 0;
    long usedSwap = 0;
};

#endif
