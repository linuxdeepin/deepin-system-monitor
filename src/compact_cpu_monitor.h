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
    void updateStatus(qreal totalCpuPercent, const QList<qreal> cPercents);

protected:
    void paintEvent(QPaintEvent *event);

private:
    void changeFont(const QFont &font);

private:
    QList<QList<qreal>> cpuPercents;
    QList<QPainterPath> cpuPaths;
    QList<QColor> cpuColors;
    int cpuRenderMaxHeight = 80;
    int cpuWaveformsRenderOffsetY = 112;
    int gridSize = 10;
    int pointsNumber = 25;
    int waveformRenderPadding = 20;
    int numCPU;
    double totalCpuPercent = 0;

    int pointerRadius = 6;

    QFont m_cpuFont;
    QFont m_statFont;
};

#endif
