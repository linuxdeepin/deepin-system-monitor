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

#include "compact_cpu_monitor.h"

#include "smooth_curve_generator.h"
#include "common/common.h"
#include "process/process_db.h"
#include "system/device_db.h"
#include "model/cpu_info_model.h"
#include "model/cpu_list_model.h"
#include "base_commandlink_button.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyleHelper>
#include <DFontSizeManager>

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QMouseEvent>

DWIDGET_USE_NAMESPACE
using namespace common;

CompactCpuMonitor::CompactCpuMonitor(QWidget *parent)
    : QWidget(parent)
{
    int statusBarMaxWidth = common::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth);
    setFixedHeight(160);

    pointsNumber = int(statusBarMaxWidth / 10);

    numCPU = int(sysconf(_SC_NPROCESSORS_ONLN));

    for (int i = 0; i < numCPU; i++) {
        QList<qreal> cpuPercent;
        for (int j = 0; j < pointsNumber; j++) {
            cpuPercent.append(0);
        }

        cpuPercents.append(cpuPercent);
    }

    cpuColors << "#1094D8"
              << "#F7B300"
              << "#55D500"
              << "#C362FF"
              << "#FF2997"
              << "#00B4C7"
              << "#F8E71C"
              << "#FB1818"
              << "#8544FF"
              << "#00D7AB"
              << "#00D7AB"
              << "#FF00FF"
              << "#30BF03"
              << "#7E41F1"
              << "#2CA7F8"
              << "#A005CE";

    m_cpuInfomodel = CPUInfoModel::instance();
    connect(m_cpuInfomodel, &CPUInfoModel::modelUpdated, this, &CompactCpuMonitor::updateStatus);

    m_detailText = tr("Details");
    m_detailButton = new BaseCommandLinkButton(m_detailText, this);
    DFontSizeManager::instance()->bind(m_detailButton, DFontSizeManager::T8, QFont::Medium);
    m_detailButton->setToolTip(m_detailText);
    connect(m_detailButton, &BaseCommandLinkButton::clicked, this, &CompactCpuMonitor::onDetailInfoClicked);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CompactCpuMonitor::changeFont);
}

CompactCpuMonitor::~CompactCpuMonitor() {}

void CompactCpuMonitor::onDetailInfoClicked()
{
    setDetailButtonVisible(false);
    emit signalDetailInfoClicked();
}

void CompactCpuMonitor::updateStatus()
{
    totalCpuPercent = m_cpuInfomodel->cpuAllPercent();

    const auto &cpulist = m_cpuInfomodel->cpuPercentList();
    for (int i = 0; i < cpulist.size(); i++) {
        QList<qreal> cpuPercent = cpuPercents[i];

        cpuPercent.append(cpulist[i] / 100.0);

        if (cpuPercent.size() > pointsNumber) {
            cpuPercent.pop_front();
        }

        cpuPercents[i] = cpuPercent;
    }

    update();
}

void CompactCpuMonitor::setDetailButtonVisible(bool visible)
{
    m_detailButton->setVisible(visible);
}

void CompactCpuMonitor::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void CompactCpuMonitor::resizeItemRect()
{
    m_detailButton->setFixedSize(m_detailButton->fontMetrics().width(m_detailButton->text()) + 12, m_detailButton->fontMetrics().height() + 4);
    const QSize &detailtextSize =  m_detailButton->size();
    m_detailButton->setGeometry(this->width() - detailtextSize.width(), 0, detailtextSize.width(), detailtextSize.height());
}

void CompactCpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    auto *dAppHelper = DApplicationHelper::instance();

    // init colors
    auto palette = dAppHelper->applicationPalette();
    QColor frameColor  = palette.color(DPalette::TextTips);
    frameColor.setAlphaF(0.3);

#ifndef THEME_FALLBACK_COLOR
    QColor cpuColor = palette.color(DPalette::TextTitle);
#else
    QColor cpuColor = palette.color(DPalette::Text);
#endif

    QColor statColor = palette.color(DPalette::TextTips);
    QColor sectionColor {"#0081FF"};

    int spacing = 4;

    // Draw cpu summary.
    QFontMetrics fm(m_cpuFont);
    QFontMetrics fmStat(m_statFont);

    QString cpuText = DApplication::translate("Process.Graph.View", "CPU");
    QString cpuStatText = QString::number(totalCpuPercent, 'f', 1).append('%');

    QRect cpuRect(pointerRadius * 2 + spacing - 2, 0, fm.width(cpuText), fm.height() + 4);
    QRect sectionRect(0, cpuRect.y() + qCeil((cpuRect.height() - pointerRadius) / 2.),
                      pointerRadius, pointerRadius);
    QRect statRect(cpuRect.x() + cpuRect.width() + spacing, cpuRect.y(), fmStat.width(cpuStatText),
                   fmStat.height() + 4);

    m_detailButton->setText(m_detailButton->fontMetrics().elidedText(m_detailText, Qt::ElideRight, this->width() - statRect.right() - 2 * spacing));
    resizeItemRect();

    // draw section
    painter.setPen(sectionColor);
    QPainterPath path;
    path.addRoundedRect(sectionRect, pointerRadius, pointerRadius);
    painter.fillPath(path, sectionColor);

    // draw cpu title
    painter.setPen(cpuColor);
    painter.setFont(m_cpuFont);
    painter.drawText(cpuRect, Qt::AlignLeft | Qt::AlignVCenter, cpuText);

    // draw cpu stat
    painter.setPen(statColor);
    painter.setFont(m_statFont);
    painter.drawText(statRect, Qt::AlignLeft | Qt::AlignVCenter, cpuStatText);

    // draw grid
    QPen framePen;
    int penSize = 1;
    framePen.setColor(frameColor);
    framePen.setWidth(penSize);
    painter.setPen(framePen);

    int gridX = rect().x() + penSize;
    int gridY = cpuRect.y() + cpuRect.height() + 10;
    int gridWidth =
        rect().width() - 3 - ((rect().width() - 3 - penSize) % (gridSize + penSize)) - penSize;
    int gridHeight = cpuRenderMaxHeight + 8 * penSize;

    painter.setRenderHint(QPainter::Antialiasing, false);
    QPainterPath framePath;
    QRect gridFrame(gridX, gridY, gridWidth, gridHeight);
    framePath.addRect(gridFrame);
    painter.drawPath(framePath);

    // Draw grid.
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 2;
    dashes << 2 << space;
    gridPen.setDashPattern(dashes);
    gridPen.setColor(frameColor);
    painter.setPen(gridPen);

    int gridLineX = gridX;
    while (gridLineX + gridSize + penSize < gridX + gridWidth) {
        gridLineX += gridSize + penSize;
        painter.drawLine(gridLineX, gridY + 1, gridLineX, gridY + gridHeight - 1);
    }
    int gridLineY = gridY;
    while (gridLineY + gridSize + penSize < gridY + gridHeight) {
        gridLineY += gridSize + penSize;
        painter.drawLine(gridX + 1, gridLineY, gridX + gridWidth - 1, gridLineY);
    }

    painter.setRenderHint(QPainter::Antialiasing, true);

    // clip internal area of the region
    QPainterPath clip;
    clip.addRect(gridFrame);
    painter.setClipPath(clip);

    painter.translate(gridFrame.x(), gridFrame.y());

    qreal strokeWidth = 1.2;                            // for now, set as 1.2 temporarily
    int drawWidth = gridFrame.width() - penSize * 2;    // exclude left/right most border
    int drawHeight = gridFrame.height() - penSize * 2;  // exclude top/bottom most border
    qreal offsetX = drawWidth + penSize;
    qreal deltaX = drawWidth * 1.0 / (pointsNumber - 3);
    // enum cpu
    for (int i = cpuPercents.size() - 1; i >= 0; i--) {
        // set stroke color
        QColor c = cpuColors[i % cpuColors.size()];
        painter.setPen(QPen(c, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        QPainterPath Painterpath;
        // move to the first point (from right to left)
        qreal y = (1.0 - cpuPercents[i].at(pointsNumber - 1)) * drawHeight + penSize;
        Painterpath.moveTo(offsetX, y);

        for (int j = pointsNumber - 2; j >= 0; j--) {
            // method#1: draw Bezier curve
            Painterpath.cubicTo(offsetX - (pointsNumber - j - 1 - 0.5) * deltaX,
                                (1.0 - cpuPercents[i][j + 1]) * drawHeight + penSize + 0.5,
                                offsetX - (pointsNumber - j - 1 - 0.5) * deltaX,
                                (1.0 - cpuPercents[i][j]) * drawHeight + penSize + 0.5,
                                offsetX - ((pointsNumber - j - 1) * deltaX),
                                (1.0 - cpuPercents[i][j]) * drawHeight + penSize + 0.5);

            // method#2: draw line instead
            // path.lineTo(offsetX - ((pointsNumber - j - 1) * deltaX),
            //            (1.0 - cpuPercents[i][j]) * drawHeight + penSize + 0.5);
        }
        painter.drawPath(Painterpath);
    }

    setFixedHeight(gridFrame.y() + gridFrame.height() + penSize);
}

void CompactCpuMonitor::changeFont(const QFont &font)
{
    m_cpuFont = font;
    m_cpuFont.setWeight(QFont::Medium);
    m_cpuFont.setPointSizeF(m_cpuFont.pointSizeF() - 1);
    m_statFont = font;
    m_statFont.setPointSizeF(m_statFont.pointSizeF() - 1);

    resizeItemRect();
}
