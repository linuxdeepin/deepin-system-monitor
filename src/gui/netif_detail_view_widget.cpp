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
#include "common/thread_manager.h"
#include "system/system_monitor_thread.h"
#include "system/device_db.h"
#include <DApplication>
#include <QTimer>

using namespace common::core;
using namespace core::system;

NetifDetailViewWidget::NetifDetailViewWidget(QWidget *parent)
    : BaseDetailViewWidget(parent)
{
    m_netifstatWIdget = new NetifStatViewWidget(this);
    m_netifsummaryWidget = new NetifSummaryViewWidget(this);
    updateData();
    setTitle(DApplication::translate("Process.Graph.View", "Network"));
    m_centralLayout->addWidget(m_netifstatWIdget);
    m_centralLayout->addWidget(m_netifsummaryWidget);
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateData()));
    fontChanged(DApplication::font());
    m_timer->start(2000);
}

void NetifDetailViewWidget::fontChanged(const QFont &font)
{
    BaseDetailViewWidget::fontChanged(font);
    //Child Todo
    m_netifstatWIdget->fontChanged(font);
    m_netifsummaryWidget->fontChanged(font);
}

void NetifDetailViewWidget::updateData()
{
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    auto *info = monitor->deviceDB();
    m_netifstatWIdget->updateWidgetGeometry();
    m_netifsummaryWidget->updateWidgetGeometry(info);
}
