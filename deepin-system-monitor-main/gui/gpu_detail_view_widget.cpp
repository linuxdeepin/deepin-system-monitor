// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gpu_detail_view_widget.h"

#include "base/base_detail_item_delegate.h"
#include "chart_view_widget.h"
#include "common/common.h"
#include "model/gpu_info_model.h"
#include "system/system_monitor.h"

#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif

#include <QAbstractTableModel>
#include <QApplication>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableView>

using namespace common::format;
using namespace core::system;

namespace {

class GPUSummaryTableModel : public QAbstractTableModel
{
public:
    explicit GPUSummaryTableModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    void setEntries(const QList<QPair<QString, QString>> &entries)
    {
        beginResetModel();
        m_entries = entries;
        endResetModel();
    }

    int rowCount(const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return 0;
        return (m_entries.size() + 1) / 2;
    }

    int columnCount(const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return 0;
        return 2;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        const int entryIndex = index.row() * 2 + index.column();
        if (entryIndex < 0 || entryIndex >= m_entries.size())
            return QVariant();

        if (role == Qt::DisplayRole)
            return m_entries.at(entryIndex).first;

        if (role == Qt::UserRole)
            return m_entries.at(entryIndex).second;

        if (
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            role == Qt::TextColorRole
#else
            role == Qt::ForegroundRole
#endif
        ) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            const auto &palette = DApplicationHelper::instance()->applicationPalette();
#else
            const auto &palette = DGuiApplicationHelper::instance()->applicationPalette();
#endif
            return palette.color(DPalette::Text);
        }

        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &) const override
    {
        return Qt::NoItemFlags;
    }

private:
    QList<QPair<QString, QString>> m_entries;
};

}

GpuDetailViewWidget::GpuDetailViewWidget(QWidget *parent)
    : BaseDetailViewWidget(parent)
{
    this->setObjectName("GpuDetailViewWidget");
    setTitle(DApplication::translate("Process.Graph.View", "GPU"));

    m_utilizationWidget = new QWidget(this);
    auto *utilizationLayout = new QGridLayout(m_utilizationWidget);
    utilizationLayout->setContentsMargins(0, 8, 0, 0);
    utilizationLayout->setHorizontalSpacing(12);
    utilizationLayout->setVerticalSpacing(6);

    m_gpuUtilizationTitleLabel = new QLabel(DApplication::translate("Process.Graph.View", "GPU Utilization"), m_utilizationWidget);
    m_npuUtilizationTitleLabel = new QLabel(DApplication::translate("Process.Graph.View", "NPU Utilization"), m_utilizationWidget);

    m_gpuUtilizationChartWidget = new ChartViewWidget(ChartViewWidget::ChartViewTypes::MEM_CHART, m_utilizationWidget);
    m_gpuUtilizationChartWidget->setData1Color(QColor("#AA00FF"));
    m_gpuUtilizationChartWidget->setMinimumHeight(180);

    m_npuUtilizationChartWidget = new ChartViewWidget(ChartViewWidget::ChartViewTypes::MEM_CHART, m_utilizationWidget);
    m_npuUtilizationChartWidget->setData1Color(QColor("#00A3FF"));
    m_npuUtilizationChartWidget->setMinimumHeight(180);

    utilizationLayout->addWidget(m_gpuUtilizationTitleLabel, 0, 0);
    utilizationLayout->addWidget(m_npuUtilizationTitleLabel, 0, 1);
    utilizationLayout->addWidget(m_gpuUtilizationChartWidget, 1, 0);
    utilizationLayout->addWidget(m_npuUtilizationChartWidget, 1, 1);
    utilizationLayout->setColumnStretch(0, 1);
    utilizationLayout->setColumnStretch(1, 1);

    m_memoryTitleLabel = new QLabel(DApplication::translate("Process.Graph.View", "GPU Memory Utilization"), this);
    m_gpuMemoryChartWidget = new ChartViewWidget(ChartViewWidget::ChartViewTypes::MEM_CHART, this);
    m_gpuMemoryChartWidget->setData1Color(QColor("#00C5C0"));
    m_gpuMemoryChartWidget->setMinimumHeight(170);

    m_deviceTable = new QTableView(this);
    m_deviceModel = new GPUSummaryTableModel(this);
    m_deviceTable->setModel(m_deviceModel);
    m_deviceTable->setItemDelegate(new BaseDetailItemDelegate(this));
    m_deviceTable->horizontalHeader()->setVisible(false);
    m_deviceTable->verticalHeader()->setVisible(false);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_deviceTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_deviceTable->setGridStyle(Qt::NoPen);
    m_deviceTable->setFrameShape(QFrame::NoFrame);
    m_deviceTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deviceTable->setFocusPolicy(Qt::NoFocus);
    m_deviceTable->setMinimumHeight(240);

    m_centralLayout->addWidget(m_utilizationWidget);
    m_centralLayout->addWidget(m_memoryTitleLabel);
    m_centralLayout->addWidget(m_gpuMemoryChartWidget);
    m_centralLayout->addWidget(m_deviceTable);

    detailFontChanged(DApplication::font());
    onModelUpdate();

    connect(SystemMonitor::instance(), &SystemMonitor::statInfoUpdated, this, &GpuDetailViewWidget::onModelUpdate);
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &GpuDetailViewWidget::detailFontChanged);
}

void GpuDetailViewWidget::detailFontChanged(const QFont &font)
{
    BaseDetailViewWidget::detailFontChanged(font);

    QFont contentFont = font;
    contentFont.setPointSizeF(contentFont.pointSizeF() - 1);

    m_utilizationWidget->setFont(contentFont);
    m_memoryTitleLabel->setFont(contentFont);
    m_deviceTable->setFont(contentFont);
}

void GpuDetailViewWidget::onModelUpdate()
{
    auto *gpuModel = GPUInfoModel::instance();

    const qreal gpuUtilization = gpuModel->gpuAllUtilization();
    const qreal npuUtilization = gpuModel->npuAllUtilization();
    const qreal gpuMemoryUtilization = gpuModel->gpuAllMemoryUtilization();

    m_gpuUtilizationChartWidget->addData1(gpuUtilization / 100.0);
    m_npuUtilizationChartWidget->addData1(npuUtilization / 100.0);
    m_gpuMemoryChartWidget->addData1(gpuMemoryUtilization / 100.0);

    m_gpuUtilizationTitleLabel->setText(QString("%1 (%2%)")
                                            .arg(DApplication::translate("Process.Graph.View", "GPU Utilization"))
                                            .arg(QString::number(gpuUtilization, 'f', 1)));

    if (gpuModel->npuCount() > 0) {
        m_npuUtilizationTitleLabel->setText(QString("%1 (%2%)")
                                                .arg(DApplication::translate("Process.Graph.View", "NPU Utilization"))
                                                .arg(QString::number(npuUtilization, 'f', 1)));
    } else {
        m_npuUtilizationTitleLabel->setText(QString("%1 (-)")
                                                .arg(DApplication::translate("Process.Graph.View", "NPU Utilization")));
    }

    setDetail(QString("%1 (%2%)")
                  .arg(DApplication::translate("Process.Graph.View", "GPU Utilization"))
                  .arg(QString::number(gpuUtilization, 'f', 1)));

    const auto devices = gpuModel->gpuList();
    QList<QPair<QString, QString>> tableEntries;

    auto addEntry = [&tableEntries](const QString &field, const QString &value) {
        tableEntries.append(qMakePair(field, value));
    };

    int deviceIndex {0};
    for (const auto &device : devices) {
        if (!device || !device->isAvailable)
            continue;

        const QString type = device->isNpu ? "NPU" : "GPU";
        const QString indexName = QString("%1 %2").arg(type).arg(deviceIndex++);

        const QString name = device->name.isEmpty() ? "-" : device->name;
        const QString vendor = device->vendor.isEmpty() ? "-" : device->vendor;

        QString deviceClass;
        if (device->isNpu) {
            deviceClass = DApplication::translate("Process.Graph.View", "NPU");
        } else if (device->isIntegrated) {
            deviceClass = DApplication::translate("Process.Graph.View", "Embedded in CPU");
        } else {
            deviceClass = DApplication::translate("Process.Graph.View", "External");
        }

        addEntry(DApplication::translate("Process.Graph.View", "Device"), QString("%1 (%2)").arg(indexName).arg(deviceClass));

        QString memory {"-"};
        if (device->totalMemory > 0) {
            memory = QString("%1 / %2")
                         .arg(formatUnit_memory_disk(device->usedMemory, B, 1))
                         .arg(formatUnit_memory_disk(device->totalMemory, B, 1));
        }

        QString temp {"-"};
        if (device->temperature > 0.0f)
            temp = QString("%1 C").arg(QString::number(device->temperature, 'f', 1));

        QString sharedMemory {"-"};
        if (!device->isNpu && device->isIntegrated) {
            if (device->totalMemory > 0)
                sharedMemory = formatUnit_memory_disk(device->totalMemory, B, 1);
            else
                sharedMemory = DApplication::translate("Process.Graph.View", "System RAM (dynamic)");
        }

        QString dedicatedMemory {"-"};
        if (!device->isNpu && !device->isIntegrated && device->totalMemory > 0)
            dedicatedMemory = formatUnit_memory_disk(device->totalMemory, B, 1);

        addEntry(DApplication::translate("Process.Graph.View", "Model"), name);
        addEntry(DApplication::translate("Process.Graph.View", "Vendor"), vendor);
        addEntry(DApplication::translate("Process.Graph.View", "Bus Interface"), device->busInfo.isEmpty() ? "-" : device->busInfo);
        addEntry(DApplication::translate("Process.Graph.View", "Active Time"), QString("%1%").arg(QString::number(device->gpuUtilization, 'f', 1)));

        QString engineValue {"-"};
        if (!device->engineNames.isEmpty() && device->engineNames.size() == device->engineLoads.size()) {
            QStringList enginePairs;
            for (int i = 0; i < device->engineNames.size(); ++i) {
                enginePairs << QString("%1 %2%")
                                   .arg(device->engineNames.at(i))
                                   .arg(QString::number(device->engineLoads.at(i), 'f', 1));
            }
            engineValue = enginePairs.join("; ");
        }
        addEntry(DApplication::translate("Process.Graph.View", "Engines"), engineValue);

        addEntry(DApplication::translate("Process.Graph.View", "Temperature"), temp);
        addEntry(DApplication::translate("Process.Graph.View", "Memory"), memory);
        addEntry(DApplication::translate("Process.Graph.View", "Shared Memory"), sharedMemory);
        addEntry(DApplication::translate("Process.Graph.View", "Dedicated Memory"), dedicatedMemory);
    }

    static_cast<GPUSummaryTableModel *>(m_deviceModel)->setEntries(tableEntries);
}
