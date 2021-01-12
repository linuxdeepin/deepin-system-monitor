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
#include "detail_view_stacked_widget.h"
#include "cpu_detail_view_widget.h"
#include "mem_detail_view_widget.h"
#include "netif_detail_view_widget.h"
#include "block_dev_detail_view_widget.h"

#include <DMenu>

DWIDGET_USE_NAMESPACE
DetailViewStackedWidget::DetailViewStackedWidget(QWidget *parent) : QStackedWidget(parent)
{

}

void DetailViewStackedWidget::addProcessWidget(QWidget *processWidget)
{
    m_processWidget = processWidget;
    this->addWidget(processWidget);
}

void DetailViewStackedWidget::onShowPerformMenu(QPoint pos)
{
    DMenu menu;
    QAction *showPerformParamAct = menu.addAction(tr("View performance parameters"));
    QAction *hidePerformParamAct = menu.addAction(tr("Hiding performance parameters"));
    if (this->currentWidget() == m_processWidget) {
        hidePerformParamAct->setDisabled(true);
    }

    DMenu subMenu;
    showPerformParamAct->setMenu(&subMenu);
    QAction *cpuAct = subMenu.addAction(tr("CPU"));
    QAction *memAct = subMenu.addAction(tr("Memory"));
    QAction *netifAct = subMenu.addAction(tr("Netif"));
    QAction *blockDevAct = subMenu.addAction(tr("BlockDev"));

    QAction *resAct = menu.exec(pos);
    if (resAct == cpuAct) {
        if (m_cpudetailWidget == nullptr) {
            m_cpudetailWidget = new CPUDetailViewWidget(this);
            this->addWidget(m_cpudetailWidget);
        }
        this->setCurrentWidget(m_cpudetailWidget);
    } else if (resAct == memAct) {
        if (m_memDetailWidget == nullptr) {
            m_memDetailWidget = new MemDetailViewWidget(this);
            this->addWidget(m_memDetailWidget);
        }
        this->setCurrentWidget(m_memDetailWidget);
    } else if (resAct == netifAct) {
        if (m_netifDetailWidget == nullptr) {
            m_netifDetailWidget = new NetifDetailViewWidget(this);
            this->addWidget(m_netifDetailWidget);
        }
        this->setCurrentWidget(m_netifDetailWidget);
    } else if (resAct == blockDevAct) {
        if (m_blockDevDetailWidget == nullptr) {
            m_blockDevDetailWidget = new BlockDevDetailViewWidget(this);
            this->addWidget(m_blockDevDetailWidget);
        }
        this->setCurrentWidget(m_blockDevDetailWidget);
    } else if (resAct == hidePerformParamAct) {
        this->setCurrentWidget(m_processWidget);
    }
}
