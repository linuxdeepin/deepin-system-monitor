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
#include <DApplication>
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE
using namespace common::format;

BlockDevItemWidget::BlockDevItemWidget(QWidget *parent) : QWidget(parent)
{
   // m_blokeDeviceInfo = info;
    m_memChartWidget = new ChartViewWidget(this);
    this->setMinimumSize(QSize(342,190));
    updateWidgetGeometry();

    m_memChartWidget->setData1Color(readColor);
    m_memChartWidget->setData2Color(writeColor);
   // updateData(info);


}

void BlockDevItemWidget::updateWidgetGeometry()
{
    int avgWidth = this->width();

    int fontHeight = QFontMetrics(m_font).height();

    m_memChartWidget->setGeometry(0, (fontHeight*3+35)/ 2, avgWidth, 100+2 * fontHeight - fontHeight / 2);
}

void BlockDevItemWidget::fontChanged(const QFont &font)
{
    m_font = font;
    setFixedHeight(100 + 2 * QFontMetrics(font).height());
}

void BlockDevItemWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateWidgetGeometry();
}

void BlockDevItemWidget::updateData(BlockDevice info)
{
    m_blokeDeviceInfo = info;
    m_listReadSpeed<< info.readSpeed()/2;
    m_listWriteSpeed << info.writeSpeed()/2;
    if (m_listReadSpeed.size() > 60) {
        m_listReadSpeed.pop_front();
    }

    if (m_listWriteSpeed.size() > 60) {
        m_listWriteSpeed.pop_front();
    }
    qreal readMaxHeight = 0;
    qreal writeMaxHeight = 0;
    for(int i = 0; i < m_listReadSpeed.size(); ++i) {
        if(m_listReadSpeed[i] > readMaxHeight) {
            readMaxHeight = m_listReadSpeed[i];
        }
    }

    for(int i = 0; i < m_listWriteSpeed.size(); ++i) {
        if(m_listWriteSpeed[i] > writeMaxHeight) {
            writeMaxHeight = m_listWriteSpeed[i];
        }
    }
    QList<qreal> readList {0};
    QList<qreal> wirteList {0};
//    int height = m_memChartWidget->getGridHeight();
//    qreal maxH = std::max(readMaxHeight, writeMaxHeight);
//    for(int i = 0; i < m_listReadSpeed.size(); ++i) {
//        qreal readval =  maxH == 0. ? 0 :m_listReadSpeed.at(i) * ulong(height-10) /maxH;
//        qreal writeval = maxH == 0. ? 0 :m_listWriteSpeed.at(i) * ulong(height-10) /maxH;
//        readList << ((readval/100 >=0.9) ? 0.9 : readval/100);
//        wirteList << ((writeval/100 >=0.9) ? 0.9 : writeval/100);
//    }
//    m_memChartWidget->addAllData1(readList);
//    m_memChartWidget->addAllData2(wirteList);
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
    QRect devtitleRect(0, 0, painter.fontMetrics().width(deviceName), painter.fontMetrics().height());
    painter.drawText(devtitleRect,deviceName);

    QString readTitle = QString("%1 %2")
                        .arg(DApplication::translate("Process.Graph.View", "Disk read"))
                        .arg(formatUnit(m_blokeDeviceInfo.readSpeed(), B, 1, true));
    QString writeTitle = QString("%1 %2")
                         .arg(DApplication::translate("Process.Graph.View", "Disk write"))
                         .arg(formatUnit(m_blokeDeviceInfo.writeSpeed(), B, 1, true));

    painter.setPen(Qt::NoPen);
    painter.setBrush(readColor);
    painter.drawEllipse(devtitleRect.bottomLeft().x(),devtitleRect.bottomLeft().y()+spacing+3, sectionSize, sectionSize); // 读硬盘速度的颜色提示
    QRect readStrRect (devtitleRect.bottomLeft().x()+3+spacing,devtitleRect.bottomLeft().y()+spacing-3,painter.fontMetrics().width(readTitle)+10, painter.fontMetrics().height());
    painter.setPen(palette.color(DPalette::TextTips));
    painter.drawText(readStrRect,readTitle);

    painter.setPen(Qt::NoPen);
    painter.setBrush(writeColor);
    painter.drawEllipse(devtitleRect.bottomLeft().x(),readStrRect.bottomLeft().y()+spacing+3, sectionSize, sectionSize); // 写硬盘速度的颜色提示
    QRect writeStrRect (devtitleRect.bottomLeft().x()+3+spacing,readStrRect.bottomLeft().y()+spacing-3,painter.fontMetrics().width(writeTitle)+10, painter.fontMetrics().height());
    painter.setPen(palette.color(DPalette::TextTips));
    painter.drawText(writeStrRect,writeTitle);

}


