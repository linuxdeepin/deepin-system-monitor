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
#include "constant.h"
#include "model/mem_info_model.h"
#include "model/mem_stat_model.h"

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

DWIDGET_USE_NAMESPACE

using namespace common;
using namespace common::format;

CompactMemoryMonitor::CompactMemoryMonitor(QWidget *parent)
    : QWidget(parent)
{
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    auto *dAppHelper = DApplicationHelper::instance();
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    int statusBarMaxWidth = common::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth - margin * 2);
    ringCenterPointerX = rect().width() - outsideRingRadius - 4;

    setFixedHeight(150);

    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this,
            &CompactMemoryMonitor::changeTheme);
    m_themeType = dAppHelper->themeType();
    changeTheme(m_themeType);

    m_animation = new QPropertyAnimation(this, "progress", this);
    m_animation->setDuration(250);
    m_animation->setEasingCurve(QEasingCurve::OutQuad);
    m_animation->setStartValue(0);
    m_animation->setEndValue(1.0);
    connect(m_animation, &QVariantAnimation::valueChanged, this, [ = ]() { update(); });

    TimePeriod period(TimePeriod::k1Min, {2, 0});
    m_model = new MemInfoModel(period, this);
    connect(m_model, &MemInfoModel::modelUpdated, this, [ = ]() {
        m_animation->start();
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

void CompactMemoryMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    auto statModel = m_model->memStatModel().lock();

    auto nrow = statModel->rowCount();

    QModelIndex prevUsedMemIndex, prevTotalMemIndex, prevUsedSwapIndex, prevTotalSwapIndex;
    QModelIndex curUsedMemIndex, curTotalMemIndex, curUsedSwapIndex, curTotalSwapIndex;

    if (nrow > 1) {
        prevUsedMemIndex = statModel->index(nrow - 2, MemStatModel::kStatUsedMem);
        prevTotalMemIndex = statModel->index(nrow - 2, MemStatModel::kStatTotalMem);
        prevUsedSwapIndex = statModel->index(nrow - 2, MemStatModel::kStatUsedSwap);
        prevTotalSwapIndex = statModel->index(nrow - 2, MemStatModel::kStatTotalSwap);
    }

    if (nrow > 0) {
        curUsedMemIndex = statModel->index(nrow - 1, MemStatModel::kStatUsedMem);
        curTotalMemIndex = statModel->index(nrow - 1, MemStatModel::kStatTotalMem);
        curUsedSwapIndex = statModel->index(nrow - 1, MemStatModel::kStatUsedSwap);
        curTotalSwapIndex = statModel->index(nrow - 1, MemStatModel::kStatTotalSwap);
    }

    qulonglong prevUsedMem {}, prevTotalMem {}, prevUsedSwap {}, prevTotalSwap {};
    qulonglong curUsedMem {}, curTotalMem {}, curUsedSwap {}, curTotalSwap {};

    if (curUsedMemIndex.isValid())
        curUsedMem = curUsedMemIndex.data(MemStatModel::kValueRole).toULongLong();
    if (curTotalMemIndex.isValid())
        curTotalMem = curTotalMemIndex.data(MemStatModel::kValueRole).toULongLong();
    if (curUsedSwapIndex.isValid())
        curUsedSwap = curUsedSwapIndex.data(MemStatModel::kValueRole).toULongLong();
    if (curTotalSwapIndex.isValid())
        curTotalSwap = curTotalSwapIndex.data(MemStatModel::kValueRole).toULongLong();

    if (prevUsedMemIndex.isValid())
        prevUsedMem = prevUsedMemIndex.data(MemStatModel::kValueRole).toULongLong();
    if (prevTotalMemIndex.isValid())
        prevTotalMem = prevTotalMemIndex.data(MemStatModel::kValueRole).toULongLong();
    if (prevUsedSwapIndex.isValid())
        prevUsedSwap = prevUsedSwapIndex.data(MemStatModel::kValueRole).toULongLong();
    if (prevTotalSwapIndex.isValid())
        prevTotalSwap = prevTotalSwapIndex.data(MemStatModel::kValueRole).toULongLong();

    auto memdiff = qlonglong(curUsedMem - prevUsedMem);
    auto memPercent = (prevUsedMem + m_progress * memdiff) / curTotalMem;
    if (memPercent > 100.) {
        memPercent = 100.;
    }

    auto swpdiff = qlonglong(curUsedSwap - prevUsedSwap);
    qreal swapPercent;
    if (curTotalSwap == 0) {
        swapPercent = 0;
    } else {
        swapPercent = (prevUsedSwap + m_progress * swpdiff) / curTotalSwap;
        if (swapPercent > 100.) {
            swapPercent = 100.;
        }
    }

    int spacing = 10;
    int sectionSize = 6;

    // Draw memory summary.
    QString memoryTitle = QString("%1(%2%)")
                          .arg(DApplication::translate("Process.Graph.View", "Memory"))
                          .arg(QString::number(memPercent * 100, 'f', 1));
    QString memoryContent = QString("%1 / %2")
                            .arg(curUsedMemIndex.data().toString())
                            .arg(curTotalMemIndex.data().toString());
    QString swapTitle = "";
    QString swapContent = "";
    if (m_model->swapTotal() == 0) {
        swapTitle = QString("%1(%2)")
                    .arg(DApplication::translate("Process.Graph.View", "Swap"))
                    .arg(DApplication::translate("Process.Graph.View", "Not enabled"));
        swapContent = "";
    } else {
        swapTitle = QString("%1(%2%)")
                    .arg(DApplication::translate("Process.Graph.View", "Swap"))
                    .arg(QString::number(swapPercent * 100, 'f', 1));
        swapContent = QString("%1 / %2")
                      .arg(curUsedSwapIndex.data().toString())
                      .arg(curTotalSwapIndex.data().toString());
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
