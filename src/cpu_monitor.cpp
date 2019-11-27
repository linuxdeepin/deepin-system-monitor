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

#include <DApplication>
#include <DApplicationHelper>
#include <DHiDPIHelper>
#include <DPalette>
#include <DStyle>
#include <QDebug>
#include <QIcon>
#include <QPainter>

#include "constant.h"
#include "cpu_monitor.h"
#include "process/system_monitor.h"
#include "settings.h"
#include "smooth_curve_generator.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE

using namespace Utils;

CpuMonitor::CpuMonitor(QWidget *parent)
    : QWidget(parent)
{
    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    setFixedSize(statusBarMaxWidth, 240);
    waveformsRenderOffsetX = (statusBarMaxWidth - 140) / 2;

    cpuPercents = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        cpuPercents->append(0);
    }

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(render()));
    timer->start(30);

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &CpuMonitor::changeTheme);
    m_themeType = dAppHelper->themeType();
    changeTheme(m_themeType);

    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        connect(sysmon, &SystemMonitor::cpuStatInfoUpdated, this, &CpuMonitor::updateStatus);
    }

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CpuMonitor::changeFont);
}

CpuMonitor::~CpuMonitor()
{
    delete cpuPercents;
    delete timer;
}

void CpuMonitor::changeTheme(DApplicationHelper::ColorType themeType)
{
    m_themeType = themeType;

    switch (m_themeType) {
        case DApplicationHelper::LightType:
            ringBackgroundColor = "#000000";
            iconImage = QIcon(":/image/light/icon_cpu_light.svg").pixmap(QSize(24, 24));
            break;
        case DApplicationHelper::DarkType:
            ringBackgroundColor = "#FFFFFF";
            iconImage = QIcon(":/image/dark/icon_cpu_light.svg").pixmap(QSize(24, 24));
            break;
        default:
            break;
    }

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    textColor = palette.color(DPalette::Text);
    numberColor = palette.color(DPalette::Text);

    update();
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

void CpuMonitor::updateStatus(qreal cpuPercent, QVector<qreal>)
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
            points.append(QPointF(i * 5 - 8, cpuPercents->at(i)));
        } else {
            points.append(
                QPointF(i * 5 - 8, cpuPercents->at(i) * cpuRenderMaxHeight / cpuMaxHeight));
        }
    }

    cpuPath = SmoothCurveGenerator::generateSmoothCurve(points);

    if (cpuPercents->last() != cpuPercents->at(cpuPercents->size() - 2)) {
        animationIndex = 0;
        timer->start(30);
    }
}

void CpuMonitor::changeFont(const QFont &font)
{
    m_cpuUsageFont = font;
    m_cpuUsageFont.setBold(true);
    m_cpuUsageFont.setPointSize(m_cpuUsageFont.pointSize() + 3);
    m_cpuDisplayFont = font;
    m_cpuDisplayFont.setPointSize(m_cpuDisplayFont.pointSize() + 12);
}

void CpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    auto *style = dynamic_cast<DStyle *>(DApplication::style());
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    QFontMetrics fm(m_cpuDisplayFont);
    int titleWidth = fm.width(DApplication::translate("Process.Graph.View", "CPU"));

    painter.setFont(m_cpuDisplayFont);
    painter.setPen(QPen(textColor));
    QRect cpuDisplayRect(((rect().x() + rect().width() - titleWidth) / 2) - paddingRight,
                         rect().y() + titleRenderOffsetY, titleWidth, fm.height());
    painter.drawText(cpuDisplayRect, Qt::AlignCenter,
                     DApplication::translate("Process.Graph.View", "CPU"));
    QPoint iconPoint(cpuDisplayRect.x() - margin - iconImage.width() + 6,
                     cpuDisplayRect.y() + (cpuDisplayRect.height() - iconImage.height()) / 2);
    painter.drawPixmap(iconPoint, iconImage);

    double percent = (cpuPercents->at(cpuPercents->size() - 2) +
                      easeInOut(animationIndex / animationFrames) *
                          (cpuPercents->last() - cpuPercents->at(cpuPercents->size() - 2)));

    painter.setFont(m_cpuUsageFont);
    painter.setPen(QPen(numberColor));
    painter.drawText(
        QRect(rect().x() - paddingRight, rect().y() + percentRenderOffsetY, rect().width(), 30),
        Qt::AlignCenter, QString("%1%").arg(QString::number(percent, 'f', 1)));

    drawLoadingRing(painter, rect().x() + rect().width() / 2 - paddingRight,
                    rect().y() + ringRenderOffsetY, ringRadius, ringWidth, 270, 135,
                    ringForegroundColor, ringForegroundOpacity, ringBackgroundColor,
                    ringBackgroundOpacity, percent / 100);

    painter.translate(waveformsRenderOffsetX, waveformsRenderOffsetY);
    painter.scale(1, -1);

    painter.setPen(QPen(QColor("#0081FF"), 1.5));
    painter.drawPath(cpuPath);
}
