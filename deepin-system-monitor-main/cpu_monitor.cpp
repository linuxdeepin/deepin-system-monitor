// Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cpu_monitor.h"

#include "smooth_curve_generator.h"
#include "gui/ui_common.h"
#include "settings.h"
#include "common/common.h"
#include "model/cpu_info_model.h"
#include "model/cpu_stat_model.h"
#include "gui/base/base_commandlink_button.h"

#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif
#include <DPalette>
#include <DStyle>
#include <DFontSizeManager>

#include <QDebug>
#include <QIcon>
#include <QPainter>
#include <QtMath>
#include <QPropertyAnimation>
#include <QPainterPath>
#include <QMouseEvent>

DWIDGET_USE_NAMESPACE
using namespace common;
using namespace DDLog;

CpuMonitor::CpuMonitor(QWidget *parent)
    : QWidget(parent)
{
    qCDebug(app) << "CpuMonitor constructor";
    int statusBarMaxWidth = common::getStatusBarMaxWidth();
    setFixedSize(statusBarMaxWidth, 240);
    waveformsRenderOffsetX = (statusBarMaxWidth - 140) / 2;

    cpuPercents = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        cpuPercents->append(0);
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &CpuMonitor::changeTheme);
#else
    DGuiApplicationHelper *dAppHelper = DGuiApplicationHelper::instance();
    connect(dAppHelper, &DGuiApplicationHelper::themeTypeChanged, this, &CpuMonitor::changeTheme);
#endif
    changeTheme(dAppHelper->themeType());

    m_cpuInfomodel = CPUInfoModel::instance();
    connect(m_cpuInfomodel, &CPUInfoModel::modelUpdated, this, &CpuMonitor::updateStatus);

    m_animation = new QPropertyAnimation(this, "progress", this);
    m_animation->setDuration(20);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_animation, &QVariantAnimation::valueChanged, [ = ]() {
        update();
    });

    m_detailButton = new BaseCommandLinkButton(tr("Details"), this);
    DFontSizeManager::instance()->bind(m_detailButton, DFontSizeManager::T8, QFont::Medium);
    connect(m_detailButton, &BaseCommandLinkButton::clicked, this, &CpuMonitor::onDetailInfoClicked);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CpuMonitor::changeFont);
}

CpuMonitor::~CpuMonitor()
{
    qCDebug(app) << "CpuMonitor destructor";
    delete cpuPercents;
}

void CpuMonitor::onDetailInfoClicked()
{
    qCDebug(app) << "onDetailInfoClicked";
    setDetailButtonVisible(false);
    emit signalDetailInfoClicked();
}

void CpuMonitor::setDetailButtonVisible(bool visible)
{
    qCDebug(app) << "setDetailButtonVisible:" << visible;
    m_detailButton->setVisible(visible);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void CpuMonitor::changeTheme(DApplicationHelper::ColorType themeType)
{
#else
void CpuMonitor::changeTheme(DGuiApplicationHelper::ColorType themeType)
{
#endif
    qCDebug(app) << "changeTheme with themeType:" << themeType;
    switch (themeType) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    case DApplicationHelper::LightType:
        qCDebug(app) << "LightType";
#else
    case DGuiApplicationHelper::LightType:
#endif
        ringBackgroundColor = "#000000";
        m_icon = QIcon(iconPathFromQrc("light/icon_cpu_light.svg"));
        break;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    case DApplicationHelper::DarkType:
        qCDebug(app) << "DarkType";
#else
    case DGuiApplicationHelper::DarkType:
#endif
        ringBackgroundColor = "#FFFFFF";
        m_icon = QIcon(iconPathFromQrc("dark/icon_cpu_light.svg"));
        break;
    default:
        qCDebug(app) << "default theme type";
        break;
    }

    // init colors
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif
    auto palette = dAppHelper->applicationPalette();
    textColor = palette.color(DPalette::Text);
#ifndef THEME_FALLBACK_COLOR
    numberColor = palette.color(DPalette::TextTitle);
#else
    numberColor = palette.color(DPalette::Text);
#endif

    update();
}

void CpuMonitor::updateStatus()
{
    qCDebug(app) << "updateStatus";
    // 如果数据无效不更新
    if (std::isnan(m_cpuInfomodel->cpuAllPercent())) {
        qCWarning(app) << "cpuAllPercent is nan";
        return;
    }
    cpuPercents->append(m_cpuInfomodel->cpuAllPercent());

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
            // qCDebug(app) << "cpuMaxHeight >= cpuRenderMaxHeight";
            points.append(
                QPointF(i * 5 - 8, cpuPercents->at(i) * cpuRenderMaxHeight / cpuMaxHeight));
        }
    }

    cpuPath = SmoothCurveGenerator::generateSmoothCurve(points);

    m_animation->start();
}

void CpuMonitor::changeFont(const QFont &font)
{
    qCDebug(app) << "changeFont";
    m_cpuUsageFont = font;
    m_cpuUsageFont.setBold(true);
    m_cpuUsageFont.setPointSizeF(m_cpuUsageFont.pointSizeF() + 3);

    m_cpuDisplayFont = font;
    m_cpuDisplayFont.setPointSizeF(m_cpuDisplayFont.pointSizeF() + 12);

    m_detailFont = font;
    m_detailFont.setWeight(QFont::Medium);
    m_detailFont.setPointSizeF(m_detailFont.pointSizeF() - 1);

    resizeItemWidgetRect();
}

void CpuMonitor::resizeEvent(QResizeEvent *event)
{
    // qCDebug(app) << "resizeEvent";
    QWidget::resizeEvent(event);
    resizeItemWidgetRect();
}

void CpuMonitor::resizeItemWidgetRect()
{
    // qCDebug(app) << "resizeItemWidgetRect";
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_detailButton->setFixedSize(m_detailButton->fontMetrics().width(m_detailButton->text()) + 16, m_detailButton->fontMetrics().height() + 4);
#else
    m_detailButton->setFixedSize(m_detailButton->fontMetrics().horizontalAdvance(m_detailButton->text()) + 16, m_detailButton->fontMetrics().height() + 4);
#endif
    const QSize &detailtextSize =  m_detailButton->size();
    m_detailButton->setGeometry(this->width() / 2 - detailtextSize.width() / 2 - 4, this->height() - detailtextSize.height() + 3, detailtextSize.width(), detailtextSize.height());
}

void CpuMonitor::paintEvent(QPaintEvent *)
{
    // qCDebug(app) << "paintEvent";
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int margin = 10;

    QFontMetrics fm(m_cpuDisplayFont);
    int titleWidth =
        fm.size(Qt::TextSingleLine, DApplication::translate("Process.Graph.View", "CPU")).width();

    int iconSize = 24;

    painter.setFont(m_cpuDisplayFont);
    painter.setPen(QPen(textColor));
    QRect cpuDisplayRect(((rect().x() + rect().width() - titleWidth) / 2),
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
        QRect(rect().x(), rect().y() + percentRenderOffsetY, rect().width(), 30),
        Qt::AlignCenter, QString("%1%").arg(QString::number(percent, 'f', 1)));

    int centerX = rect().x() + rect().width() / 2;
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

    setFixedHeight(cpuDisplayRect.y() + cpuDisplayRect.height() + 1 + m_detailButton->fontMetrics().height());
}

void CpuMonitor::mouseReleaseEvent(QMouseEvent *ev)
{
    // qCDebug(app) << "mouseReleaseEvent";
    if (ev->button() == Qt::LeftButton) {
        // qCDebug(app) << "mouseReleaseEvent left button";
        emit clicked("MSG_CPU");
    }
}

void CpuMonitor::mouseMoveEvent(QMouseEvent *event)
{
    // qCDebug(app) << "mouseMoveEvent";
    Q_UNUSED(event);
    return;
}
