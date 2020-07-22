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

#include "cpu_monitor.h"

#include "process/system_monitor.h"
#include "process/stats_collector.h"
#include "smooth_curve_generator.h"
#include "gui/ui_common.h"
#include "constant.h"
#include "settings.h"
#include "utils.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>

#include <QDebug>
#include <QIcon>
#include <QPainter>
#include <QtMath>
#include <QPropertyAnimation>

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

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &CpuMonitor::changeTheme);
    m_themeType = dAppHelper->themeType();
    changeTheme(m_themeType);

    auto *smo = SystemMonitor::instance();
    Q_ASSERT(smo != nullptr);
    connect(smo->jobInstance(), &StatsCollector::cpuStatInfoUpdated,
            this, &CpuMonitor::updateStatus);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CpuMonitor::changeFont);

    m_animation = new QPropertyAnimation(this, "progress", this);
    m_animation->setDuration(250);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_animation, &QVariantAnimation::valueChanged, [ = ]() {
        update();
    });
}

CpuMonitor::~CpuMonitor()
{
    delete cpuPercents;
}

void CpuMonitor::changeTheme(DApplicationHelper::ColorType themeType)
{
    m_themeType = themeType;

    switch (m_themeType) {
    case DApplicationHelper::LightType:
        ringBackgroundColor = "#000000";
        m_icon = QIcon(iconPathFromQrc("light/icon_cpu_light.svg"));
        break;
    case DApplicationHelper::DarkType:
        ringBackgroundColor = "#FFFFFF";
        m_icon = QIcon(iconPathFromQrc("dark/icon_cpu_light.svg"));
        break;
    default:
        break;
    }

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    textColor = palette.color(DPalette::Text);
#ifndef THEME_FALLBACK_COLOR
    numberColor = palette.color(DPalette::TextTitle);
#else
    numberColor = palette.color(DPalette::Text);
#endif

    update();
}

void CpuMonitor::updateStatus(qreal cpuPercent, const QList<qreal>)
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

    m_animation->start();
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
    int titleWidth =
        fm.size(Qt::TextSingleLine, DApplication::translate("Process.Graph.View", "CPU")).width();

    int iconSize = 24;

    painter.setFont(m_cpuDisplayFont);
    painter.setPen(QPen(textColor));
    QRect cpuDisplayRect(((rect().x() + rect().width() - titleWidth) / 2) - paddingRight,
                         rect().y() + titleRenderOffsetY, titleWidth, fm.height());
    painter.drawText(cpuDisplayRect, Qt::AlignCenter,
                     DApplication::translate("Process.Graph.View", "CPU"));

    QRect iconRect(cpuDisplayRect.x() - margin - iconSize + 6,
                   cpuDisplayRect.y() + qCeil((cpuDisplayRect.height() - iconSize) / 2.) + 2,
                   iconSize, iconSize);
    m_icon.paint(&painter, iconRect);

    auto cdiff = cpuPercents->last() - cpuPercents->at(cpuPercents->size() - 2);
    double percent = (cpuPercents->at(cpuPercents->size() - 2) + m_progress * cdiff);

    painter.setFont(m_cpuUsageFont);
    painter.setPen(QPen(numberColor));
    painter.drawText(
        QRect(rect().x() - paddingRight, rect().y() + percentRenderOffsetY, rect().width(), 30),
        Qt::AlignCenter, QString("%1%").arg(QString::number(percent, 'f', 1)));

    int centerX = rect().x() + rect().width() / 2 - paddingRight;
    int centerY = rect().y() + ringRenderOffsetY;
    drawLoadingRing(painter, centerX, centerY, ringRadius, ringWidth, 270, 135, ringForegroundColor,
                    ringForegroundOpacity, ringBackgroundColor, ringBackgroundOpacity,
                    percent / 100);

    // clip internal area of the ring
    QRect clipRect(centerX - ringRadius + ringWidth, centerY - ringRadius + ringWidth,
                   (ringRadius - ringWidth) * 2, (ringRadius - ringWidth) * 2);
    QPainterPath clip;
    clipRect = clipRect.marginsRemoved({ringWidth, ringWidth, ringWidth, ringWidth});
    clip.addRoundedRect(clipRect, clipRect.width(), clipRect.height());
    painter.setClipPath(clip);

    painter.translate(waveformsRenderOffsetX, waveformsRenderOffsetY);
    painter.scale(1, -1);
    painter.setPen(QPen(QColor("#0081FF"), 1.5));
    painter.drawPath(cpuPath);

    setFixedHeight(cpuDisplayRect.y() + cpuDisplayRect.height() + 1);
}
