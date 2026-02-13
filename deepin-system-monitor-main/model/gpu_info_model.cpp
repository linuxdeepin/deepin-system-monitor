// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "gpu_info_model.h"
#include "system/device_db.h"
#include "system/system_monitor.h"

#include <QGlobalStatic>

using namespace DDLog;

static GPUInfo filteredDeviceByIndex(GPUInfoSet *set, int index, bool npu)
{
    if (!set || index < 0)
        return nullptr;

    int matched {0};
    const auto devices = set->gpuList();
    for (const auto &device : devices) {
        if (!device || !device->isAvailable)
            continue;
        if (device->isNpu != npu)
            continue;
        if (matched == index)
            return device;
        matched++;
    }
    return nullptr;
}

Q_GLOBAL_STATIC(GPUInfoModel, theGPUInstance)
GPUInfoModel::GPUInfoModel()
    : QObject(nullptr)
    , m_gpuInfoSet(DeviceDB::instance()->gpuInfoSet())
{
    qCDebug(app) << "GPUInfoModel constructor";
    
    // Connect to SystemMonitor's statInfoUpdated signal
    connect(SystemMonitor::instance(), &SystemMonitor::statInfoUpdated, this, &GPUInfoModel::updateModel);
}

GPUInfoModel *GPUInfoModel::instance()
{
    qCDebug(app) << "GPUInfoModel::instance()";
    return theGPUInstance();
}

void GPUInfoModel::updateModel()
{
    qCDebug(app) << "GPUInfoModel::updateModel()";
    // GPU data is updated via DeviceDB, we just emit the signal
    emit modelUpdated();
}

int GPUInfoModel::gpuCount() const
{
    if (!m_gpuInfoSet)
        return 0;
    int count {0};
    const auto gpus = m_gpuInfoSet->gpuList();
    for (const auto &gpu : gpus) {
        if (gpu && gpu->isAvailable && !gpu->isNpu)
            count++;
    }
    return count;
}

int GPUInfoModel::npuCount() const
{
    if (!m_gpuInfoSet)
        return 0;
    return m_gpuInfoSet->npuCount();
}

QList<GPUInfo> GPUInfoModel::gpuList() const
{
    if (!m_gpuInfoSet)
        return QList<GPUInfo>();
    return m_gpuInfoSet->gpuList();
}

qreal GPUInfoModel::gpuAllUtilization() const
{
    if (!m_gpuInfoSet)
        return 0.0;
    return m_gpuInfoSet->averageUtilization();
}

qreal GPUInfoModel::npuAllUtilization() const
{
    if (!m_gpuInfoSet)
        return 0.0;
    return m_gpuInfoSet->averageNpuUtilization();
}

qreal GPUInfoModel::gpuAllMemoryUtilization() const
{
    auto gpus = gpuList();
    if (gpus.isEmpty())
        return 0.0;
    
    qreal total = 0.0;
    int count = 0;
    for (const auto &gpu : gpus) {
        if (gpu && gpu->isAvailable && !gpu->isNpu) {
            total += gpu->memoryUtilization;
            count++;
        }
    }
    return count > 0 ? total / count : 0.0;
}

quint64 GPUInfoModel::gpuTotalMemory() const
{
    if (!m_gpuInfoSet)
        return 0;
    return m_gpuInfoSet->totalMemory();
}

quint64 GPUInfoModel::gpuUsedMemory() const
{
    if (!m_gpuInfoSet)
        return 0;
    return m_gpuInfoSet->usedMemory();
}

quint64 GPUInfoModel::gpuFreeMemory() const
{
    if (!m_gpuInfoSet)
        return 0;
    return m_gpuInfoSet->freeMemory();
}

qreal GPUInfoModel::gpuUtilization(int index) const
{
    if (!m_gpuInfoSet)
        return 0.0;
    auto gpu = filteredDeviceByIndex(m_gpuInfoSet, index, false);
    if (gpu)
        return gpu->gpuUtilization;
    return 0.0;
}

qreal GPUInfoModel::gpuMemoryUtilization(int index) const
{
    if (!m_gpuInfoSet)
        return 0.0;
    auto gpu = filteredDeviceByIndex(m_gpuInfoSet, index, false);
    if (gpu)
        return gpu->memoryUtilization;
    return 0.0;
}

quint64 GPUInfoModel::gpuTotalMemory(int index) const
{
    if (!m_gpuInfoSet)
        return 0;
    auto gpu = filteredDeviceByIndex(m_gpuInfoSet, index, false);
    if (gpu)
        return gpu->totalMemory;
    return 0;
}

quint64 GPUInfoModel::gpuUsedMemory(int index) const
{
    if (!m_gpuInfoSet)
        return 0;
    auto gpu = filteredDeviceByIndex(m_gpuInfoSet, index, false);
    if (gpu)
        return gpu->usedMemory;
    return 0;
}

quint64 GPUInfoModel::gpuFreeMemory(int index) const
{
    if (!m_gpuInfoSet)
        return 0;
    auto gpu = filteredDeviceByIndex(m_gpuInfoSet, index, false);
    if (gpu)
        return gpu->freeMemory;
    return 0;
}

float GPUInfoModel::gpuTemperature(int index) const
{
    if (!m_gpuInfoSet)
        return 0.0f;
    auto gpu = filteredDeviceByIndex(m_gpuInfoSet, index, false);
    if (gpu)
        return gpu->temperature;
    return 0.0f;
}

QString GPUInfoModel::gpuName(int index) const
{
    if (!m_gpuInfoSet)
        return QString();
    auto gpu = filteredDeviceByIndex(m_gpuInfoSet, index, false);
    if (gpu)
        return gpu->name;
    return QString();
}

QString GPUInfoModel::gpuVendor(int index) const
{
    if (!m_gpuInfoSet)
        return QString();
    auto gpu = filteredDeviceByIndex(m_gpuInfoSet, index, false);
    if (gpu)
        return gpu->vendor;
    return QString();
}

GPUInfoSet *GPUInfoModel::gpuInfoSet()
{
    return m_gpuInfoSet;
}
