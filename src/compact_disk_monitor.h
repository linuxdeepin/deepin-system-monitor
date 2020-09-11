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

#include <DApplicationHelper>
#include <QWidget>
#include <QPainterPath>

DWIDGET_USE_NAMESPACE

class CompactDiskMonitor : public QWidget
{
    Q_OBJECT

public:
    CompactDiskMonitor(QWidget *parent = nullptr);
    ~CompactDiskMonitor();

public slots:
    void updateStatus(qreal readBps, qreal writeBps);

protected:
    void paintEvent(QPaintEvent *event);

private:
    void changeFont(const QFont &font);

private:
    QList<qreal> *readSpeeds;
    QList<qreal> *writeSpeeds;
    qreal m_readBps {};
    qreal m_writeBps {};

    int m_bulletSize = 6;

    QColor m_diskReadColor {"#8F88FF"};
    QColor m_diskWriteColor {"#6AD787"};

    QPainterPath readPath;
    QPainterPath writePath;

    int gridSize = 10;
    int pointsNumber = 60;
    int readRenderMaxHeight = 30;
    int writeRenderMaxHeight = 30;
    int writeWaveformsRenderOffsetY = -5;

    QFont m_tagFont;
    QFont m_statFont;
};

#endif
