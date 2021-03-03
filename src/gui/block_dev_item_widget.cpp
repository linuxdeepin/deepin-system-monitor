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
#include <QtMath>

#include <DApplication>
#include <DApplicationHelper>

DWIDGET_USE_NAMESPACE
using namespace common::format;

const int margin = 6;
const int spacing = 6;
const int sectionSize = 6;

BlockDevItemWidget::BlockDevItemWidget(QWidget *parent) : QWidget(parent)
{
    m_memChartWidget = new ChartViewWidget(this);
    m_memChartWidget->setSpeedAxis(true);
    m_memChartWidget->setData1Color(readColor);
    m_memChartWidget->setData2Color(writeColor);
}

void BlockDevItemWidget::updateWidgetGeometry()
{
    int fontHeight = QFontMetrics(m_font).height();
    if (m_mode == TITLE_HORIZONTAL) {
        m_memChartWidget->setGeometry(margin, fontHeight / 2, this->width() - 2 * margin, this->height() - fontHeight / 2 - margin);
    } else {
        m_memChartWidget->setGeometry(margin, fontHeight * 2 + fontHeight / 2, this->width() - 2 * margin, this->height() - fontHeight * 2 - fontHeight / 2 - margin);
    }
}

void BlockDevItemWidget::fontChanged(const QFont &font)
{
    m_font = font;
    updateWidgetGeometry();
}

void BlockDevItemWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateWidgetGeometry();
}

void BlockDevItemWidget::updateData(const BlockDevice &info)
{
    m_blokeDeviceInfo = info;
    m_memChartWidget->addData1(info.readSpeed());
    m_memChartWidget->addData2(info.writeSpeed());
    m_memChartWidget->update();

    this->update();
}

void BlockDevItemWidget::activeItemWidget(bool isShow)
{
    m_isActive = isShow;
    update();
}

void BlockDevItemWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    emit clicked(m_blokeDeviceInfo.deviceName());
}

void BlockDevItemWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setFont(m_font);
    painter.setRenderHint(QPainter:: Antialiasing, true);

    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();

    QString deviceName = m_blokeDeviceInfo.deviceName().data();
    QRect devtitleRect(margin, margin, painter.fontMetrics().width(deviceName), painter.fontMetrics().height());

    const QColor &textColor = palette.color(DPalette::TextTips);
    if (m_isActive && m_mode == TITLE_VERTICAL) {
        painter.setPen(Qt::NoPen);
        QColor selectColor = palette.color(DPalette::Highlight);
        selectColor.setAlphaF(0.1);

        painter.setBrush(selectColor);
        painter.drawRoundedRect(rect(), 8, 8);

        painter.setPen(palette.color(DPalette::Highlight));
    } else {
        painter.setPen(textColor);
    }
    painter.drawText(devtitleRect, deviceName);
    painter.setPen(textColor);

    QString readTitle = QString("%1 %2")
                        .arg(tr("Read"))
                        .arg(formatUnit(m_blokeDeviceInfo.readSpeed(), B, 1, true));

    QString writeTitle = QString("%1 %2")
                         .arg(tr("Write"))
                         .arg(formatUnit(m_blokeDeviceInfo.writeSpeed(), B, 1, true));

    if (m_mode == TITLE_HORIZONTAL) {
        QRect memtitleRect(sectionSize + devtitleRect.right() + spacing * 2, devtitleRect.y(), painter.fontMetrics().width(readTitle), painter.fontMetrics().height());
        painter.drawText(memtitleRect, Qt::AlignLeft | Qt::AlignVCenter, readTitle);

        QRect swaptitleRect(sectionSize + memtitleRect.right() + 2 * spacing, memtitleRect.y(), painter.fontMetrics().width(writeTitle), painter.fontMetrics().height());
        painter.drawText(swaptitleRect, Qt::AlignLeft | Qt::AlignVCenter, writeTitle);

        painter.setPen(Qt::NoPen);
        painter.setBrush(readColor);
        painter.drawEllipse(devtitleRect.right() + spacing, memtitleRect.y() + qCeil((memtitleRect.height() - sectionSize) / 2.0), sectionSize, sectionSize);

        painter.setBrush(writeColor);
        painter.drawEllipse(memtitleRect.right() + spacing, swaptitleRect.y() + qCeil((swaptitleRect.height() - sectionSize) / 2.0), sectionSize, sectionSize);
    } else {
        painter.setPen(Qt::NoPen);
        painter.setBrush(readColor);

        QRect readStrRect(devtitleRect.left() + spacing + sectionSize, devtitleRect.bottom(), painter.fontMetrics().width(readTitle), painter.fontMetrics().height());
        painter.drawEllipse(devtitleRect.left(), readStrRect.y() + qCeil((readStrRect.height() - sectionSize) / 2.0), sectionSize, sectionSize); // 读硬盘速度的颜色提示
        painter.setPen(palette.color(DPalette::TextTips));
        painter.drawText(readStrRect, readTitle);

        painter.setPen(Qt::NoPen);
        painter.setBrush(writeColor);

        QRect writeStrRect(devtitleRect.left() + spacing + sectionSize, readStrRect.bottom(), painter.fontMetrics().width(writeTitle), painter.fontMetrics().height());
        painter.drawEllipse(devtitleRect.left(), writeStrRect.y() + qCeil((writeStrRect.height() - sectionSize) / 2.0), sectionSize, sectionSize); // 写硬盘速度的颜色提示
        painter.setPen(palette.color(DPalette::TextTips));
        painter.drawText(writeStrRect, writeTitle);
    }
}

void BlockDevItemWidget::setMode(int mode)
{
    m_mode = mode;
    this->update();
}



