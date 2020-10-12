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

#include "compact_network_monitor.h"

#include "process/system_monitor.h"
#include "process/stats_collector.h"
#include "smooth_curve_generator.h"
#include "utils.h"
#include "constant.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QtMath>
#include <QPainterPath>

DWIDGET_USE_NAMESPACE

using namespace Utils;

CompactNetworkMonitor::CompactNetworkMonitor(QWidget *parent)
    : QWidget(parent)
{
    auto *dAppHelper = DApplicationHelper::instance();
    auto margin = DStyle::pixelMetric(style(), DStyle::PM_ContentsMargins);

    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth - margin * 2);
    setFixedHeight(150);

    pointsNumber = int(statusBarMaxWidth / 5.4);

    downloadSpeeds = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        downloadSpeeds->append(0);
    }

    uploadSpeeds = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        uploadSpeeds->append(0);
    }

    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this,
            &CompactNetworkMonitor::changeTheme);
    changeTheme(dAppHelper->themeType());

    auto *smo = SystemMonitor::instance();
    Q_ASSERT(smo != nullptr);
    connect(smo->jobInstance(), &StatsCollector::networkStatInfoUpdated,
            this, &CompactNetworkMonitor::updateStatus);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()),
            &DApplication::fontChanged,
            this,
            &CompactNetworkMonitor::changeFont);
}

CompactNetworkMonitor::~CompactNetworkMonitor()
{
    delete downloadSpeeds;
    delete uploadSpeeds;
}

void CompactNetworkMonitor::updateStatus(qulonglong tRecvBytes, qulonglong tSentBytes,
                                         qreal recvBps, qreal sentBps)
{
    m_totalRecvBytes = tRecvBytes;
    m_totalSentBytes = tSentBytes;
    m_recvBps = recvBps;
    m_sentBps = sentBps;

    // Init download path.
    downloadSpeeds->append(m_recvBps);

    if (downloadSpeeds->size() > pointsNumber) {
        downloadSpeeds->pop_front();
    }

    double downloadMaxHeight = 0;
    for (int i = 0; i < downloadSpeeds->size(); i++) {
        if (downloadSpeeds->at(i) > downloadMaxHeight) {
            downloadMaxHeight = downloadSpeeds->at(i);
        }
    }

    // Init upload path.
    uploadSpeeds->append(m_sentBps);

    if (uploadSpeeds->size() > pointsNumber) {
        uploadSpeeds->pop_front();
    }

    QList<QPointF> uploadPoints;

    double uploadMaxHeight = 0;
    for (int i = 0; i < uploadSpeeds->size(); i++) {
        if (uploadSpeeds->at(i) > uploadMaxHeight) {
            uploadMaxHeight = uploadSpeeds->at(i);
        }
    }

    qreal maxHeight = std::max(uploadMaxHeight, downloadMaxHeight);
    // top/bottom margin
    int modDownloadRenderMaxHeight = downloadRenderMaxHeight - 2;
    int modUploadRenderMaxHeight = uploadRenderMaxHeight - 2;

    QList<QPointF> downloadPoints;
    for (int i = 0; i < downloadSpeeds->size(); i++) {
        if (downloadMaxHeight < modDownloadRenderMaxHeight) {
            downloadPoints.append(QPointF(i * 5, downloadSpeeds->at(i)));
        } else {
            qreal scale = downloadSpeeds->at(i) * modDownloadRenderMaxHeight / maxHeight;
            if (scale > 0 && scale < 0.5) {
                scale = 0.5;
            }
            downloadPoints.append(QPointF(i * 5, scale));
        }
    }

    downloadPath = SmoothCurveGenerator::generateSmoothCurve(downloadPoints);

    for (int i = 0; i < uploadSpeeds->size(); i++) {
        if (uploadMaxHeight < modUploadRenderMaxHeight) {
            uploadPoints.append(QPointF(i * 5, uploadSpeeds->at(i)));
        } else {
            qreal scale = uploadSpeeds->at(i) * modUploadRenderMaxHeight / maxHeight;
            if (scale > 0 && scale < 0.5) {
                scale = 0.5;
            }
            uploadPoints.append(QPointF(i * 5, scale));
        }
    }

    uploadPath = SmoothCurveGenerator::generateSmoothCurve(uploadPoints);

    update();
}

void CompactNetworkMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int bulletSize = 6;
    int padleft = bulletSize * 2 + 2;
    QRect contentRect(padleft, 0, rect().x() + rect().width() - padleft, 1);

    auto spacing = DStyle::pixelMetric(style(), DStyle::PM_ContentsSpacing);
    auto margin = DStyle::pixelMetric(style(), DStyle::PM_ContentsMargins);

    // Draw network summary.
    QString recvTitle = DApplication::translate("Process.Graph.View", "Download");
    QString recvContent = formatUnit(m_recvBps, B, 1, true);
    QString recvTotalTitle = DApplication::translate("Process.Graph.View", "Total Received");
    QString recvTotalContent = formatUnit(m_totalRecvBytes, B, 1);

    QString sentTitle = DApplication::translate("Process.Graph.View", "Upload");
    QString sentContent = formatUnit(m_sentBps, B, 1, true);
    QString sentTotalTitle = DApplication::translate("Process.Graph.View", "Total Sent");
    QString sentTotalContent = formatUnit(m_totalSentBytes, B, 1);

    QFontMetrics fmContent(m_contentFont);
    QFontMetrics fmSubContent(m_subContentFont);
    QRect crect11(contentRect.x(), contentRect.y(), qCeil((contentRect.width() - spacing) / 2),
                  fmContent.height());
    QRect crect12(qCeil((contentRect.width() - spacing) / 2) + spacing, crect11.y(),
                  contentRect.width() - crect11.width() - spacing, crect11.height());
    QRect crect21(contentRect.x(), crect11.y() + crect11.height(), crect11.width(),
                  fmSubContent.height());
    QRect crect22(crect12.x(), crect21.y(), crect12.width(), crect21.height());
    QRect crect31(contentRect.x(), crect21.y() + crect21.height() + spacing, crect11.width(),
                  crect11.height());
    QRect crect32(crect12.x(), crect31.y(), crect12.width(), crect11.height());
    QRect crect41(contentRect.x(), crect31.y() + crect31.height(), crect11.width(),
                  crect21.height());
    QRect crect42(crect12.x(), crect41.y(), crect12.width(), crect21.height());
    QRectF r1Ind(0, crect11.y() + qCeil((crect11.height() - bulletSize) / 2.), bulletSize,
                 bulletSize);
    QRectF r2Ind(0, crect31.y() + qCeil((crect31.height() - bulletSize) / 2.), bulletSize,
                 bulletSize);

    painter.setPen(textColor);
    painter.setFont(m_contentFont);
    painter.drawText(crect11, Qt::AlignLeft | Qt::AlignVCenter,
                     fmContent.elidedText(recvTitle, Qt::ElideRight, crect11.width()));
    painter.drawText(crect31, Qt::AlignLeft | Qt::AlignVCenter,
                     fmContent.elidedText(sentTitle, Qt::ElideRight, crect31.width()));
    painter.drawText(crect12, Qt::AlignLeft | Qt::AlignVCenter,
                     fmContent.elidedText(recvTotalTitle, Qt::ElideRight, crect12.width()));
    painter.drawText(crect32, Qt::AlignLeft | Qt::AlignVCenter,
                     fmContent.elidedText(sentTotalTitle, Qt::ElideRight, crect32.width()));

    painter.setPen(summaryColor);
    painter.setFont(m_subContentFont);
    painter.drawText(crect21, Qt::AlignLeft | Qt::AlignVCenter,
                     fmSubContent.elidedText(recvContent, Qt::ElideRight, crect21.width()));
    painter.drawText(crect41, Qt::AlignLeft | Qt::AlignVCenter,
                     fmSubContent.elidedText(sentContent, Qt::ElideRight, crect41.width()));
    painter.drawText(crect22, Qt::AlignLeft | Qt::AlignVCenter,
                     fmSubContent.elidedText(recvTotalContent, Qt::ElideRight, crect22.width()));
    painter.drawText(crect42, Qt::AlignLeft | Qt::AlignVCenter,
                     fmSubContent.elidedText(sentTotalContent, Qt::ElideRight, crect42.width()));

    QPainterPath path1, path2;
    path1.addEllipse(r1Ind);
    path2.addEllipse(r2Ind);

    painter.fillPath(path1, m_recvIndicatorColor);
    painter.fillPath(path2, m_sentIndicatorColor);

    // Draw background grid.
    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen framePen;
    int penSize = 1;
    framePen.setColor(m_frameColor);
    framePen.setWidth(penSize);
    painter.setPen(framePen);

    int gridX = rect().x() + penSize;
    int gridY = rect().y() + crect42.y() + crect42.height() + margin;
    int gridWidth =
        rect().width() - 3 - ((rect().width() - 3 - penSize) % (gridSize + penSize)) - penSize;
    int gridHeight = downloadRenderMaxHeight + uploadRenderMaxHeight + 4 * penSize;

    QPainterPath framePath;
    QRect gridFrame(gridX, gridY, gridWidth, gridHeight);
    framePath.addRect(gridFrame);
    painter.drawPath(framePath);

    // Draw grid.
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 2;
    dashes << 2 << space;
    gridPen.setColor(m_frameColor);
    gridPen.setWidth(penSize);
    gridPen.setDashPattern(dashes);
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

    QPainterPath clip;
    clip.addRect(gridFrame);
    painter.setClipPath(clip);

    qreal networkCurveWidth = 1.2;

    QBrush recvBrush(recvColor), sentBrush(sentColor);

    painter.translate(gridFrame.x() + 2, gridFrame.y() + gridFrame.height() / 2 - 2);
    painter.scale(1, -1);
    painter.setPen(QPen(recvBrush, networkCurveWidth));
    painter.drawPath(downloadPath);

    painter.translate(0, uploadWaveformsRenderOffsetY);
    painter.scale(1, -1);
    painter.setPen(QPen(sentBrush, networkCurveWidth));
    painter.drawPath(uploadPath);

    setFixedHeight(gridFrame.y() + gridFrame.height() + penSize);
}

void CompactNetworkMonitor::changeTheme(DGuiApplicationHelper::ColorType themeType)
{
    Q_UNUSED(themeType);

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
#ifndef THEME_FALLBACK_COLOR
    textColor = palette.color(DPalette::TextTitle);
#else
    textColor = palette.color(DPalette::Text);
#endif

    summaryColor = palette.color(DPalette::TextTips);
    m_frameColor = palette.color(DPalette::FrameBorder);
}

void CompactNetworkMonitor::changeFont(const QFont &font)
{
    m_contentFont = font;
    m_contentFont.setWeight(QFont::Medium);
    m_contentFont.setPointSize(m_contentFont.pointSize() - 1);
    m_subContentFont = font;
    m_subContentFont.setPointSize(m_subContentFont.pointSize() - 1);
}
