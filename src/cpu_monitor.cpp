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

#include "cpu_monitor.h"
#include <QPainter>
#include <QDebug>

#include "utils.h"
#include "smooth_curve_generator.h"

using namespace Utils;

CpuMonitor::CpuMonitor(QWidget *parent) : QWidget(parent)
{
    setFixedSize(280, 250);

    iconImage = QImage(Utils::getQrcPath("icon_cpu.png"));

    cpuPercents = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        cpuPercents->append(0);
    }

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(render()));
    timer->start(30);
}

CpuMonitor::~CpuMonitor()
{
    delete cpuPercents;
    delete timer;
}

void CpuMonitor::render()
{
    if (animationIndex < animationFrames) {
        animationIndex++;

        repaint();
    } else {
        timer->stop();
    }
}

void CpuMonitor::updateStatus(double cpuPercent)
{
    cpuPercents->append(cpuPercent);

    if (cpuPercents->size() > pointsNumber) {
        cpuPercents->pop_front();
    }

    QList<QPointF> points;

    double cpuMaxHeight = 0;
    for (int i = 0; i < cpuPercents->size(); i++) {
        if (cpuPercents->at(i) > cpuMaxHeight) {
            cpuMaxHeight = cpuPercents->at(i);
        }
    }

    for (int i = 0; i < cpuPercents->size(); i++) {
        if (cpuMaxHeight < cpuRenderMaxHeight) {
            points.append(QPointF(i * 5, cpuPercents->at(i)));
        } else {
            points.append(QPointF(i * 5, cpuPercents->at(i) * cpuRenderMaxHeight / cpuMaxHeight));
        }
    }

    cpuPath = SmoothCurveGenerator::generateSmoothCurve(points);

    if (cpuPercents->last() != cpuPercents->at(cpuPercents->size() - 2)) {
        animationIndex = 0;
        timer->start(30);
    }
}

void CpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);


    QFont font = painter.font() ;
    font.setPointSize(20);
    font.setWeight(QFont::Light);
    
    QFontMetrics fm(font);
    int titleWidth = fm.width("处理器");
    
    int iconTitleWidth = iconImage.width() + iconPadding + titleWidth;
    
    painter.drawImage(QPoint((rect().x() + (rect().width() - iconTitleWidth) / 2) - titleAreaPaddingX - paddingRight, iconRenderOffsetY), iconImage);
    
    painter.setFont(font);
    painter.setPen(QPen(QColor("#ffffff")));
    painter.drawText(QRect((rect().x() + (rect().width() - iconTitleWidth) / 2) + iconImage.width() + iconPadding - titleAreaPaddingX - paddingRight,
                           rect().y() + titleRenderOffsetY,
                           titleWidth,
                           30
                         ), Qt::AlignCenter, "处理器");

    double percent = (cpuPercents->at(cpuPercents->size() - 2) + easeInOut(animationIndex / animationFrames) * (cpuPercents->last() - cpuPercents->at(cpuPercents->size() - 2)));

    setFontSize(painter, 15);
    painter.setPen(QPen(QColor("#aaaaaa")));
    painter.drawText(QRect(rect().x() - paddingRight,
                           rect().y() + percentRenderOffsetY,
                           rect().width(),
                           30
                         ), Qt::AlignCenter, QString("%1%").arg(QString::number(percent, 'f', 1)));

    drawLoadingRing(
        painter,
        rect().x() + rect().width() / 2 - paddingRight,
        rect().y() + ringRenderOffsetY,
        ringRadius,
        ringWidth,
        300,
        150,
        "#8442FB",
        0.1,
        percent / 100
        );

    painter.translate(waveformsRenderOffsetX - paddingRight, waveformsRenderOffsetY);
    painter.scale(1, -1);

    painter.setPen(QPen(QColor("#8442FB"), 2));
    painter.drawPath(cpuPath);
}
