/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     leiyu <leiyu@uniontech.com>
*
* Maintainer: leiyu <leiyu@uniontech.com>
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
#include "mem_stat_view_widget.h"
#include "chart_view_widget.h"
#include "model/mem_info_model.h"
#include "common/common.h"

#include <QPainter>

#include <DApplication>
#include <DApplicationHelper>

using namespace common::format;

MemStatViewWidget::MemStatViewWidget(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_memChartWidget = new ChartViewWidget(this);
    m_swapChartWidget = new ChartViewWidget(this);

    m_memChartWidget->setDataColor(memoryColor);
    m_swapChartWidget->setDataColor(swapColor);

    TimePeriod period(TimePeriod::k1Min, {2, 0});
    m_model = new MemInfoModel(period, this);

    onModelUpdate();
    connect(m_model, &MemInfoModel::modelUpdated, this, &MemStatViewWidget::onModelUpdate);
}

void MemStatViewWidget::fontChanged(const QFont &font)
{
    m_font = font;
    setFixedHeight(130 + 2 * QFontMetrics(font).height());
}

void MemStatViewWidget::onModelUpdate()
{
    QString memoryDetail = QString("%1(%2)")
                           .arg(tr("Memory Size"))
                           .arg(formatUnit(m_model->memTotal() << 10, B, 1));
    parent()->setProperty("detail", memoryDetail);

    m_memChartWidget->addData((m_model->memTotal() - m_model->memAvailable()) * 1.0 / m_model->memTotal());
    m_swapChartWidget->addData((m_model->swapTotal() - m_model->swapFree()) * 1.0 / m_model->swapTotal());
    updateWidgetGeometry();
}

void MemStatViewWidget::updateWidgetGeometry()
{
    int avgWidth = this->width();
    if (m_model->swapTotal() > 0) {
        avgWidth = this->width() / 2 - 10;
        m_swapChartWidget->setVisible(true);
    } else {
        m_swapChartWidget->setVisible(false);
    }

    int fontHeight = QFontMetrics(m_font).height();
    m_memChartWidget->setGeometry(0, fontHeight / 2, avgWidth, 130 + 2 * fontHeight - fontHeight / 2);
    m_swapChartWidget->setGeometry(m_memChartWidget->width() + 20, m_memChartWidget->y(), avgWidth, 130 + 2 * fontHeight - fontHeight / 2);
}

void MemStatViewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateWidgetGeometry();
}

void MemStatViewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setFont(m_font);

    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    painter.setPen(palette.color(DPalette::TextTips));

    int spacing = 10;
    int sectionSize = 6;

    QString memory = DApplication::translate("Process.Graph.Title", "Memory");
    QRect memtitleRect(sectionSize + spacing, 0, painter.fontMetrics().width(memory), painter.fontMetrics().height());
    painter.drawText(memtitleRect, Qt::AlignLeft | Qt::AlignVCenter, memory);

    QString swap = DApplication::translate("Process.Graph.View", "Swap");
    QRect swaptitleRect(sectionSize + memtitleRect.right() + 2 * spacing, memtitleRect.y(), painter.fontMetrics().width(swap), painter.fontMetrics().height());
    painter.drawText(swaptitleRect, Qt::AlignLeft | Qt::AlignVCenter, swap);

    painter.setPen(Qt::NoPen);
    painter.setBrush(memoryColor);
    painter.drawEllipse(0, memtitleRect.center().y(), sectionSize, sectionSize);

    painter.setBrush(swapColor);
    painter.drawEllipse(memtitleRect.right() + spacing, swaptitleRect.center().y(), sectionSize, sectionSize);
}
