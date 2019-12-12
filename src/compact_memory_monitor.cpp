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
#include <QPainter>
#include <QtMath>

#include "compact_memory_monitor.h"
#include "constant.h"
#include "process/system_monitor.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE

using namespace Utils;

CompactMemoryMonitor::CompactMemoryMonitor(QWidget *parent)
    : QWidget(parent)
{
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    auto *dAppHelper = DApplicationHelper::instance();
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth - margin * 2);
    ringCenterPointerX = rect().width() - outsideRingRadius - 4;

    usedMemory = 0;
    totalMemory = 0;
    usedSwap = 0;
    totalSwap = 0;

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(render()));
    timer->start();

    setFixedHeight(150);

    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this,
            &CompactMemoryMonitor::changeTheme);
    m_themeType = dAppHelper->themeType();
    changeTheme(m_themeType);

    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        connect(sysmon, &SystemMonitor::memStatInfoUpdated, this,
                &CompactMemoryMonitor::updateStatus);
    }

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CompactMemoryMonitor::changeFont);
}

CompactMemoryMonitor::~CompactMemoryMonitor() {}

void CompactMemoryMonitor::changeTheme(DApplicationHelper::ColorType themeType)
{
    m_themeType = themeType;

    switch (m_themeType) {
        case DApplicationHelper::LightType:
            memoryBackgroundColor = "#000000";
            swapBackgroundColor = "#000000";

            break;
        case DApplicationHelper::DarkType:
            memoryBackgroundColor = "#FFFFFF";
            swapBackgroundColor = "#FFFFFF";

            break;
        default:
            break;
    }

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
#ifndef THEME_FALLBACK_COLOR
    textColor = palette.color(DPalette::TextTitle);
    numberColor = palette.color(DPalette::TextTitle);
#else
    textColor = palette.color(DPalette::Text);
    numberColor = palette.color(DPalette::Text);
#endif

    summaryColor = palette.color(DPalette::TextTips);
}

void CompactMemoryMonitor::changeFont(const QFont &font)
{
    m_contentFont = font;
    m_contentFont.setWeight(QFont::Medium);
    m_contentFont.setPointSize(m_contentFont.pointSize() - 1);
    m_subContentFont = font;
    m_subContentFont.setPointSize(m_subContentFont.pointSize() - 1);
    m_memPercentFont = font;
    m_memPercentFont.setPointSize(m_memPercentFont.pointSize());
    m_memPercentFont.setBold(true);
}

void CompactMemoryMonitor::render()
{
    if (animationIndex < animationFrames) {
        animationIndex++;

        repaint();
    } else {
        timer->stop();
    }
}

void CompactMemoryMonitor::updateStatus(long uMemory, long tMemory, long uSwap, long tSwap)
{
    if ((uMemory != usedMemory) || (tMemory != totalMemory) || (uSwap != usedSwap) ||
        (tSwap != totalSwap)) {
        prevUsedMemory = usedMemory;
        prevUsedSwap = usedSwap;

        usedMemory = uMemory;
        totalMemory = tMemory;
        usedSwap = uSwap;
        totalSwap = tSwap;

        animationIndex = 0;
        timer->start(30);
    }
}

QPointF CompactMemoryMonitor::getEndPointerCoordinate(double percent, int r)
{
    int angle = 360 - (270 * percent);

    double sinValue = qSin((angle / 360.0) * 2 * M_PI);
    double cosValue = qCos((angle / 360.0) * 2 * M_PI);

    int pointerX = rect().x() + ringCenterPointerX + static_cast<int>(r * cosValue) +
                   static_cast<int>(pointerRadius * sinValue);
    int pointerY = rect().y() + ringCenterPointerY - static_cast<int>(r * sinValue) +
                   static_cast<int>(pointerRadius * cosValue);

    // I don't why this need adjust 1 pixel, it's weird.
    if (angle > 270) {
        pointerY += 1;
    }

    return QPointF(pointerX, pointerY);
}

void CompactMemoryMonitor::paintEvent(QPaintEvent *)
{
    // Init.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    double memoryPercent =
        Utils::filterInvalidNumber((prevUsedMemory + easeInOut(animationIndex / animationFrames) *
                                                         (usedMemory - prevUsedMemory)) *
                                   1.0 / totalMemory);
    double swapPercent;
    if (totalSwap == 0) {
        swapPercent = 0;
    } else {
        swapPercent = (prevUsedSwap +
                       easeInOut(animationIndex / animationFrames) * (usedSwap - prevUsedSwap)) *
                      1.0 / totalSwap;
    }

    int spacing = 10;
    int sectionSize = 6;

    // Draw memory summary.
    QString memoryTitle = QString("%1(%2%)")
                              .arg(DApplication::translate("Process.Graph.View", "Memory"))
                              .arg(QString::number(memoryPercent * 100, 'f', 1));
    QString memoryContent = QString("%1/%2")
                                .arg(formatByteCount(qulonglong(usedMemory), false, 2))
                                .arg(formatByteCount(qulonglong(totalMemory), true, 0));
    QString swapTitle = "";
    QString swapContent = "";
    if (totalSwap == 0) {
        swapTitle = QString("%1(%2)")
                        .arg(DApplication::translate("Process.Graph.View", "Swap"))
                        .arg(DApplication::translate("Process.Graph.View", "Not enabled"));
        swapContent = "";
    } else {
        swapTitle = QString("%1(%2%)")
                        .arg(DApplication::translate("Process.Graph.View", "Swap"))
                        .arg(QString::number(swapPercent * 100, 'f', 1));
        swapContent = QString("%1/%2")
                          .arg(formatByteCount(qulonglong(usedSwap), false, 2))
                          .arg(formatByteCount(qulonglong(totalSwap), true, 0));
    }

    QFontMetrics fmMem(m_contentFont);
    QFontMetrics fmMemStat(m_subContentFont);
    QRect memRect(sectionSize * 2 + 2, rect().y(),
                  fmMem.size(Qt::TextSingleLine, memoryTitle).width(), fmMem.height());
    QRect memStatRect(memRect.x(), memRect.y() + memRect.height(),
                      fmMemStat.size(Qt::TextSingleLine, memoryContent).width(),
                      fmMemStat.height());
    QRectF memIndicatorRect(0, memRect.y() + qCeil((memRect.height() - sectionSize) / 2.),
                            sectionSize, sectionSize);

    QPainterPath section;
    section.addEllipse(memIndicatorRect);
    painter.fillPath(section, memoryColor);

    m_contentFont.setWeight(QFont::Medium);
    painter.setFont(m_contentFont);
    painter.setPen(QPen(textColor));
    painter.drawText(memRect, Qt::AlignLeft | Qt::AlignVCenter, memoryTitle);

    painter.setFont(m_subContentFont);
    painter.setPen(QPen(summaryColor));
    painter.drawText(memStatRect, Qt::AlignLeft | Qt::AlignVCenter, memoryContent);

    // Draw swap summary.
    QFontMetrics fmSwap(m_contentFont);
    QFontMetrics fmSwapStat(m_subContentFont);
    QRect swapRect(memRect.x(), memStatRect.y() + memStatRect.height() + spacing,
                   fmSwap.size(Qt::TextSingleLine, swapTitle).width(), fmSwap.height());
    QRect swapStatRect(swapRect.x(), swapRect.y() + swapRect.height(),
                       fmSwapStat.size(Qt::TextSingleLine, swapContent).width(),
                       fmSwapStat.height());
    QRectF swapIndicatorRect(memIndicatorRect.x(),
                             swapRect.y() + qCeil((swapRect.height() - sectionSize) / 2.),
                             sectionSize, sectionSize);

    QPainterPath section2;
    section2.addEllipse(swapIndicatorRect);
    painter.fillPath(section2, swapColor);

    painter.setFont(m_contentFont);
    painter.setPen(QPen(textColor));
    painter.drawText(swapRect, swapTitle);

    painter.setFont(m_subContentFont);
    painter.setPen(QPen(summaryColor));
    painter.drawText(swapStatRect, Qt::AlignLeft | Qt::AlignVCenter, swapContent);

    // Draw memory ring.
    drawLoadingRing(painter, rect().x() + ringCenterPointerX, rect().y() + ringCenterPointerY,
                    outsideRingRadius, ringWidth, 270, 270, memoryForegroundColor,
                    memoryForegroundOpacity, memoryBackgroundColor, memoryBackgroundOpacity,
                    memoryPercent);

    // Draw swap ring.
    drawLoadingRing(painter, rect().x() + ringCenterPointerX, rect().y() + ringCenterPointerY,
                    insideRingRadius, ringWidth, 270, 270, swapForegroundColor,
                    swapForegroundOpacity, swapBackgroundColor, swapBackgroundOpacity, swapPercent);

    // Draw percent text.
    painter.setFont(m_memPercentFont);
    painter.setPen(QPen(numberColor));
    painter.drawText(QRect(rect().x() + ringCenterPointerX - insideRingRadius,
                           rect().y() + ringCenterPointerY - insideRingRadius, insideRingRadius * 2,
                           insideRingRadius * 2),
                     Qt::AlignHCenter | Qt::AlignVCenter,
                     QString("%1%").arg(QString::number(memoryPercent * 100, 'f', 1)));
}
