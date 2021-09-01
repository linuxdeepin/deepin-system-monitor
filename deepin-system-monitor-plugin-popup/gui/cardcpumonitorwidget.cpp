/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan  <yukuan@uniontech.com>
*
* Maintainer: yukuan  <yukuan@uniontech.com>
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
#include "cardcpumonitorwidget.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyleHelper>

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QBrush>
#include <QPaintEvent>

DWIDGET_USE_NAMESPACE

CardCPUMonitorWidget::CardCPUMonitorWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &CardCPUMonitorWidget::changeTheme);
    changeTheme(dAppHelper->themeType());

//    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
//    QStyleOption option;
//    option.initFrom(this);
//    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    setFixedSize(280, 111);

//    pointsNumber = int(statusBarMaxWidth / 10);

//    numCPU = int(sysconf(_SC_NPROCESSORS_ONLN));

//    for (int i = 0; i < numCPU; i++) {
//        QList<double> cpuPercent;
//        for (int j = 0; j < pointsNumber; j++) {
//            cpuPercent.append(0);
//        }

//        cpuPercents.append(cpuPercent);
//    }

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

//    auto *smo = SystemMonitor::instance();
//    Q_ASSERT(smo != nullptr);
//    connect(smo->jobInstance(), &StatsCollector::cpuStatInfoUpdated,
//            this, &CompactCpuMonitor::updateStatus);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CardCPUMonitorWidget::changeFont);
}

CardCPUMonitorWidget::~CardCPUMonitorWidget() {}

void CardCPUMonitorWidget::updateStatus(qreal cpuPercent, const QList<qreal> cPercents)
{
    totalCpuPercent = cpuPercent;

    for (int i = 0; i < cPercents.size(); i++) {
        QList<qreal> cpuPercent = cpuPercents[i];

        cpuPercent.append(cPercents[i]);

        if (cpuPercent.size() > pointsNumber) {
            cpuPercent.pop_front();
        }

        cpuPercents[i] = cpuPercent;
    }

    update();
}

void CardCPUMonitorWidget::changeTheme(DApplicationHelper::ColorType themeType)
{
    switch (themeType) {
    case DApplicationHelper::LightType:
        m_icon = QIcon(QString(":/icons/icon_network_light.png"));
        break;
    case DApplicationHelper::DarkType:
//        m_icon = QIcon(iconPathFromQrc("dark/icon_network_light.svg"));
        break;
    default:
        break;
    }

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
#ifndef THEME_FALLBACK_COLOR
    ltextColor = palette.color(DPalette::TextTitle);
#else
    ltextColor = palette.color(DPalette::Text);
#endif

    textColor = palette.color(DPalette::Text);
    textColor = palette.color(DPalette::Text);
    summaryColor = palette.color(DPalette::TextTips);

}

void CardCPUMonitorWidget::paintEvent(QPaintEvent *e)
{
    setFixedWidth(280);
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QFontMetrics fmContent(m_TitleFont);
    QFontMetrics fmSubContent(m_cpuFont);
    QFontMetrics fmTextContent(m_textFont);

    //裁剪绘制区域
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    painter.setClipPath(path);
    painter.fillRect(rect(), QBrush(QColor(255, 255, 255, 0)));

    //背景
    QRect titleRect(rect().x(), rect().y(), 280, 36);
    painter.fillRect(titleRect, QBrush(QColor(255, 255, 255,25)));
    QRect contentRect(rect().x(), rect().y()+36, 280, 75);
    painter.fillRect(contentRect, QBrush(QColor(255, 255, 255,50)));

    //图标
    m_icon = QIcon(QString(":/icons/icon_cpu_light.png"));
    int iconSize = 20;
    QRect iconRect(titleRect.x()+97, titleRect.y() + qCeil((titleRect.height() - iconSize) / 2.) + 2,iconSize, iconSize);
    m_icon.paint(&painter, iconRect);

    //标题
    painter.setFont(m_TitleFont);
    QFontMetrics fmTitle = painter.fontMetrics();
    int widthTitle = fmTitle.width("处理器");
    int heightTitle = fmTitle.descent()+fmTitle.ascent();
    QRect cpuTitleRect(titleRect.x(), titleRect.y(), widthTitle, heightTitle);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter,
                     fmContent.elidedText("处理器", Qt::ElideRight, cpuTitleRect.width()));

    //cpu使用率
    painter.setPen(ltextColor);
    painter.setFont(m_cpuFont);
    QFontMetrics fmCpu = painter.fontMetrics();
    int widthCpu = fmCpu.width("3.4");
    int heightCpu = fmCpu.descent()+fmCpu.ascent();
    QRect cpuUsageRect(contentRect.x()+27, contentRect.y()+13, widthCpu, heightCpu);
    painter.drawText(cpuUsageRect, Qt::AlignLeft | Qt::AlignTop, "3.4");
    //cpu使用率百分号
    painter.setFont(m_TitleFont);
    QFontMetrics fmCpuper = painter.fontMetrics();
    int widthCpuper = fmCpuper.width("%");
    int heightCpuper = fmCpuper.descent()+fmCpuper.ascent();
    QRect cpuperUsageRect(contentRect.x()+27+widthCpu, contentRect.y()+13+ heightCpu/4, widthCpuper, heightCpuper);
    painter.drawText(cpuperUsageRect, Qt::AlignLeft | Qt::AlignTop, "%");

    //总使用率 文本
    painter.setPen(summaryColor);
    painter.setFont(m_textFont);
    QFontMetrics fmText = painter.fontMetrics();
    int widthText = fmText.width("总使用率");
    int heightText = fmText.descent()+fmCpu.ascent();
    QRect cpuTextRect(contentRect.x()+27, contentRect.y()+13 + heightCpuper+8, widthText, heightText);
    painter.drawText(cpuTextRect, Qt::AlignLeft | Qt::AlignTop,"总使用率");


    //分隔符
    painter.setFont(m_cpuFont);
    QRect separatorRect1(contentRect.x()+97, contentRect.y()+10, 1, 50);
    painter.fillRect(separatorRect1, QBrush(QColor(0, 0, 0,20)));
    QRect separatorRect2(contentRect.x()+97*2, contentRect.y()+10, 1, 50);
    painter.fillRect(separatorRect2, QBrush(QColor(0, 0, 0,20)));
}

void CardCPUMonitorWidget::changeFont(const QFont &font)
{
    m_TitleFont = font;
    m_TitleFont.setFamily("SourceHanSansSC");
    m_TitleFont.setWeight(QFont::DemiBold);
    m_TitleFont.setPointSizeF(m_TitleFont.pointSizeF());

    m_cpuFont = font;
    m_cpuFont.setFamily("HelveticaNeueLT");
    m_cpuFont.setWeight(QFont::Normal);
    m_cpuFont.setPointSizeF(m_cpuFont.pointSizeF()+8);

    m_textFont = font;
    m_textFont.setFamily("SourceHanSansSC");
    m_textFont.setWeight(QFont::ExtraLight);
    m_textFont.setPointSizeF(m_textFont.pointSizeF() - 2);
}
