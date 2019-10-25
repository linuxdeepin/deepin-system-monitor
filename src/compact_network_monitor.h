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

#ifndef COMPACTNETWORKMONITOR_H
#define COMPACTNETWORKMONITOR_H

#include <QWidget>

class CompactNetworkMonitor : public QWidget
{
    Q_OBJECT

public:
    CompactNetworkMonitor(QWidget *parent = nullptr);
    ~CompactNetworkMonitor();

public slots:
    void updateStatus(long totalRecvBytes, long totalSentBytes, float totalRecvKbs,
                      float totalSentKbs);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QList<double> *downloadSpeeds;
    QList<double> *uploadSpeeds;
    QPainterPath downloadPath;
    QPainterPath uploadPath;
    QColor downloadColor {"#55D500"};
    QColor summaryColor;
    QColor textColor;
    QColor uploadColor = {"#C362FF"};
    float totalRecvKbs = 0;
    float totalSentKbs = 0;
    int downloadRenderMaxHeight = 50;
    int downloadRenderPaddingX = 13;
    int downloadRenderPaddingY = 15;
    int downloadRenderSize = 9;
    int downloadWaveformsRenderOffsetX = 4;
    int downloadWaveformsRenderOffsetY = 112;
    int gridPaddingRight = 21;
    int gridPaddingTop = 10;
    int gridRenderOffsetY = 52;
    int gridSize = 20;
    int pointerRadius = 3;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 9;
    int pointsNumber = 51;
    int textPadding = 12;
    int uploadRenderMaxHeight = 10;
    int uploadRenderPaddingX = 13;
    int uploadRenderPaddingY = 37;
    int uploadRenderSize = 9;
    int uploadWaveformsRenderOffsetY = -4;
    int waveformRenderPadding = 20;
    long totalRecvBytes = 0;
    long totalSentBytes = 0;
};

#endif
