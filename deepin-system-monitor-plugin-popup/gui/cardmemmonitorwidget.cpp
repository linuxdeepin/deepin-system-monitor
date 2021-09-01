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
#include "cardmemmonitorwidget.h"

#include "common/utils.h"

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
#include <QFontMetrics>

DWIDGET_USE_NAMESPACE

using namespace Utils;

CardMemMonitorWidget::CardMemMonitorWidget(QWidget *parent)
    : QWidget(parent)
{
     auto *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &CardMemMonitorWidget::changeTheme);
    changeTheme(dAppHelper->themeType());

//    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
//    QStyleOption option;
//    option.initFrom(this);
//    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    setFixedSize(280, 153);

//    pointsNumber = int(statusBarMaxWidth / 10);

//    numCPU = int(sysconf(_SC_NPROCESSORS_ONLN));

//    for (int i = 0; i < numCPU; i++) {
//        QList<double> cpuPercent;
//        for (int j = 0; j < pointsNumber; j++) {
//            cpuPercent.append(0);
//        }

//        cpuPercents.append(cpuPercent);
//    }

//    cpuColors << "#1094D8"
//              << "#F7B300"
//              << "#55D500"
//              << "#C362FF"
//              << "#FF2997"
//              << "#00B4C7"
//              << "#F8E71C"
//              << "#FB1818"
//              << "#8544FF"
//              << "#00D7AB"
//              << "#00D7AB"
//              << "#FF00FF"
//              << "#30BF03"
//              << "#7E41F1"
//              << "#2CA7F8"
//              << "#A005CE";

//    auto *smo = SystemMonitor::instance();
//    Q_ASSERT(smo != nullptr);
//    connect(smo->jobInstance(), &StatsCollector::cpuStatInfoUpdated,
//            this, &CompactCpuMonitor::updateStatus);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CardMemMonitorWidget::changeFont);

}
CardMemMonitorWidget::~CardMemMonitorWidget() {}


void CardMemMonitorWidget::updateStatus(qreal cpuPercent, const QList<qreal> cPercents)
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

void CardMemMonitorWidget::changeTheme(DApplicationHelper::ColorType themeType)
{
    switch (themeType) {
    case DApplicationHelper::LightType:
        m_icon = QIcon(QString(":/icons/icon_memory_light.png"));
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

void CardMemMonitorWidget::paintEvent(QPaintEvent *e)
{
    setFixedWidth(280);
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //裁剪绘制区域
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    painter.setClipPath(path);
    painter.fillRect(rect(), QBrush(QColor(255, 255, 255, 0)));

    //背景
    QRect titleRect(rect().x(), rect().y(), 280, 36);
    painter.fillRect(titleRect, QBrush(QColor(255, 255, 255,25)));
    QRect contentRect(rect().x(), rect().y()+36, 280, 117);
    painter.fillRect(contentRect, QBrush(QColor(255, 255, 255,20)));

    //标题
    painter.setFont(m_sectionFont);
    QFontMetrics fmTitle = painter.fontMetrics();
    int widthTitleTxt = fmTitle.width("内存");
    int heightTitleTxt = fmTitle.descent()+fmTitle.ascent();
    QRect netTitleRect(titleRect.x(), titleRect.y(), widthTitleTxt, heightTitleTxt);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter,"内存");

    //图标
    int iconSize = 20;
    QRect iconRect(titleRect.x()+titleRect.width()/2-netTitleRect.width() -5, titleRect.y() + qCeil((titleRect.height() - iconSize) / 2.) + 2,iconSize, iconSize);
    m_icon.paint(&painter, iconRect);

    int bulletSize = 6;
    int padleft = 36;
    auto spacing = 20;
    auto margin = 10;

    //test
    m_recvBps = 2.96;

    // Draw network summary.
    QString recvTitle = DApplication::translate("Process.Graph.View", "内存(70%)");//Download
    QString recvContent = formatUnit(m_recvBps, B, 1, true);
    QString sentTitle = DApplication::translate("Process.Graph.View", "交换空间1%");//Upload
    QString sentContent = formatUnit(m_sentBps, B, 1, true);


    QFontMetrics fmContent(m_contentFont);
    QFontMetrics fmSubContent(m_subContentFont);
    QRect contentNetRect(padleft, netTitleRect.y() + netTitleRect.height(),
                      rect().x() + rect().width() - padleft, 1);

    //1.2 M/s "内存"的值
    QRect recvContentRect(contentNetRect.x(), contentNetRect.y()+margin*2, qCeil((contentNetRect.width() - spacing) / 2),
                  fmContent.height());
    //内存
    QRect recvTitleRect(contentNetRect.x(), recvContentRect.y() + recvContentRect.height(), recvContentRect.width(),
                  fmSubContent.height());
    //"交换空间"的值
    QRect crect31(contentNetRect.x(), recvTitleRect.y() + recvTitleRect.height() + 5, recvContentRect.width(),
                  recvContentRect.height());
    //交换空间
    QRect crect41(contentNetRect.x(), crect31.y() + crect31.height(), recvContentRect.width(),
                  recvTitleRect.height());
    //红点
    QRectF r1Ind(contentNetRect.x() - margin, recvTitleRect.y() + qCeil((recvTitleRect.height() - bulletSize) / 2.), bulletSize,
                 bulletSize);
    QRectF r2Ind(contentNetRect.x() - margin, crect41.y() + qCeil((crect41.height() - bulletSize) / 2.), bulletSize,
                 bulletSize);

    painter.setPen(ltextColor);
    painter.setFont(m_contentFont);
    painter.drawText(recvContentRect, Qt::AlignLeft | Qt::AlignVCenter,
                     fmContent.elidedText(recvContent, Qt::ElideRight, recvContentRect.width()));
    painter.drawText(crect31, Qt::AlignLeft | Qt::AlignVCenter,
                     fmContent.elidedText(sentContent, Qt::ElideRight, crect31.width()));

    painter.setPen(summaryColor);
    painter.setFont(m_subContentFont);
    painter.drawText(recvTitleRect, Qt::AlignLeft | Qt::AlignVCenter,
                     fmSubContent.elidedText(recvTitle, Qt::ElideRight, recvTitleRect.width()));
    painter.drawText(crect41, Qt::AlignLeft | Qt::AlignVCenter,
                     fmSubContent.elidedText(sentTitle, Qt::ElideRight, crect41.width()));

    QPainterPath path1, path2;
    path1.addEllipse(r1Ind);
    path2.addEllipse(r2Ind);

    QColor m_recvIndicatorColor {"#00C5C0"};
    QColor m_sentIndicatorColor {"#FEDF19"};
    painter.fillPath(path1, m_recvIndicatorColor);
    painter.fillPath(path2, m_sentIndicatorColor);
}

void CardMemMonitorWidget::changeFont(const QFont &font)
{
    m_sectionFont = font;
    m_sectionFont.setFamily("SourceHanSansSC");
    m_sectionFont.setWeight(QFont::DemiBold);
    m_sectionFont.setPointSizeF(m_sectionFont.pointSizeF());

    m_contentFont = font;
    m_contentFont.setFamily("HelveticaNeueLT");
    m_contentFont.setWeight(QFont::Normal);
    m_contentFont.setPointSizeF(m_contentFont.pointSizeF()+5);

    m_subContentFont = font;
    m_subContentFont.setFamily("SourceHanSansSC");
    m_subContentFont.setWeight(QFont::ExtraLight);
    m_subContentFont.setPointSizeF(m_subContentFont.pointSizeF()-2 );
}
