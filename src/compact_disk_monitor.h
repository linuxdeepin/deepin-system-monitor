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

#ifndef COMPACTDISKMONITOR_H
#define COMPACTDISKMONITOR_H

#include <QWidget>

class CompactDiskMonitor : public QWidget
{
    Q_OBJECT

public:
    CompactDiskMonitor(QWidget *parent = nullptr);
    ~CompactDiskMonitor();

public slots:
    void updateStatus(unsigned long totalReadKbs, unsigned long totalWriteKbs);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QList<unsigned long> *readSpeeds;
    QList<unsigned long> *writeSpeeds;
    QPainterPath readPath;
    QPainterPath writePath;
    unsigned long totalReadKbs = 0;
    unsigned long totalWriteKbs = 0;
    int readRenderMaxHeight = 50;
    int readRenderPaddingX = 13;
    int readRenderPaddingY = 20;
    int readRenderSize = 9;
    int readWaveformsRenderOffsetX = 4;
    int readWaveformsRenderOffsetY = 112;
    int gridPaddingRight = 21;
    int gridPaddingTop = 10;
    int gridRenderOffsetY = 55;
    int gridSize = 20;
    int pointerRadius = 3;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 9;
    int pointsNumber = 51;
    int writeRenderMaxHeight = 10;
    int writeRenderPaddingX = 13;
    int writeRenderPaddingY = 40;
    int writeRenderSize = 9;
    int writeWaveformsRenderOffsetY = -4;
    int waveformRenderPadding = 20;
};

#endif
