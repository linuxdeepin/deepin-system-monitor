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
#include "netif_item_view_widget.h"
#include "chart_view_widget.h"
#include "common/common.h"
#include "system/netif.h"

#include <QtMath>
#include <QPainter>

#include <DApplication>
#include <DApplicationHelper>

using namespace common::format;
using namespace core::system;

const int margin = 6;
const int spacing = 6;
const int sectionSize = 6;

DWIDGET_USE_NAMESPACE
NetifItemViewWidget::NetifItemViewWidget(QWidget *parent, const QByteArray &mac) : QWidget(parent)
{
    m_mac = mac;
    m_ifname = "Ethernet";

    m_ChartWidget = new ChartViewWidget(this);
    m_ChartWidget->setData1Color(m_recvColor);
    m_ChartWidget->setData2Color(m_sentColor);
    m_ChartWidget->setSpeedAxis(true);
    updateWidgetGeometry();
}

void NetifItemViewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setFont(m_font);

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    const DPalette &palette = dAppHelper->applicationPalette();

    const QColor &textColor = palette.color(DPalette::TextTips);
    if (m_isActive) {
        painter.setPen(Qt::NoPen);
        QColor selectColor = palette.color(DPalette::Highlight);
        selectColor.setAlphaF(0.1);

        painter.setBrush(selectColor);
        painter.drawRoundedRect(rect(), 8, 8);

        painter.setPen(palette.color(DPalette::Highlight));
    } else {
        painter.setPen(textColor);
    }


    QRect ifnameRect(margin, margin / 2, painter.fontMetrics().width(m_ifname), painter.fontMetrics().height());
    painter.drawText(ifnameRect, Qt::AlignLeft | Qt::AlignVCenter, m_ifname);

    painter.setPen(textColor);
    if (m_mode == TITLE_HORIZONTAL) {
        QString strLeftBrackets = "(";
        QRect leftBracketsRect(ifnameRect.right() + spacing, ifnameRect.y(), painter.fontMetrics().width(strLeftBrackets), painter.fontMetrics().height());
        painter.drawText(leftBracketsRect, Qt::AlignLeft | Qt::AlignVCenter, strLeftBrackets);

        QString strRecvData = DApplication::translate("Process.Graph.Title", "Receive");
        strRecvData = strRecvData + " " + m_recv_bps;
        QRect recvRect(leftBracketsRect.right() + spacing * 2 + sectionSize, ifnameRect.y(), painter.fontMetrics().width(strRecvData), painter.fontMetrics().height());
        painter.drawText(recvRect, Qt::AlignLeft | Qt::AlignVCenter, strRecvData);

        QString strSentData = DApplication::translate("Process.Graph.View", "Send");
        strSentData = strSentData + " " + m_sent_bps + " )";
        QRect sentRect(sectionSize + recvRect.right() + 3 * spacing, ifnameRect.y(), painter.fontMetrics().width(strSentData), painter.fontMetrics().height());
        painter.drawText(sentRect, Qt::AlignLeft | Qt::AlignVCenter, strSentData);

        painter.setPen(Qt::NoPen);
        painter.setBrush(m_recvColor);
        painter.drawEllipse(leftBracketsRect.right() + spacing, leftBracketsRect.y() + qCeil((leftBracketsRect.height() - sectionSize) / 2.0), sectionSize, sectionSize);

        painter.setBrush(m_sentColor);
        painter.drawEllipse(recvRect.right() + 2 * spacing, recvRect.y() + qCeil((recvRect.height() - sectionSize) / 2.0), sectionSize, sectionSize);
    } else {

        QString strRecvData = DApplication::translate("Process.Graph.Title", "Receive");
        strRecvData = strRecvData + " " + m_recv_bps;
        QRect recvRect(margin + spacing + sectionSize, ifnameRect.bottom(), painter.fontMetrics().width(strRecvData), painter.fontMetrics().height());
        painter.drawText(recvRect, Qt::AlignLeft | Qt::AlignVCenter, strRecvData);

        QString strSentData = DApplication::translate("Process.Graph.View", "Send");
        strSentData = strSentData  + " " + m_sent_bps;
        QRect sentRect(margin + spacing + sectionSize, recvRect.bottom(), painter.fontMetrics().width(strSentData), painter.fontMetrics().height());
        painter.drawText(sentRect, Qt::AlignLeft | Qt::AlignVCenter, strSentData);


        painter.setPen(Qt::NoPen);
        painter.setBrush(m_recvColor);
        painter.drawEllipse(margin, recvRect.y() + qCeil((recvRect.height() - sectionSize) / 2.0), sectionSize, sectionSize);

        painter.setBrush(m_sentColor);
        painter.drawEllipse(margin, sentRect.y() + qCeil((sentRect.height() - sectionSize) / 2.0), sectionSize, sectionSize);
    }
}

void NetifItemViewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateWidgetGeometry();
}

void NetifItemViewWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    emit clicked(m_mac);
}

void NetifItemViewWidget::fontChanged(const QFont &font)
{
    m_font = font;
    this->setFont(m_font);
    updateWidgetGeometry();
}

void NetifItemViewWidget::setMode(int mode)
{
    m_mode = mode;
    this->update();
}

void NetifItemViewWidget::updateData(const std::shared_ptr<class core::system::NetifInfo> &netifInfo)
{
    m_ChartWidget->addData1(netifInfo->recv_bps());
    m_ChartWidget->addData2(netifInfo->sent_bps());

    if (!netifInfo->ifname().isNull()) {m_ifname = netifInfo->ifname();}

    m_recv_bps = formatUnit(netifInfo->recv_bps(), B, 1, true);
    m_sent_bps = formatUnit(netifInfo->sent_bps(), B, 1, true);

    this->update();
}

void NetifItemViewWidget::updateActiveStatus(bool active)
{
    m_isActive = active;
    update();
}

void NetifItemViewWidget::updateWidgetGeometry()
{
    int fontHeight = QFontMetrics(m_font).height();
    if (m_mode == TITLE_HORIZONTAL) {
        m_ChartWidget->setGeometry(margin, fontHeight / 2, this->width() - 2 * margin, this->height() - fontHeight / 2 - margin);
    } else {
        m_ChartWidget->setGeometry(margin, int(fontHeight * 2.5), this->width() - 2 * margin, this->height() - int(fontHeight * 2.5) - margin);
    }
}
