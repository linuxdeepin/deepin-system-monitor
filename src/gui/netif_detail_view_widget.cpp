/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "netif_detail_view_widget.h"
#include "netif_stat_view_widget.h"
#include "netif_summary_view_widget.h"
#include "system/system_monitor.h"

#include <DApplication>

using namespace core::system;
NetifDetailViewWidget::NetifDetailViewWidget(QWidget *parent)
    : BaseDetailViewWidget(parent)
{
    this->setObjectName("NetifDetailViewWidget");

    m_netifstatWIdget = new NetifStatViewWidget(this);
    m_netifsummaryWidget = new NetifSummaryViewWidget(this);

    connect(SystemMonitor::instance(), &SystemMonitor::statInfoUpdated, this, &NetifDetailViewWidget::updateData);
    connect(m_netifstatWIdget, &NetifStatViewWidget::netifItemClicked, m_netifsummaryWidget, &NetifSummaryViewWidget::onNetifItemClicked);

    setTitle(DApplication::translate("Process.Graph.View", "Network"));
    m_centralLayout->addWidget(m_netifstatWIdget);
    m_centralLayout->addWidget(m_netifsummaryWidget);

    detailFontChanged(DApplication::font());

    updateData();
}

void NetifDetailViewWidget::detailFontChanged(const QFont &font)
{
    BaseDetailViewWidget::detailFontChanged(font);

    m_netifstatWIdget->fontChanged(font);
    m_netifsummaryWidget->fontChanged(font);
}

void NetifDetailViewWidget::updateData()
{
    m_netifstatWIdget->onModelUpdate();
    m_netifsummaryWidget->onModelUpdate();
}
