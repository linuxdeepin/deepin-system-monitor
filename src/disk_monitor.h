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

#ifndef DISKMONITOR_H
#define DISKMONITOR_H

#include <QWidget>

class DiskMonitor : public QWidget
{
    Q_OBJECT
    
public:
    DiskMonitor(QWidget *parent = 0);
    ~DiskMonitor();
    
public slots:
    void changeTheme(QString theme);
    void initTheme();
    void updateStatus(float totalReadKbs, float totalWriteKbs);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QPixmap iconImage;
    QPixmap iconDarkImage;
    QPixmap iconLightImage;
    QList<double> *readSpeeds;
    QList<double> *writeSpeeds;
    QPainterPath readPath;
    QPainterPath writePath;
    QString readColor = "#1094D8";
    QString summaryColor;
    QString textColor;
    QString writeColor = "#F7B300";
    float totalReadKbs = 0;
    float totalWriteKbs = 0;
    int readRenderMaxHeight = 50;
    int readRenderPaddingX = 13;
    int readRenderPaddingY = 50;
    int readRenderSize = 9;
    int readWaveformsRenderOffsetX = 4;
    int readWaveformsRenderOffsetY = 156;
    int gridPaddingRight = 21;
    int gridPaddingTop = 10;
    int gridRenderOffsetY = 99;
    int gridSize = 20;
    int iconRenderOffsetX = -5;
    int iconRenderOffsetY = 10;
    int pointerRadius = 3;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 9;
    int pointsNumber = 51;
    int textPadding = 12;
    int titleRenderOffsetX = 20;
    int titleRenderSize = 20;
    int writeRenderMaxHeight = 10;
    int writeRenderPaddingX = 13;
    int writeRenderPaddingY = 70;
    int writeRenderSize = 9;
    int writeWaveformsRenderOffsetY = -5;
    int waveformRenderPadding = 20;
};

#endif    
