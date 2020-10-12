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

#include "memory_monitor.h"

#include "process/system_monitor.h"
#include "process/stats_collector.h"
#include "gui/ui_common.h"
#include "utils.h"
#include "constant.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>

#include <QDebug>
#include <QPainter>
#include <QtMath>
#include <QPropertyAnimation>
#include <QPainterPath>

DWIDGET_USE_NAMESPACE

using namespace Utils;

MemoryMonitor::MemoryMonitor(QWidget *parent)
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

    m_usedMemory = 0;
    m_totalMemory = 0;
    m_usedSwap = 0;
    m_totalSwap = 0;

    setFixedHeight(160);

    connect(dAppHelper, &DApplicationHelper::themeTypeChanged,
            this, &MemoryMonitor::changeTheme);
    m_themeType = dAppHelper->themeType();
    changeTheme(m_themeType);

    auto *smo = SystemMonitor::instance();
    Q_ASSERT(smo != nullptr);
    connect(smo->jobInstance(), &StatsCollector::memStatInfoUpdated,
            this, &MemoryMonitor::updateStatus);

    m_animation = new QPropertyAnimation(this, "progress", this);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setDuration(250);
    connect(m_animation, &QPropertyAnimation::valueChanged, [ = ]() {
        update();
    });

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &MemoryMonitor::changeFont);
}

MemoryMonitor::~MemoryMonitor() {}

void MemoryMonitor::changeTheme(DApplicationHelper::ColorType themeType)
{
    m_themeType = themeType;

    switch (m_themeType) {
    case DApplicationHelper::LightType:
        memoryBackgroundColor = "#000000";
        swapBackgroundColor = "#000000";

        m_icon = QIcon(iconPathFromQrc("light/icon_memory_light.svg"));
        break;
    case DApplicationHelper::DarkType:
        memoryBackgroundColor = "#FFFFFF";
        swapBackgroundColor = "#FFFFFF";

        m_icon = QIcon(iconPathFromQrc("dark/icon_memory_light.svg"));
        break;
    default:
        break;
    }

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
#ifndef THEME_FALLBACK_COLOR
    numberColor = palette.color(DPalette::TextTitle);
    ltextColor = palette.color(DPalette::TextTitle);
#else
    numberColor = palette.color(DPalette::Text);
    ltextColor = palette.color(DPalette::Text);
#endif

    textColor = palette.color(DPalette::Text);
    summaryColor = palette.color(DPalette::TextTips);
}

void MemoryMonitor::updateStatus(qulonglong uMemory, qulonglong tMemory,
                                 qulonglong uSwap, qulonglong tSwap)
{
    if ((uMemory != m_usedMemory) || (tMemory != m_totalMemory) || (uSwap != m_usedSwap) ||
            (tSwap != m_totalSwap)) {
        m_prevUsedMemory = m_usedMemory;
        m_prevUsedSwap = m_usedSwap;

        m_usedMemory = uMemory;
        m_totalMemory = tMemory;
        m_usedSwap = uSwap;
        m_totalSwap = tSwap;

        m_animation->start();
    }
}

void MemoryMonitor::changeFont(const QFont &font)
{
    m_titleFont = font;
    m_titleFont.setPointSize(m_titleFont.pointSize() + 12);
    m_contentFont = font;
    m_contentFont.setWeight(QFont::Medium);
    m_contentFont.setPointSize(m_contentFont.pointSize() - 1);
    m_subContentFont = font;
    m_subContentFont.setPointSize(m_subContentFont.pointSize() - 1);
    m_memPercentFont = font;
    m_memPercentFont.setPointSize(m_memPercentFont.pointSize());
    m_memPercentFont.setBold(true);
}

void MemoryMonitor::paintEvent(QPaintEvent *)
{
    // Init.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    auto memdiff = qlonglong(m_usedMemory - m_prevUsedMemory);
    auto memoryPercent = (m_prevUsedMemory + memdiff * m_progress) / m_totalMemory;
    if (memoryPercent > 100.) {
        memoryPercent = 100.;
    }

    qreal swapPercent;
    auto swpdiff = qlonglong(m_usedSwap - m_prevUsedSwap);
    if (m_totalSwap == 0) {
        swapPercent = 0;
    } else {
        swapPercent = (m_prevUsedSwap + swpdiff * m_progress) / m_totalSwap;
        if (swapPercent > 100.) {
            swapPercent = 100.;
        }
    }

    int iconSize = 24;

    QString title = DApplication::translate("Process.Graph.Title", "Memory");
    QFontMetrics fm(m_titleFont);
    QRect titleRect(rect().x() + iconSize + 4, rect().y(),
                    fm.size(Qt::TextSingleLine, title).width(), fm.height());

    // Draw icon.
    QRect iconRect(rect().x(), titleRect.y() + qCeil((titleRect.height() - iconSize) / 2.) + 2,
                   iconSize, iconSize);
    m_icon.paint(&painter, iconRect);

    // Draw title.
    painter.setPen(QPen(textColor));
    painter.setFont(m_titleFont);
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                     fm.elidedText(title, Qt::ElideRight,
                                   rect().width() - titleRect.x() - outsideRingRadius - 50));

    int spacing = 10;
    int sectionSize = 6;

    // Draw memory summary.
    QString memoryTitle = QString("%1(%2%)")
                          .arg(DApplication::translate("Process.Graph.View", "Memory"))
                          .arg(QString::number(memoryPercent * 100, 'f', 1));
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
    QRect memRect(sectionSize * 2 + 4, titleRect.y() + titleRect.height() + spacing,
                  fmMem.size(Qt::TextSingleLine, memoryTitle).width(), fmMem.height());
    QRect memStatRect(memRect.x(), memRect.y() + memRect.height(),
                      fmMemStat.size(Qt::TextSingleLine, memoryContent).width(),
                      fmMemStat.height());
    QRectF memIndicatorRect(3, memRect.y() + qCeil((memRect.height() - sectionSize) / 2.),
                            sectionSize, sectionSize);

    QPainterPath section;
    section.addEllipse(memIndicatorRect);
    painter.fillPath(section, memoryColor);

    m_contentFont.setWeight(QFont::Medium);
    painter.setFont(m_contentFont);
    painter.setPen(QPen(ltextColor));
    painter.drawText(memRect, Qt::AlignLeft | Qt::AlignVCenter,
                     fmMem.elidedText(memoryTitle, Qt::ElideRight,
                                      ringCenterPointerX - memRect.x() - outsideRingRadius));

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
    painter.setPen(QPen(ltextColor));
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

    int memoryFixedHeight = swapStatRect.y() + swapStatRect.height();
    if (ringCenterPointerY + outsideRingRadius > swapStatRect.y() + swapStatRect.height()) {
        memoryFixedHeight = ringCenterPointerY + outsideRingRadius;
    }
    setFixedHeight(memoryFixedHeight + 1);
}
