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
#include "process/stats_collector.h"

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

    setFixedHeight(150);

    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this,
            &CompactMemoryMonitor::changeTheme);
    m_themeType = dAppHelper->themeType();
    changeTheme(m_themeType);

    auto *smo = SystemMonitor::instance();
    Q_ASSERT(smo != nullptr);
    connect(smo->jobInstance(), &StatsCollector::memStatInfoUpdated,
            this, &CompactMemoryMonitor::updateStatus);

    m_animation = new QPropertyAnimation(this, "progress", this);
    m_animation->setDuration(250);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_animation, &QVariantAnimation::valueChanged, [ = ]() {
        update();
    });

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

void CompactMemoryMonitor::updateStatus(qulonglong uMemory, qulonglong tMemory,
                                        qulonglong uSwap, qulonglong tSwap)
{
    if ((uMemory != m_usedMemory) || (tMemory != m_totalMemory)
            || (uSwap != m_usedSwap) || (tSwap != m_totalSwap)) {
        m_prevUsedMemory = m_usedMemory;
        m_prevUsedSwap = m_usedSwap;

        m_usedMemory = uMemory;
        m_totalMemory = tMemory;
        m_usedSwap = uSwap;
        m_totalSwap = tSwap;

        m_animation->start();
    }
}

void CompactMemoryMonitor::paintEvent(QPaintEvent *)
{
    // Init.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    auto memdiff = qlonglong(m_usedMemory - m_prevUsedMemory);
    auto memPercent = (m_prevUsedMemory + m_progress * memdiff) / m_totalMemory;

    auto swpdiff = qulonglong(m_usedSwap - m_prevUsedSwap);
    qreal swapPercent;
    if (m_totalSwap == 0) {
        swapPercent = 0;
    } else {
        swapPercent = (m_prevUsedSwap + m_progress * swpdiff) / m_totalSwap;
    }

    int spacing = 10;
    int sectionSize = 6;

    // Draw memory summary.
    QString memoryTitle = QString("%1(%2%)")
                          .arg(DApplication::translate("Process.Graph.View", "Memory"))
                          .arg(QString::number(memPercent * 100, 'f', 1));
    QString memoryContent = QString("%1/%2")
                            .arg(formatUnit(m_usedMemory, KB, 2))
                            .arg(formatUnit(m_totalMemory, KB, 1));
    QString swapTitle = "";
    QString swapContent = "";
    if (m_totalSwap == 0) {
        swapTitle = QString("%1(%2)")
                    .arg(DApplication::translate("Process.Graph.View", "Swap"))
                    .arg(DApplication::translate("Process.Graph.View", "Not enabled"));
        swapContent = "";
    } else {
        swapTitle = QString("%1(%2%)")
                    .arg(DApplication::translate("Process.Graph.View", "Swap"))
                    .arg(QString::number(swapPercent * 100, 'f', 1));
        swapContent = QString("%1/%2")
                      .arg(formatUnit(m_usedSwap * 1024, B, 2))
                      .arg(formatUnit(m_totalSwap, KB, 1));
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
    painter.drawText(memRect, Qt::AlignLeft | Qt::AlignVCenter,
                     fmMem.elidedText(memoryTitle, Qt::ElideRight,
                                      rect().width() - memRect.x() - outsideRingRadius - 105));

    painter.setFont(m_subContentFont);
    painter.setPen(QPen(summaryColor));
    painter.drawText(memStatRect, Qt::AlignLeft | Qt::AlignVCenter,
                     fmMemStat.elidedText(memoryContent, Qt::ElideRight, memStatRect.width()));

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
                    memPercent);

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
                     QString("%1%").arg(QString::number(memPercent * 100, 'f', 1)));

    setFixedHeight(
        std::max(swapStatRect.y() + swapStatRect.height(), rect().y() + ringCenterPointerY * 2));
}
