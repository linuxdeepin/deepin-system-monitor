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

#ifndef COMPACTCPUMONITOR_H
#define COMPACTCPUMONITOR_H

#include <QWidget>

class CompactCpuMonitor : public QWidget
{
    Q_OBJECT

public:
    CompactCpuMonitor(QWidget *parent = nullptr);
    ~CompactCpuMonitor();

public slots:
    void updateStatus(qreal totalCpuPercent, QVector<qreal> cPercents);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QList<QList<double>> cpuPercents;
    QList<QPainterPath> cpuPaths;
    QList<QString> cpuColors;
    int cpuRenderMaxHeight = 80;
    int cpuWaveformsRenderOffsetY = 112;
    int gridPaddingRight = 21;
    int gridPaddingTop = 10;
    int gridRenderOffsetY = 16;
    int gridSize = 20;
    int pointsNumber = 51;
    int waveformRenderPadding = 20;
    int numCPU;
    double totalCpuPercent = 0;

    int cpuTextRenderSize = 9;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 9;
    int cpuRenderPaddingX = 13;
    int cpuRenderPaddingY = 2;
    int pointerRadius = 6;
};

#endif
