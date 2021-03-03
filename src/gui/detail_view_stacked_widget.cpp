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
#include "cpu_detail_widget.h"
#include "mem_detail_view_widget.h"
#include "netif_detail_view_widget.h"
#include "block_dev_detail_view_widget.h"

#include <DMenu>
#include <DApplication>
#include <QActionGroup>

#define DELETE_PAGE(obj) if(obj) { delete obj; obj = nullptr; }

DetailViewStackedWidget::DetailViewStackedWidget(QWidget *parent) : AnimationStackedWidget(LR, parent)
{
    connect(this, &AnimationStackedWidget::signalIsFinished, this, &DetailViewStackedWidget::onSwitchPageFinished);
}

void DetailViewStackedWidget::addProcessWidget(QWidget *processWidget)
{
    m_processWidget = processWidget;
    this->insertWidget(0, processWidget);
}

void DetailViewStackedWidget::onSwitchPageFinished()
{
    if (this->currentWidget() == m_processWidget) {
        deleteDetailPage();
    } else {
        m_lastDteailWidgetName = this->currentWidget()->objectName();
    }
}

void DetailViewStackedWidget::onDetailInfoClicked()
{
    if (m_lastDteailWidgetName == "MemDetailViewWidget") {
        if (m_memDetailWidget == nullptr) {
            m_memDetailWidget = new MemDetailViewWidget(this);
            this->insertWidget(2, m_memDetailWidget);
        }
        this->setCurrent(m_memDetailWidget);
    } else if (m_lastDteailWidgetName == "NetifDetailViewWidget") {
        if (m_netifDetailWidget == nullptr) {
            m_netifDetailWidget = new NetifDetailViewWidget(this);
            this->insertWidget(3, m_netifDetailWidget);
        }
        this->setCurrent(m_netifDetailWidget);
    } else if (m_lastDteailWidgetName == "BlockDevDetailViewWidget") {
        if (m_blockDevDetailWidget == nullptr) {
            m_blockDevDetailWidget = new BlockDevDetailViewWidget(this);
            this->insertWidget(4, m_blockDevDetailWidget);
        }
        this->setCurrent(m_blockDevDetailWidget);
    } else {
        if (m_cpudetailWidget == nullptr) {
            m_cpudetailWidget = new CPUDetailWidget(this);
            this->insertWidget(1, m_cpudetailWidget);
        }
        this->setCurrent(m_cpudetailWidget);
    }
}

void DetailViewStackedWidget::onSwitchProcessPage()
{
    this->setCurrent(m_processWidget);
}

void DetailViewStackedWidget::onShowPerformMenu(QPoint pos)
{
    if (m_menu == nullptr) {
        m_menu = new DMenu(this);
        cpuAct = m_menu->addAction(DApplication::translate("Process.Graph.View", "CPU"));
        memAct = m_menu->addAction(DApplication::translate("Process.Graph.Title", "Memory"));
        netifAct = m_menu->addAction(DApplication::translate("Process.Graph.View", "Network"));
        blockDevAct = m_menu->addAction(DApplication::translate("Process.Graph.View", "Disks"));

        QActionGroup *actionGroup = new QActionGroup(m_menu);
        actionGroup->addAction(cpuAct);
        actionGroup->addAction(memAct);
        actionGroup->addAction(netifAct);
        actionGroup->addAction(blockDevAct);

        cpuAct->setCheckable(true);
        memAct->setCheckable(true);
        netifAct->setCheckable(true);
        blockDevAct->setCheckable(true);
    }



    if (this->currentWidget() == m_cpudetailWidget) {
        cpuAct->setChecked(true);
    } else if (this->currentWidget() == m_memDetailWidget) {
        memAct->setChecked(true);
    } else if (this->currentWidget() == m_netifDetailWidget) {
        netifAct->setChecked(true);
    } else if (this->currentWidget() == m_blockDevDetailWidget) {
        blockDevAct->setChecked(true);
    }

    QAction *resAct = m_menu->exec(pos);
    if (resAct == cpuAct) {
        if (m_cpudetailWidget == nullptr) {
            m_cpudetailWidget = new CPUDetailWidget(this);
            this->insertWidget(1, m_cpudetailWidget);
        }
        this->setCurrent(m_cpudetailWidget);
    } else if (resAct == memAct) {
        if (m_memDetailWidget == nullptr) {
            m_memDetailWidget = new MemDetailViewWidget(this);
            this->insertWidget(2, m_memDetailWidget);
        }
        this->setCurrent(m_memDetailWidget);
    } else if (resAct == netifAct) {
        if (m_netifDetailWidget == nullptr) {
            m_netifDetailWidget = new NetifDetailViewWidget(this);
            this->insertWidget(3, m_netifDetailWidget);
        }
        this->setCurrent(m_netifDetailWidget);
    } else if (resAct == blockDevAct) {
        if (m_blockDevDetailWidget == nullptr) {
            m_blockDevDetailWidget = new BlockDevDetailViewWidget(this);
            this->insertWidget(4, m_blockDevDetailWidget);
        }
        this->setCurrent(m_blockDevDetailWidget);
    }
}

void DetailViewStackedWidget::deleteDetailPage()
{
    DELETE_PAGE(m_cpudetailWidget);
    DELETE_PAGE(m_memDetailWidget);
    DELETE_PAGE(m_netifDetailWidget);
    DELETE_PAGE(m_blockDevDetailWidget);
}
