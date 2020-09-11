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

#ifndef CpuMONITOR_H
#define CpuMONITOR_H

#include <DApplicationHelper>
#include <QIcon>
#include <QList>
#include <QPointF>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QPropertyAnimation>
#include <QPainterPath>

DWIDGET_USE_NAMESPACE

class Settings;

class CpuMonitor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    CpuMonitor(QWidget *parent = nullptr);
    ~CpuMonitor();

public slots:
    void changeTheme(DApplicationHelper::ColorType themeType);
    void updateStatus(qreal cpuPercent, const QList<qreal> cPercents);

private:
    void changeFont(const QFont &font);

protected:
    void paintEvent(QPaintEvent *event);

    qreal progress() const
    {
        return m_progress;
    }
    void setProgress(qreal p)
    {
        m_progress = p;
    }

private:
    QIcon m_icon;

    QList<double> *cpuPercents;
    QPainterPath cpuPath;
    QColor numberColor;
    QColor ringBackgroundColor;
    QColor ringForegroundColor {"#0081FF"};
    QColor textColor;
    double ringBackgroundOpacity = 0.1;
    double ringForegroundOpacity = 1;
    int animationIndex = 0;
    int cpuRenderMaxHeight = 45;
    int paddingRight = 10;
    int percentRenderOffsetY = 131;
    int pointsNumber = 28;
    int ringRadius = 90;
    int ringRenderOffsetY = 86;
    int ringWidth = 8;
    int titleAreaPaddingX = 5;
    int titleRenderOffsetY = 161;
    int waveformsRenderOffsetX;
    int waveformsRenderOffsetY = 96;

    DApplicationHelper::ColorType m_themeType;

    Settings *m_settings;

    QFont m_cpuUsageFont;
    QFont m_cpuDisplayFont;

    qreal m_progress {};
    QPropertyAnimation *m_animation {};
};

#endif
