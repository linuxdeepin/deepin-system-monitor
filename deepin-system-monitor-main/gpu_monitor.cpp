// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gpu_monitor.h"

#include "gui/ui_common.h"
#include "common/common.h"
#include "model/gpu_info_model.h"
#include "system/system_monitor.h"

#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif
#include <DPalette>
#include <DStyle>

#include <QDebug>
#include <QPainter>
#include <QtMath>
#include <QPropertyAnimation>
#include <QPainterPath>
#include <QMouseEvent>
#include "ddlog.h"

DWIDGET_USE_NAMESPACE

using namespace common;
using namespace common::format;
using namespace DDLog;

GpuMonitor::GpuMonitor(QWidget *parent)
    : QWidget(parent)
{
    qCDebug(app) << "GpuMonitor constructor";
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif

    int statusBarMaxWidth = common::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth);
    setFixedHeight(240);
    // Calculate ring position after widget size is set
    ringCenterPointerX = statusBarMaxWidth - outsideRingRadius;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &GpuMonitor::changeTheme);
#else
    connect(dAppHelper, &DGuiApplicationHelper::themeTypeChanged, this, &GpuMonitor::changeTheme);
#endif
    m_themeType = dAppHelper->themeType();
    changeTheme(m_themeType);

    m_animation = new QPropertyAnimation(this, "progress", this);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setDuration(10);
    connect(m_animation, &QPropertyAnimation::valueChanged, [ = ]() {
        update();
    });
    connect(m_animation, &QPropertyAnimation::finished, this, &GpuMonitor::onAnimationFinished);

    m_gpuInfoModel = GPUInfoModel::instance();
    connect(m_gpuInfoModel, &GPUInfoModel::modelUpdated, this, &GpuMonitor::updateStatus);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &GpuMonitor::changeFont);
}

GpuMonitor::~GpuMonitor()
{
}

void GpuMonitor::updateStatus()
{
    qCDebug(app) << "GpuMonitor updateStatus";
    m_lastGpuPercent = m_gpuInfoModel->gpuAllUtilization() / 100.0;
    m_animation->start();
}

void GpuMonitor::onAnimationFinished()
{
    qCDebug(app) << "GpuMonitor onAnimationFinished";
    m_lastGpuPercent = m_gpuInfoModel->gpuAllUtilization() / 100.0;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void GpuMonitor::changeTheme(DApplicationHelper::ColorType themeType)
#else
void GpuMonitor::changeTheme(DGuiApplicationHelper::ColorType themeType)
#endif
{
    qCDebug(app) << "GpuMonitor changeTheme, themeType:" << themeType;
    m_themeType = themeType;

    switch (m_themeType) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    case DApplicationHelper::LightType:
#else
    case DGuiApplicationHelper::LightType:
#endif
        ringBackgroundColor = "#000000";
        m_icon = QIcon(iconPathFromQrc("light/icon_gpu_light.svg"));
        break;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    case DApplicationHelper::DarkType:
#else
    case DGuiApplicationHelper::DarkType:
#endif
        ringBackgroundColor = "#FFFFFF";
        m_icon = QIcon(iconPathFromQrc("dark/icon_gpu_light.svg"));
        break;
    default:
        break;
    }

    // init colors
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif
    auto palette = dAppHelper->applicationPalette();

#ifndef THEME_FALLBACK_COLOR
    numberColor = palette.color(DPalette::TextTitle);
#else
    numberColor = palette.color(DPalette::Text);
#endif

    textColor = palette.color(DPalette::Text);
    summaryColor = palette.color(DPalette::TextTips);

    update();
}

void GpuMonitor::changeFont(const QFont &font)
{
    qCDebug(app) << "GpuMonitor changeFont";
    m_titleFont = font;
    m_titleFont.setPointSizeF(m_titleFont.pointSizeF() + 12);
    m_contentFont = font;
    m_contentFont.setWeight(QFont::Medium);
    m_contentFont.setPointSizeF(m_contentFont.pointSizeF() - 1);
    m_subContentFont = font;
    m_subContentFont.setPointSizeF(m_subContentFont.pointSizeF() - 1);
    m_percentFont = font;
    m_percentFont.setPointSizeF(m_percentFont.pointSizeF());
    m_percentFont.setBold(true);
}

void GpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Recalculate ring position based on current width
    ringCenterPointerX = rect().width() - outsideRingRadius;

    int iconSize = 24;

    QString title = DApplication::translate("Process.Graph.View", "GPU");
    QFontMetrics fm(m_titleFont);
    QRect titleRect(rect().x() + iconSize, rect().y(),
                    fm.size(Qt::TextSingleLine, title).width(), fm.height());

    // Draw icon
    QRect iconRect(rect().x() - 4, titleRect.y() + qCeil((titleRect.height() - iconSize) / 2.) + 2,
                   iconSize, iconSize);
    m_icon.paint(&painter, iconRect);

    // Draw title
    painter.setPen(QPen(textColor));
    painter.setFont(m_titleFont);
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                     fm.elidedText(title, Qt::ElideRight,
                                   rect().width() - titleRect.x() - outsideRingRadius - 50));

    int spacing = 10;
    int sectionSize = 6;

    qreal gpuPercent = m_gpuInfoModel->gpuAllUtilization() / 100.0;
    qreal currentPercent = m_lastGpuPercent + (gpuPercent - m_lastGpuPercent) * m_progress;

    // Draw GPU utilization summary
    QString gpuTitle = QString("%1 (%2%)")
                       .arg(DApplication::translate("Process.Graph.View", "Utilization"))
                       .arg(QString::number(currentPercent * 100, 'f', 1));

    QString gpuContent;
    int gpuCount = m_gpuInfoModel->gpuCount();
    if (gpuCount > 0) {
        gpuContent = QString("%1: %2").arg(m_gpuInfoModel->gpuName(0))
                     .arg(m_gpuInfoModel->gpuVendor(0));
    } else {
        gpuContent = DApplication::translate("Process.Graph.View", "No GPU detected");
    }

    // Draw memory info if available
    quint64 totalMem = m_gpuInfoModel->gpuTotalMemory();
    quint64 usedMem = m_gpuInfoModel->gpuUsedMemory();
    QString memTitle;
    QString memContent;
    if (totalMem > 0) {
        qreal memPercent = m_gpuInfoModel->gpuAllMemoryUtilization();
        memTitle = QString("%1 (%2%)")
                   .arg(DApplication::translate("Process.Graph.View", "Memory"))
                   .arg(QString::number(memPercent, 'f', 1));
        memContent = QString("%1 / %2")
                     .arg(formatUnit_memory_disk(usedMem, B, 1))
                     .arg(formatUnit_memory_disk(totalMem, B, 1));
    }

    titleRect.translate(0, 7);
    QFontMetrics fmGpu(m_contentFont);
    QFontMetrics fmGpuStat(m_subContentFont);
    QRect gpuRect(sectionSize * 2, titleRect.y() + titleRect.height(),
                  fmGpu.size(Qt::TextSingleLine, gpuTitle).width(), fmGpu.height());
    QRect gpuStatRect(gpuRect.x(), gpuRect.y() + gpuRect.height(),
                      fmGpuStat.size(Qt::TextSingleLine, gpuContent).width(),
                      fmGpuStat.height());
    QRectF gpuIndicatorRect(0, gpuRect.y() + qCeil((gpuRect.height() - sectionSize) / 2.),
                            sectionSize, sectionSize);

    QPainterPath section;
    section.addEllipse(gpuIndicatorRect);
    painter.fillPath(section, memoryColor);

    m_contentFont.setWeight(QFont::Medium);
    painter.setFont(m_contentFont);
    painter.setPen(QPen(numberColor));
    painter.drawText(gpuRect, Qt::AlignLeft | Qt::AlignVCenter,
                     fmGpu.elidedText(gpuTitle, Qt::ElideRight,
                                      ringCenterPointerX - gpuRect.x() - outsideRingRadius));

    painter.setFont(m_subContentFont);
    painter.setPen(QPen(summaryColor));
    painter.drawText(gpuStatRect, Qt::AlignLeft | Qt::AlignVCenter, gpuContent);

    // Draw memory summary if available
    int lastY = gpuStatRect.y() + gpuStatRect.height();
    if (totalMem > 0) {
        QRect memRect(gpuRect.x(), gpuStatRect.y() + gpuStatRect.height() + spacing,
                      fmGpu.size(Qt::TextSingleLine, memTitle).width(), fmGpu.height());
        QRect memStatRect(memRect.x(), memRect.y() + memRect.height(),
                          fmGpuStat.size(Qt::TextSingleLine, memContent).width(),
                          fmGpuStat.height());

        painter.setFont(m_contentFont);
        painter.setPen(QPen(numberColor));
        painter.drawText(memRect, memTitle);

        painter.setFont(m_subContentFont);
        painter.setPen(QPen(summaryColor));
        painter.drawText(memStatRect, Qt::AlignLeft | Qt::AlignVCenter, memContent);

        lastY = memStatRect.y() + memStatRect.height();
    }

    // Draw GPU utilization ring
    drawLoadingRing(painter, rect().x() + ringCenterPointerX, rect().y() + ringCenterPointerY,
                    outsideRingRadius, ringWidth, 270, 270, ringForegroundColor,
                    ringForegroundOpacity, ringBackgroundColor, ringBackgroundOpacity,
                    currentPercent);

    // Draw percent text in the ring
    painter.setFont(m_percentFont);
    painter.setPen(QPen(numberColor));
    painter.drawText(QRect(rect().x() + ringCenterPointerX - insideRingRadius,
                           rect().y() + ringCenterPointerY - insideRingRadius, insideRingRadius * 2,
                           insideRingRadius * 2),
                     Qt::AlignHCenter | Qt::AlignVCenter,
                     QString("%1%").arg(QString::number(currentPercent * 100, 'f', 1)));

    int fixedHeight = lastY;
    if (ringCenterPointerY + outsideRingRadius > lastY) {
        fixedHeight = ringCenterPointerY + outsideRingRadius;
    }
    const int alignedCpuHeight = 210;
    setFixedHeight(qMax(fixedHeight + 1, alignedCpuHeight));
}

void GpuMonitor::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        emit clicked("MSG_GPU");
    }
}

void GpuMonitor::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    return;
}
