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

#include "compact_memory_monitor.h"

#include "common/common.h"
#include "system/mem.h"
#include "system/device_db.h"
#include "system/system_monitor.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DHiDPIHelper>
#include <DPalette>
#include <DStyle>

#include <QDebug>
#include <QPainter>
#include <QtMath>
#include <QPropertyAnimation>
#include <QPainterPath>
#include <QMouseEvent>

DWIDGET_USE_NAMESPACE

using namespace common;
using namespace common::format;
using namespace core::system;

CompactMemoryMonitor::CompactMemoryMonitor(QWidget *parent)
    : QWidget(parent)
{
    auto *dAppHelper = DApplicationHelper::instance();

    int statusBarMaxWidth = common::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth);
    ringCenterPointerX = rect().width() - outsideRingRadius - 4;

    setFixedHeight(150);

    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this,
            &CompactMemoryMonitor::changeTheme);

    changeTheme(dAppHelper->themeType());

    m_animation = new QPropertyAnimation(this, "progress", this);
    m_animation->setDuration(250);
    m_animation->setEasingCurve(QEasingCurve::OutQuad);
    m_animation->setStartValue(0);
    m_animation->setEndValue(1.0);
    connect(m_animation, &QVariantAnimation::valueChanged, this, &CompactMemoryMonitor::onValueChanged);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CompactMemoryMonitor::changeFont);

    m_memInfo = DeviceDB::instance()->memInfo();
    connect(SystemMonitor::instance(), &SystemMonitor::statInfoUpdated, this, &CompactMemoryMonitor::onStatInfoUpdated);
    connect(m_animation, &QPropertyAnimation::finished, this, &CompactMemoryMonitor::animationFinshed);
}

CompactMemoryMonitor::~CompactMemoryMonitor() {}

qreal CompactMemoryMonitor::progress() const
{
    return m_progress;
}
void CompactMemoryMonitor::setProgress(qreal p)
{
    m_progress = p;
}

void CompactMemoryMonitor::onStatInfoUpdated()
{
    m_animation->start();
}

void CompactMemoryMonitor::animationFinshed()
{
    m_lastMemPercent = (m_memInfo->memTotal() - m_memInfo->memAvailable()) * 1. / m_memInfo->memTotal();
    m_lastSwapPercent = (m_memInfo->swapTotal() - m_memInfo->swapFree()) * 1. / m_memInfo->swapTotal();
}

void CompactMemoryMonitor::onValueChanged()
{
    this->update();
}

void CompactMemoryMonitor::changeTheme(int themeType)
{
    switch (themeType) {
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
    m_contentFont.setPointSizeF(m_contentFont.pointSizeF() - 1);
    m_subContentFont = font;
    m_subContentFont.setPointSizeF(m_subContentFont.pointSizeF() - 1);
    m_memPercentFont = font;
    m_memPercentFont.setPointSizeF(m_memPercentFont.pointSizeF());
    m_memPercentFont.setBold(true);
}

void CompactMemoryMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int spacing = 10;
    int sectionSize = 6;

    qreal memPercent = m_lastMemPercent + ((m_memInfo->memTotal() - m_memInfo->memAvailable()) * 1. / m_memInfo->memTotal() - m_lastMemPercent) * m_progress;
    qreal swapPercent = m_lastSwapPercent + ((m_memInfo->swapTotal() - m_memInfo->swapFree()) * 1. / m_memInfo->swapTotal() - m_lastSwapPercent) * m_progress;

    // if memPercent is not valid, set it zero
    if (std::isnan(memPercent)) {
        memPercent = 0.0;
    }
    // Draw memory summary.
    // After the memory and swap space text, add a space before the brackets
    QString memoryTitle = QString("%1 (%2%)")
                          .arg(DApplication::translate("Process.Graph.View", "Memory"))
                          .arg(QString::number(memPercent * 100, 'f', 1));
    QString memoryContent = QString("%1 / %2")
                            .arg(formatUnit((m_memInfo->memTotal() - m_memInfo->memAvailable()) << 10, B, 1))
                            .arg(formatUnit(m_memInfo->memTotal() << 10, B, 1));

    QString swapTitle = "";
    QString swapContent = "";
    if (m_memInfo->swapTotal() == 0) {
        // After the memory and swap space text, add a space before the brackets
        swapTitle = QString("%1 (%2)")
                    .arg(DApplication::translate("Process.Graph.View", "Swap"))
                    .arg(DApplication::translate("Process.Graph.View", "Not enabled"));
        swapContent = "";
    } else {
        // After the memory and swap space text, add a space before the brackets
        swapTitle = QString("%1 (%2%)")
                    .arg(DApplication::translate("Process.Graph.View", "Swap"))
                    .arg(QString::number(swapPercent * 100, 'f', 1));
        swapContent = QString("%1 / %2")
                      .arg(formatUnit((m_memInfo->swapTotal() - m_memInfo->swapFree()) << 10, B, 1))
                      .arg(formatUnit(m_memInfo->swapTotal() << 10, B, 1));
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
                                      rect().width() - memRect.x() - outsideRingRadius));

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
