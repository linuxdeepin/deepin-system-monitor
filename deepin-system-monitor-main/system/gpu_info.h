// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GPU_INFO_H
#define GPU_INFO_H

#include <QSharedDataPointer>
#include <QList>
#include <QString>
#include <QStringList>

namespace core {
namespace system {

struct gpu_info_t {
    int index {0};
    QString name {};
    QString vendor {};
    quint64 totalMemory {0};  // Total memory in bytes
    quint64 usedMemory {0};   // Used memory in bytes
    quint64 freeMemory {0};   // Free memory in bytes
    float gpuUtilization {0.0f};  // GPU load percentage (0-100)
    float memoryUtilization {0.0f}; // Memory utilization percentage (0-100)
    float temperature {0.0f};  // Temperature in Celsius
    quint64 clockSpeed {0};    // Clock speed in MHz
    bool isAvailable {false};  // Whether GPU info is available
    bool isNpu {false};        // Whether this device is NPU
    bool isIntegrated {false}; // Integrated in SoC/CPU package
    QString busInfo {};        // Bus/interface description
    QStringList engineNames {};
    QList<float> engineLoads {}; // Per-engine utilization 0-100
};

using GPUInfo = std::shared_ptr<struct gpu_info_t>;

class GPUInfoSetPrivate;

class GPUInfoSet
{
public:
    GPUInfoSet();
    GPUInfoSet(const GPUInfoSet &other);
    GPUInfoSet &operator=(const GPUInfoSet &rhs);
    virtual ~GPUInfoSet();

public:
    int gpuCount() const;
    QList<GPUInfo> gpuList() const;
    GPUInfo gpu(int index) const;
    
    // Overall stats
    quint64 totalMemory() const;
    quint64 usedMemory() const;
    quint64 freeMemory() const;
    float averageUtilization() const;
    float averageNpuUtilization() const;
    int npuCount() const;
    
    bool hasNvidiaGPU() const;
    bool hasAmdGPU() const;
    bool hasIntelGPU() const;
    bool hasNPU() const;

public:
    void update();

private:
    void detectGPUs();
    void readNvidiaInfo();
    void readAmdInfo();
    void readIntelInfo();
    void readNpuInfo();
    void readSysfsInfo();
    
private:
    QSharedDataPointer<GPUInfoSetPrivate> d;
};

} // namespace system
} // namespace core

#endif // GPU_INFO_H
