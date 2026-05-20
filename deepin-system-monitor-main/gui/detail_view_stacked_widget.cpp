// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "detail_view_stacked_widget.h"
#include "cpu_detail_widget.h"
#include "gpu_detail_view_widget.h"
#include "mem_detail_view_widget.h"
#include "netif_detail_view_widget.h"
#include "block_dev_detail_view_widget.h"
#include "model/gpu_info_model.h"
#include "ddlog.h"

#include <DMenu>
#include <DApplication>
#include <QActionGroup>

using namespace DDLog;

namespace {

bool hasAvailableGpu()
{
    return GPUInfoModel::instance()->gpuCount() > 0;
}

}

#define DELETE_PAGE(obj) if(obj) { delete obj; obj = nullptr; }

DetailViewStackedWidget::DetailViewStackedWidget(QWidget *parent) : AnimationStackedWidget(LR, parent)
{
    qCDebug(app) << "DetailViewStackedWidget constructor";
    connect(this, &AnimationStackedWidget::signalIsFinished, this, &DetailViewStackedWidget::onSwitchPageFinished);
}

DetailViewStackedWidget::~DetailViewStackedWidget()
{
    // qCDebug(app) << "DetailViewStackedWidget destructor";
    if (m_processWidget) {
        // qCDebug(app) << "Deleting process widget";
        delete m_processWidget;
        m_processWidget = nullptr;
    }
    if (m_cpudetailWidget) {
        // qCDebug(app) << "Deleting CPU detail widget";
        delete m_cpudetailWidget;
        m_cpudetailWidget = nullptr;
    }
    if (m_gpuDetailWidget) {
        // qCDebug(app) << "Deleting GPU detail widget";
        delete m_gpuDetailWidget;
        m_gpuDetailWidget = nullptr;
    }
    if (m_memDetailWidget) {
        // qCDebug(app) << "Deleting memory detail widget";
        delete m_memDetailWidget;
        m_memDetailWidget = nullptr;
    }
    if (m_netifDetailWidget) {
        // qCDebug(app) << "Deleting network interface detail widget";
        delete m_netifDetailWidget;
        m_netifDetailWidget = nullptr;
    }
    if (m_blockDevDetailWidget) {
        // qCDebug(app) << "Deleting block device detail widget";
        delete m_blockDevDetailWidget;
        m_blockDevDetailWidget = nullptr;
    }
}

void DetailViewStackedWidget::addProcessWidget(QWidget *processWidget)
{
    qCDebug(app) << "Adding process widget";
    m_processWidget = processWidget;
    this->insertWidget(0, processWidget);
}

void DetailViewStackedWidget::onSwitchPageFinished()
{
    qCDebug(app) << "onSwitchPageFinished";
    if (this->currentWidget() == m_processWidget) {
        qCDebug(app) << "Current widget is process widget, deleting detail pages";
        deleteDetailPage();
    } else {
        qCDebug(app) << "Current widget is a detail page, updating last detail widget name";
        m_lastDteailWidgetName = this->currentWidget()->objectName();
    }
}

void DetailViewStackedWidget::onDbusSendMsgChangeDetailInfoWidget(QString msgCode)
{
    qCDebug(app) << "onDbusSendMsgChangeDetailInfoWidget with message code:" << msgCode;
    if (msgCode.compare(QString("MSG_CPU"), Qt::CaseInsensitive) == 0) {
        qCDebug(app) << "Switching to CPU page";
        if (m_cpudetailWidget == nullptr) {
            qCDebug(app) << "CPU detail widget is null, creating new one";
            m_cpudetailWidget = new CPUDetailWidget(this);
            this->insertWidget(1, m_cpudetailWidget);
        }
        qCDebug(app) << "Setting current widget to CPU detail widget";
        this->setCurrent(m_cpudetailWidget);
        return;
    }

    if (msgCode.compare(QString("MSG_GPU"), Qt::CaseInsensitive) == 0) {
        if (!hasAvailableGpu()) {
            qCDebug(app) << "GPU unavailable, ignore MSG_GPU";
            return;
        }
        qCDebug(app) << "Switching to GPU page";
        if (m_gpuDetailWidget == nullptr) {
            qCDebug(app) << "GPU detail widget is null, creating new one";
            m_gpuDetailWidget = new GpuDetailViewWidget(this);
            this->insertWidget(2, m_gpuDetailWidget);
        }
        qCDebug(app) << "Setting current widget to GPU detail widget";
        this->setCurrent(m_gpuDetailWidget);
        return;
    }

    if (msgCode.compare(QString("MSG_NET"), Qt::CaseInsensitive) == 0) {
        qCDebug(app) << "Switching to Network page";
        if (m_netifDetailWidget == nullptr) {
            qCDebug(app) << "Network detail widget is null, creating new one";
            m_netifDetailWidget = new NetifDetailViewWidget(this);
            this->insertWidget(4, m_netifDetailWidget);
        }
        qCDebug(app) << "Setting current widget to Network detail widget";
        this->setCurrent(m_netifDetailWidget);
        return;
    }

    if (msgCode.compare(QString("MSG_MEM"), Qt::CaseInsensitive) == 0) {
        qCDebug(app) << "Switching to Memory page";
        if (m_memDetailWidget == nullptr) {
            qCDebug(app) << "Memory detail widget is null, creating new one";
            m_memDetailWidget = new MemDetailViewWidget(this);
            this->insertWidget(3, m_memDetailWidget);
        }
        qCDebug(app) << "Setting current widget to Memory detail widget";
        this->setCurrent(m_memDetailWidget);
        return;
    }

    if (msgCode.compare(QString("MSG_DISK"), Qt::CaseInsensitive) == 0) {
        qCDebug(app) << "Switching to Disk page";
        if (m_blockDevDetailWidget == nullptr) {
            qCDebug(app) << "Disk detail widget is null, creating new one";
            m_blockDevDetailWidget = new BlockDevDetailViewWidget(this);
            this->insertWidget(5, m_blockDevDetailWidget);
        }
        qCDebug(app) << "Setting current widget to Disk detail widget";
        this->setCurrent(m_blockDevDetailWidget);
        return;
    }
}

void DetailViewStackedWidget::onDetailInfoClicked()
{
    qCDebug(app) << "onDetailInfoClicked, last detail widget name was" << m_lastDteailWidgetName;
    if (m_lastDteailWidgetName == "GpuDetailViewWidget") {
        if (!hasAvailableGpu()) {
            qCDebug(app) << "GPU unavailable, fallback to CPU page";
            m_lastDteailWidgetName.clear();
        } else {
        qCDebug(app) << "Switching to GPU page";
        if (m_gpuDetailWidget == nullptr) {
            qCDebug(app) << "GPU detail widget is null, creating new one";
            m_gpuDetailWidget = new GpuDetailViewWidget(this);
            this->insertWidget(2, m_gpuDetailWidget);
        }
        this->setCurrent(m_gpuDetailWidget);
            return;
        }
    } else if (m_lastDteailWidgetName == "MemDetailViewWidget") {
        qCDebug(app) << "Switching to Memory page";
        if (m_memDetailWidget == nullptr) {
            qCDebug(app) << "Memory detail widget is null, creating new one";
            m_memDetailWidget = new MemDetailViewWidget(this);
            this->insertWidget(3, m_memDetailWidget);
        }
        this->setCurrent(m_memDetailWidget);
    } else if (m_lastDteailWidgetName == "NetifDetailViewWidget") {
        qCDebug(app) << "Switching to Network page";
        if (m_netifDetailWidget == nullptr) {
            qCDebug(app) << "Network detail widget is null, creating new one";
            m_netifDetailWidget = new NetifDetailViewWidget(this);
            this->insertWidget(4, m_netifDetailWidget);
        }
        this->setCurrent(m_netifDetailWidget);
    } else if (m_lastDteailWidgetName == "BlockDevDetailViewWidget") {
        qCDebug(app) << "Switching to Disk page";
        if (m_blockDevDetailWidget == nullptr) {
            qCDebug(app) << "Disk detail widget is null, creating new one";
            m_blockDevDetailWidget = new BlockDevDetailViewWidget(this);
            this->insertWidget(5, m_blockDevDetailWidget);
        }
        this->setCurrent(m_blockDevDetailWidget);
    } else {
        qCDebug(app) << "Switching to CPU page by default";
        if (m_cpudetailWidget == nullptr) {
            qCDebug(app) << "CPU detail widget is null, creating new one";
            m_cpudetailWidget = new CPUDetailWidget(this);
            this->insertWidget(1, m_cpudetailWidget);
        }
        this->setCurrent(m_cpudetailWidget);
    }
}

void DetailViewStackedWidget::onSwitchProcessPage()
{
    qCDebug(app) << "Switching to process page";
    this->setCurrent(m_processWidget);
}

void DetailViewStackedWidget::onShowPerformMenu(QPoint pos)
{
    qCDebug(app) << "onShowPerformMenu";
    if (m_menu == nullptr) {
        qCDebug(app) << "Menu is null, creating new one";
        m_menu = new DMenu(this);
        cpuAct = m_menu->addAction(DApplication::translate("Process.Graph.View", "CPU"));
        gpuAct = m_menu->addAction(DApplication::translate("Process.Graph.View", "GPU"));
        memAct = m_menu->addAction(DApplication::translate("Process.Graph.Title", "Memory"));
        netifAct = m_menu->addAction(DApplication::translate("Process.Graph.View", "Network"));
        blockDevAct = m_menu->addAction(DApplication::translate("Process.Graph.View", "Disks"));

        QActionGroup *actionGroup = new QActionGroup(m_menu);
        actionGroup->addAction(cpuAct);
        actionGroup->addAction(gpuAct);
        actionGroup->addAction(memAct);
        actionGroup->addAction(netifAct);
        actionGroup->addAction(blockDevAct);

        cpuAct->setCheckable(true);
        gpuAct->setCheckable(true);
        memAct->setCheckable(true);
        netifAct->setCheckable(true);
        blockDevAct->setCheckable(true);
    }

    const bool gpuAvailable = hasAvailableGpu();
    gpuAct->setVisible(gpuAvailable);



    if (this->currentWidget() == m_cpudetailWidget) {
        qCDebug(app) << "Current widget is CPU, checking CPU action";
        cpuAct->setChecked(true);
    } else if (this->currentWidget() == m_gpuDetailWidget) {
        qCDebug(app) << "Current widget is GPU, checking GPU action";
        gpuAct->setChecked(true);
    } else if (this->currentWidget() == m_memDetailWidget) {
        qCDebug(app) << "Current widget is Memory, checking Memory action";
        memAct->setChecked(true);
    } else if (this->currentWidget() == m_netifDetailWidget) {
        qCDebug(app) << "Current widget is Network, checking Network action";
        netifAct->setChecked(true);
    } else if (this->currentWidget() == m_blockDevDetailWidget) {
        qCDebug(app) << "Current widget is Disk, checking Disk action";
        blockDevAct->setChecked(true);
    }

    QAction *resAct = m_menu->exec(pos);
    if (resAct == cpuAct) {
        qCDebug(app) << "CPU action triggered";
        if (m_cpudetailWidget == nullptr) {
            qCDebug(app) << "CPU detail widget is null, creating new one";
            m_cpudetailWidget = new CPUDetailWidget(this);
            this->insertWidget(1, m_cpudetailWidget);
        }
        this->setCurrent(m_cpudetailWidget);
    } else if (resAct == gpuAct) {
        if (!hasAvailableGpu())
            return;
        qCDebug(app) << "GPU action triggered";
        if (m_gpuDetailWidget == nullptr) {
            qCDebug(app) << "GPU detail widget is null, creating new one";
            m_gpuDetailWidget = new GpuDetailViewWidget(this);
            this->insertWidget(2, m_gpuDetailWidget);
        }
        this->setCurrent(m_gpuDetailWidget);
    } else if (resAct == memAct) {
        qCDebug(app) << "Memory action triggered";
        if (m_memDetailWidget == nullptr) {
            qCDebug(app) << "Memory detail widget is null, creating new one";
            m_memDetailWidget = new MemDetailViewWidget(this);
            this->insertWidget(3, m_memDetailWidget);
        }
        this->setCurrent(m_memDetailWidget);
    } else if (resAct == netifAct) {
        qCDebug(app) << "Network action triggered";
        if (m_netifDetailWidget == nullptr) {
            qCDebug(app) << "Network detail widget is null, creating new one";
            m_netifDetailWidget = new NetifDetailViewWidget(this);
            this->insertWidget(4, m_netifDetailWidget);
        }
        this->setCurrent(m_netifDetailWidget);
    } else if (resAct == blockDevAct) {
        qCDebug(app) << "Disk action triggered";
        if (m_blockDevDetailWidget == nullptr) {
            qCDebug(app) << "Disk detail widget is null, creating new one";
            m_blockDevDetailWidget = new BlockDevDetailViewWidget(this);
            this->insertWidget(5, m_blockDevDetailWidget);
        }
        this->setCurrent(m_blockDevDetailWidget);
    }
}

void DetailViewStackedWidget::deleteDetailPage()
{
    qCDebug(app) << "Deleting all detail pages";
    DELETE_PAGE(m_cpudetailWidget);
    DELETE_PAGE(m_gpuDetailWidget);
    DELETE_PAGE(m_memDetailWidget);
    DELETE_PAGE(m_netifDetailWidget);
    DELETE_PAGE(m_blockDevDetailWidget);
}
