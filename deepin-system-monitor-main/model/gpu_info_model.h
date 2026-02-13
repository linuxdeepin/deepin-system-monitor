// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GPU_INFO_MODEL_H
#define GPU_INFO_MODEL_H

#include "system/gpu_info.h"

#include <QObject>
#include <QList>

#include <memory>

using namespace core::system;

class GPUInfoModel : public QObject
{
    Q_OBJECT

public:
    explicit GPUInfoModel();
    ~GPUInfoModel() override = default;

    static GPUInfoModel *instance();

    int gpuCount() const;
    int npuCount() const;
    QList<GPUInfo> gpuList() const;
    
    // Overall stats
    qreal gpuAllUtilization() const;  // Average utilization across all GPUs (0-100)
    qreal npuAllUtilization() const;
    qreal gpuAllMemoryUtilization() const;  // Average memory utilization (0-100)
    quint64 gpuTotalMemory() const;
    quint64 gpuUsedMemory() const;
    quint64 gpuFreeMemory() const;
    
    // Per-GPU stats
    qreal gpuUtilization(int index) const;
    qreal gpuMemoryUtilization(int index) const;
    quint64 gpuTotalMemory(int index) const;
    quint64 gpuUsedMemory(int index) const;
    quint64 gpuFreeMemory(int index) const;
    float gpuTemperature(int index) const;
    QString gpuName(int index) const;
    QString gpuVendor(int index) const;
    
    GPUInfoSet *gpuInfoSet();

signals:
    void modelUpdated();

public slots:
    void updateModel();

private:
    GPUInfoSet *m_gpuInfoSet;
};

#endif // GPU_INFO_MODEL_H
