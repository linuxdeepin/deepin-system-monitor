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

#include "dapplication.h"

DWIDGET_USE_NAMESPACE

class Settings;

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

private:
    void changeFont(const QFont &font);

protected:
    QPointF getEndPointerCoordinate(double percent, int r);
    void paintEvent(QPaintEvent *event);

    QPixmap iconImage;

    QColor memoryBackgroundColor;
    QColor memoryColor {"#00C5C0"};
    QColor memoryForegroundColor {"#00C5C0"};
    QColor numberColor;
    QColor summaryColor;
    QColor swapBackgroundColor;
    QColor swapColor {"#FEDF19"};
    QColor swapForegroundColor {"#FEDF19"};
    QColor textColor;

    QTimer *timer;
    QVBoxLayout *layout;
    double animationFrames = 20;
    double memoryBackgroundOpacity = 0.1;
    double memoryForegroundOpacity = 1.0;
    double swapBackgroundOpacity = 0.1;
    double swapForegroundOpacity = 1.0;
    int animationIndex = 0;
    int insideRingRadius = 41;
    int outsideRingRadius = 48;
    int pointerRadius = 3;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 9;
    int ringCenterPointerX;
    int ringCenterPointerY = 70;
    int ringWidth = 6;

    long prevUsedMemory = 0;
    long prevUsedSwap = 0;
    long totalMemory = 0;
    long totalSwap = 0;
    long usedMemory = 0;
    long usedSwap = 0;

    QFont m_titleFont;
    QFont m_contentFont;
    QFont m_subContentFont;
    QFont m_memPercentFont;

    Settings *m_settings;
    DApplicationHelper::ColorType m_themeType;
};

#endif
