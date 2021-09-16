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
#include "mem_detail_view_widget.h"
#include "mem_stat_view_widget.h"
#include "mem_summary_view_widget.h"
#include "system/system_monitor.h"

#include <DApplicationHelper>
#include <DApplication>
#include <QVBoxLayout>

using namespace core::system;
MemDetailViewWidget::MemDetailViewWidget(QWidget *parent)
    : BaseDetailViewWidget(parent)
{
    this->setObjectName("MemDetailViewWidget");
    m_memstatWIdget = new MemStatViewWidget(this);
    m_memsummaryWidget = new MemSummaryViewWidget(this);

    setTitle(DApplication::translate("Process.Graph.Title", "Memory"));
    m_centralLayout->addWidget(m_memstatWIdget);
    m_centralLayout->addWidget(m_memsummaryWidget);

    detailFontChanged(DApplication::font());

    onModelUpdate();
    connect(SystemMonitor::instance(), &SystemMonitor::statInfoUpdated, this, &MemDetailViewWidget::onModelUpdate);
}

void MemDetailViewWidget::onModelUpdate()
{
    m_memstatWIdget->onModelUpdate();
    m_memsummaryWidget->onModelUpdate();
}

void MemDetailViewWidget::detailFontChanged(const QFont &font)
{
    BaseDetailViewWidget::detailFontChanged(font);
    m_memstatWIdget->fontChanged(font);
    m_memsummaryWidget->fontChanged(font);
}
