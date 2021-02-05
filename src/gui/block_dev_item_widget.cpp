/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     tenghuan <tenghuan@uniontech.com>
*
* Maintainer: tenghuan <tenghuan@uniontech.com>
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
#include "block_dev_item_widget.h"
#include "chart_view_widget.h"
#include "common/common.h"
#include <QPen>
#include <QPainter>
#include <QPalette>
#include <DApplication>
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE
using namespace common::format;

BlockDevItemWidget::BlockDevItemWidget(BlockDevice info,QWidget *parent) : QWidget(parent)
{
    m_blokeDeviceInfo = info;
    m_memChartWidget = new ChartViewWidget(this);
    m_memChartWidget->setSpeedAxis(true);
    m_memChartWidget->setData1Color(readColor);
    m_memChartWidget->setData2Color(writeColor);
    updateData(info);
}

void BlockDevItemWidget::updateWidgetGeometry()
{
    int avgWidth = this->width();
    int avHeight = this->height();
    int fontHeight = QFontMetrics(m_font).height();
    if(m_mode != 0) {
        m_memChartWidget->setGeometry(5, (fontHeight*3+35)/ 2, avgWidth-10, avHeight - fontHeight * 2-5);
    } else {
        m_memChartWidget->setGeometry(5, (fontHeight)/ 2, avgWidth-10, avHeight - fontHeight * 2+28);
    }
}

void BlockDevItemWidget::fontChanged(const QFont &font)
{
    m_font = font;
    updateWidgetGeometry();
    //setFixedHeight(100 + 2 * QFontMetrics(font).height());
}

void BlockDevItemWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateWidgetGeometry();
}

void BlockDevItemWidget::updateData(BlockDevice info)
{
    m_blokeDeviceInfo = info;
    m_memChartWidget->addData1(info.readSpeed());

    m_memChartWidget->addData2(info.writeSpeed());
    m_memChartWidget->update();
}

void BlockDevItemWidget::showBackGround(bool isShow)
{
    haveBackGround = isShow;
    update();
}

void BlockDevItemWidget::mousePressEvent(QMouseEvent *event)
{
    if(m_mode != 0) {
        Q_UNUSED(event)
        haveBackGround = true;
    }
    emit clicked(m_blokeDeviceInfo.deviceName());
    update();
}

void BlockDevItemWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setFont(m_font);
    painter.setRenderHint(QPainter:: Antialiasing, true);  //设置渲染,启动反锯齿
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    painter.setPen(palette.color(DPalette::TextTips));
    int spacing = 5;
    int sectionSize = 6;
    QString deviceName = m_blokeDeviceInfo.deviceName().data();
    QRect devtitleRect(5, 0, painter.fontMetrics().width(deviceName), painter.fontMetrics().height());
    if(haveBackGround) {
        painter.save();
        painter.setPen(palette.color(DPalette::Highlight));
        painter.drawText(devtitleRect,deviceName);
        painter.restore();
    } else {
        painter.drawText(devtitleRect,deviceName);
    }


    QString readTitle = QString("%1 %2")
                        .arg(DApplication::translate("Process.Graph.View", "Disk read"))
                        .arg(formatUnit(m_blokeDeviceInfo.readSpeed(), B, 1, true));
    QString writeTitle = QString("%1 %2")
                         .arg(DApplication::translate("Process.Graph.View", "Disk write"))
                         .arg(formatUnit(m_blokeDeviceInfo.writeSpeed(), B, 1, true));

    if(m_mode == 0){

       QRect memtitleRect(sectionSize + devtitleRect.topRight().x()+spacing*2,devtitleRect.y(), painter.fontMetrics().width(readTitle), painter.fontMetrics().height());
       painter.drawText(memtitleRect, Qt::AlignLeft | Qt::AlignVCenter, readTitle);

       QRect swaptitleRect(sectionSize + memtitleRect.right() + 2 * spacing, memtitleRect.y(), painter.fontMetrics().width(writeTitle), painter.fontMetrics().height());
       painter.drawText(swaptitleRect, Qt::AlignLeft | Qt::AlignVCenter, writeTitle);


       painter.setPen(Qt::NoPen);
       painter.setBrush(readColor);
       painter.drawEllipse(devtitleRect.topRight().x()+spacing, memtitleRect.center().y(), sectionSize, sectionSize);

       painter.setBrush(writeColor);
       painter.drawEllipse(memtitleRect.right() + spacing, swaptitleRect.center().y(), sectionSize, sectionSize);
   } else {
        painter.setPen(Qt::NoPen);
        painter.setBrush(readColor);
        painter.drawEllipse(devtitleRect.bottomLeft().x(),devtitleRect.bottomLeft().y()+spacing+4, sectionSize, sectionSize); // 读硬盘速度的颜色提示
        QRect readStrRect (devtitleRect.bottomLeft().x()+8+spacing,devtitleRect.bottomLeft().y()+spacing-4,painter.fontMetrics().width(readTitle)+10, painter.fontMetrics().height());
        painter.setPen(palette.color(DPalette::TextTips));
        painter.drawText(readStrRect,readTitle);

        painter.setPen(Qt::NoPen);
        painter.setBrush(writeColor);
        painter.drawEllipse(devtitleRect.bottomLeft().x(),readStrRect.bottomLeft().y()+spacing+4, sectionSize, sectionSize); // 写硬盘速度的颜色提示
        QRect writeStrRect (devtitleRect.bottomLeft().x()+8+spacing,readStrRect.bottomLeft().y()+spacing-4,painter.fontMetrics().width(writeTitle)+10, painter.fontMetrics().height());
        painter.setPen(palette.color(DPalette::TextTips));
        painter.drawText(writeStrRect,writeTitle);
    }
    if(haveBackGround) {
        painter.save();
        painter.setPen(Qt::NoPen);
        QColor color = palette.color(DPalette::Highlight);
        color.setAlpha(26);
        painter.setBrush(color);
        painter.drawRoundRect(rect(),8,8);
        painter.restore();

    }
}

void BlockDevItemWidget::setMode(int mode)
{
    m_mode = mode;
    if(mode == 0) {
        haveBackGround = false;
    }
}



