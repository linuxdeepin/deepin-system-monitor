// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "gpu_info.h"

#include <QSharedData>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QFileInfo>

using namespace DDLog;

namespace core {
namespace system {

class GPUInfoSetPrivate : public QSharedData
{
public:
    QList<GPUInfo> m_gpuList;
    bool m_hasNvidia {false};
    bool m_hasAmd {false};
    bool m_hasIntel {false};
    bool m_hasNpu {false};
    bool m_detected {false};
};

static float readPercentFromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return -1.0f;

    const QString text = QString::fromUtf8(file.readAll()).trimmed();
    file.close();
    if (text.isEmpty())
        return -1.0f;

    bool ok {false};
    float value = text.toFloat(&ok);
    if (!ok) {
        QRegularExpression re("([0-9]+(?:\\.[0-9]+)?)");
        QRegularExpressionMatchIterator it = re.globalMatch(text);
        QList<float> nums;
        while (it.hasNext()) {
            const auto m = it.next();
            nums.append(m.captured(1).toFloat());
        }
        if (nums.isEmpty())
            return -1.0f;

        if (nums.size() >= 2 && nums[1] > 0.0f && nums[0] <= nums[1])
            value = nums[0] * 100.0f / nums[1];
        else
            value = nums[0];
    }

    if (value > 100.0f && value <= 1000.0f)
        value /= 10.0f;
    if (value < 0.0f)
        value = 0.0f;
    if (value > 100.0f)
        value = 100.0f;
    return value;
}

static bool isNpuLikeName(const QString &name)
{
    const QString lower = name.toLower();
    return lower.contains("npu") || lower.contains("rknpu") || lower.contains("apu")
           || lower.contains("neuron") || lower.contains("vpu");
}

static float readTempFromHwmon(const QString &devicePath)
{
    QDir hwmonDir(devicePath + "/hwmon");
    if (!hwmonDir.exists())
        return 0.0f;

    const QStringList entries = hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        QFile tempFile(hwmonDir.path() + "/" + entry + "/temp1_input");
        if (!tempFile.open(QIODevice::ReadOnly))
            continue;
        float value = tempFile.readAll().trimmed().toFloat();
        tempFile.close();
        if (value > 1000.0f)
            value /= 1000.0f;
        if (value > 0.0f)
            return value;
    }
    return 0.0f;
}

static QString pciBusLabelFromDevicePath(const QString &devicePath)
{
    const QString canonical = QFileInfo(devicePath).canonicalFilePath();
    if (canonical.isEmpty())
        return QString();

    const QString slot = QFileInfo(canonical).fileName();
    if (slot.contains(':') && slot.contains('.'))
        return QString("PCIe %1").arg(slot);
    return QString();
}

static QString pciDeviceNameFromDevicePath(const QString &devicePath)
{
    const QString canonical = QFileInfo(devicePath).canonicalFilePath();
    if (canonical.isEmpty())
        return QString();

    const QString slot = QFileInfo(canonical).fileName();
    if (slot.isEmpty())
        return QString();

    QProcess lspci;
    lspci.start("lspci", QStringList() << "-s" << slot);
    if (!lspci.waitForFinished(2000) || lspci.exitCode() != 0)
        return QString();

    QString text = QString::fromUtf8(lspci.readAllStandardOutput()).trimmed();
    if (text.isEmpty())
        return QString();

    text.remove(QRegularExpression("^\\S+\\s+"));
    const int splitPos = text.indexOf(": ");
    if (splitPos >= 0)
        text = text.mid(splitPos + 2).trimmed();

    return text;
}

static bool readU64FromFile(const QString &path, quint64 &value)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    bool ok {false};
    const quint64 parsed = file.readAll().trimmed().toULongLong(&ok);
    file.close();
    if (!ok)
        return false;

    value = parsed;
    return true;
}

static bool detectDedicatedMemoryFromSysfs(const QString &devicePath, quint64 &totalBytes, quint64 &usedBytes)
{
    totalBytes = 0;
    usedBytes = 0;

    quint64 detectedTotal {0};
    quint64 detectedUsed {0};

    const QStringList totalCandidates {
        devicePath + "/mem_info_vram_total",
        devicePath + "/lmem_total_bytes"
    };

    const QStringList usedCandidates {
        devicePath + "/mem_info_vram_used",
        devicePath + "/lmem_used_bytes"
    };

    for (const QString &path : totalCandidates) {
        if (readU64FromFile(path, detectedTotal) && detectedTotal > 0)
            break;
        detectedTotal = 0;
    }

    if (detectedTotal == 0)
        return false;

    for (const QString &path : usedCandidates) {
        if (readU64FromFile(path, detectedUsed))
            break;
        detectedUsed = 0;
    }

    totalBytes = detectedTotal;
    usedBytes = qMin(detectedUsed, detectedTotal);
    return true;
}

static bool isLikelyAmdIntegratedGpuName(const QString &name)
{
    const QString lower = name.toLower();
    if (lower.isEmpty())
        return false;

    if (lower.contains("rx ") || lower.contains("radeon pro") || lower.contains("firepro"))
        return false;

    return lower.contains("graphics") || lower.contains("apu");
}

static void appendEngineLoadIfValid(GPUInfo &device, const QString &engineName, const QString &path)
{
    const float value = readPercentFromFile(path);
    if (value < 0.0f)
        return;
    device->engineNames.append(engineName);
    device->engineLoads.append(value);
}

static QString normalizeEngineName(const QString &engineDirName)
{
    const QString lower = engineDirName.toLower();
    if (lower.startsWith("rcs"))
        return "3D";
    if (lower.startsWith("ccs"))
        return "Compute";
    if (lower.startsWith("bcs"))
        return "Copy";
    if (lower.startsWith("vcs") || lower.startsWith("vcn"))
        return "Video Decode";
    if (lower.startsWith("vecs"))
        return "Video Enhance";
    return engineDirName;
}

static void appendDrmEngineLoads(GPUInfo &device, const QString &devicePath)
{
    QDir engineDir(devicePath + "/engine");
    if (!engineDir.exists())
        return;

    const QStringList engineEntries = engineDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : engineEntries) {
        const QString entryPath = engineDir.path() + "/" + entry;
        float load = readPercentFromFile(entryPath + "/busy_percent");
        if (load < 0.0f)
            load = readPercentFromFile(entryPath + "/busy");
        if (load < 0.0f)
            load = readPercentFromFile(entryPath + "/utilization");
        if (load < 0.0f)
            continue;

        device->engineNames.append(normalizeEngineName(entry));
        device->engineLoads.append(load);
    }
}

GPUInfoSet::GPUInfoSet()
    : d(new GPUInfoSetPrivate())
{
    qCDebug(app) << "GPUInfoSet constructor";
    detectGPUs();
}

GPUInfoSet::GPUInfoSet(const GPUInfoSet &other)
    : d(other.d)
{
}

GPUInfoSet &GPUInfoSet::operator=(const GPUInfoSet &rhs)
{
    if (this != &rhs)
        d.operator=(rhs.d);
    return *this;
}

GPUInfoSet::~GPUInfoSet()
{
}

int GPUInfoSet::gpuCount() const
{
    return d->m_gpuList.size();
}

QList<GPUInfo> GPUInfoSet::gpuList() const
{
    return d->m_gpuList;
}

GPUInfo GPUInfoSet::gpu(int index) const
{
    if (index >= 0 && index < d->m_gpuList.size())
        return d->m_gpuList[index];
    return nullptr;
}

quint64 GPUInfoSet::totalMemory() const
{
    quint64 total = 0;
    for (const auto &gpu : d->m_gpuList) {
        if (gpu && gpu->isAvailable)
            total += gpu->totalMemory;
    }
    return total;
}

quint64 GPUInfoSet::usedMemory() const
{
    quint64 used = 0;
    for (const auto &gpu : d->m_gpuList) {
        if (gpu && gpu->isAvailable)
            used += gpu->usedMemory;
    }
    return used;
}

quint64 GPUInfoSet::freeMemory() const
{
    quint64 free = 0;
    for (const auto &gpu : d->m_gpuList) {
        if (gpu && gpu->isAvailable)
            free += gpu->freeMemory;
    }
    return free;
}

float GPUInfoSet::averageUtilization() const
{
    float total = 0.0f;
    int count = 0;
    for (const auto &gpu : d->m_gpuList) {
        if (gpu && gpu->isAvailable && !gpu->isNpu) {
            total += gpu->gpuUtilization;
            count++;
        }
    }
    return count > 0 ? total / count : 0.0f;
}

float GPUInfoSet::averageNpuUtilization() const
{
    float total = 0.0f;
    int count = 0;
    for (const auto &gpu : d->m_gpuList) {
        if (gpu && gpu->isAvailable && gpu->isNpu) {
            total += gpu->gpuUtilization;
            count++;
        }
    }
    return count > 0 ? total / count : 0.0f;
}

int GPUInfoSet::npuCount() const
{
    int count {0};
    for (const auto &gpu : d->m_gpuList) {
        if (gpu && gpu->isAvailable && gpu->isNpu)
            count++;
    }
    return count;
}

bool GPUInfoSet::hasNvidiaGPU() const
{
    return d->m_hasNvidia;
}

bool GPUInfoSet::hasAmdGPU() const
{
    return d->m_hasAmd;
}

bool GPUInfoSet::hasIntelGPU() const
{
    return d->m_hasIntel;
}

bool GPUInfoSet::hasNPU() const
{
    return d->m_hasNpu;
}

void GPUInfoSet::detectGPUs()
{
    if (d->m_detected)
        return;
    
    qCDebug(app) << "Detecting GPUs...";
    
    // Check for NVIDIA GPUs
    QProcess nvidiaSmi;
    nvidiaSmi.start("which", QStringList() << "nvidia-smi");
    nvidiaSmi.waitForFinished();
    if (nvidiaSmi.exitCode() == 0) {
        d->m_hasNvidia = true;
        qCDebug(app) << "NVIDIA GPU detected";
    }
    
    // Check for AMD/Intel GPUs via sysfs
    QDir drmDir("/sys/class/drm");
    if (drmDir.exists()) {
        QStringList entries = drmDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &entry : entries) {
            if (entry.startsWith("card")) {
                QString devicePath = QString("/sys/class/drm/%1/device").arg(entry);
                QFile vendorFile(devicePath + "/vendor");
                if (vendorFile.open(QIODevice::ReadOnly)) {
                    QString vendor = vendorFile.readAll().trimmed();
                    // AMD vendor ID is 0x1002
                    if (vendor == "0x1002") {
                        d->m_hasAmd = true;
                        qCDebug(app) << "AMD GPU detected at" << entry;
                    }
                    // Intel vendor ID is 0x8086
                    else if (vendor == "0x8086") {
                        d->m_hasIntel = true;
                        qCDebug(app) << "Intel GPU detected at" << entry;
                    }
                    vendorFile.close();
                }
            }
        }
    }

    // Check for NPU via devfreq naming convention
    QDir devfreqDir("/sys/class/devfreq");
    if (devfreqDir.exists()) {
        const QStringList entries = devfreqDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &entry : entries) {
            if (isNpuLikeName(entry)) {
                d->m_hasNpu = true;
                qCDebug(app) << "NPU device detected at" << entry;
                break;
            }

            QFile typeFile(devfreqDir.path() + "/" + entry + "/device/of_node/name");
            if (typeFile.open(QIODevice::ReadOnly)) {
                const QString ofName = QString::fromUtf8(typeFile.readAll()).trimmed();
                typeFile.close();
                if (isNpuLikeName(ofName)) {
                    d->m_hasNpu = true;
                    qCDebug(app) << "NPU device detected by of_node at" << entry << ofName;
                    break;
                }
            }
        }
    }

    if (QFileInfo::exists("/sys/class/misc/apex_0")) {
        d->m_hasNpu = true;
        qCDebug(app) << "Edge TPU (apex) detected";
    }

    QDir miscDir("/sys/class/misc");
    if (!d->m_hasNpu && miscDir.exists()) {
        const QStringList entries = miscDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &entry : entries) {
            if (isNpuLikeName(entry)) {
                d->m_hasNpu = true;
                qCDebug(app) << "NPU misc device detected at" << entry;
                break;
            }
        }
    }
    
    d->m_detected = true;
}

void GPUInfoSet::readNvidiaInfo()
{
    if (!d->m_hasNvidia)
        return;
    
    QProcess nvidiaSmi;
    // Query: index, name, utilization.gpu, utilization.memory, memory.total, memory.used, memory.free, temperature.gpu, clocks.current.graphics
    nvidiaSmi.start("nvidia-smi", QStringList() 
        << "--query-gpu=index,name,utilization.gpu,utilization.memory,memory.total,memory.used,memory.free,temperature.gpu,clocks.current.graphics,pci.bus_id"
        << "--format=csv,noheader,nounits");
    
    if (!nvidiaSmi.waitForFinished(5000)) {
        qCWarning(app) << "nvidia-smi command timed out";
        return;
    }
    
    if (nvidiaSmi.exitCode() != 0) {
        qCWarning(app) << "nvidia-smi command failed:" << nvidiaSmi.readAllStandardError();
        return;
    }
    
    QString output = nvidiaSmi.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString &line : lines) {
        QStringList parts = line.split(", ");
        if (parts.size() >= 10) {
            GPUInfo gpu(new gpu_info_t());
            gpu->index = parts[0].toInt();
            gpu->name = parts[1].trimmed();
            gpu->vendor = "NVIDIA";
            gpu->isIntegrated = false;
            gpu->gpuUtilization = parts[2].toFloat();
            gpu->memoryUtilization = parts[3].toFloat();
            // Memory values are in MiB, convert to bytes
            gpu->totalMemory = static_cast<quint64>(parts[4].toDouble() * 1024 * 1024);
            gpu->usedMemory = static_cast<quint64>(parts[5].toDouble() * 1024 * 1024);
            gpu->freeMemory = static_cast<quint64>(parts[6].toDouble() * 1024 * 1024);
            gpu->temperature = parts[7].toFloat();
            gpu->clockSpeed = parts[8].toULongLong();
            gpu->busInfo = QString("PCIe %1").arg(parts[9].trimmed());
            gpu->engineNames.append("Graphics");
            gpu->engineLoads.append(gpu->gpuUtilization);
            gpu->isAvailable = true;
            
            d->m_gpuList.append(gpu);
            qCDebug(app) << "NVIDIA GPU" << gpu->index << ":" << gpu->name 
                         << "Utilization:" << gpu->gpuUtilization << "%"
                         << "Memory:" << gpu->usedMemory / (1024*1024) << "/" << gpu->totalMemory / (1024*1024) << "MiB";
        }
    }
}

void GPUInfoSet::readAmdInfo()
{
    // For AMD GPUs using open source drivers, we can read from sysfs
    QDir drmDir("/sys/class/drm");
    if (!drmDir.exists())
        return;
    
    QStringList entries = drmDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    int gpuIndex = 0;
    
    for (const QString &entry : entries) {
        if (!entry.startsWith("card"))
            continue;
            
        QString devicePath = QString("/sys/class/drm/%1/device").arg(entry);
        QFile vendorFile(devicePath + "/vendor");
        if (!vendorFile.open(QIODevice::ReadOnly))
            continue;
            
        QString vendor = vendorFile.readAll().trimmed();
        vendorFile.close();
        
        if (vendor != "0x1002")  // Not AMD
            continue;
        
        GPUInfo gpu(new gpu_info_t());
        gpu->index = gpuIndex++;
        gpu->vendor = "AMD";
        gpu->busInfo = pciBusLabelFromDevicePath(devicePath);

        gpu->name = pciDeviceNameFromDevicePath(devicePath);
        if (gpu->name.isEmpty())
            gpu->name = "AMD GPU";
        
        quint64 dedicatedTotal {0};
        quint64 dedicatedUsed {0};
        const bool hasDedicatedVramStats = detectDedicatedMemoryFromSysfs(devicePath, dedicatedTotal, dedicatedUsed);
        if (hasDedicatedVramStats) {
            gpu->totalMemory = dedicatedTotal;
            gpu->usedMemory = dedicatedUsed;
        }
        
        if (gpu->totalMemory > 0 && gpu->usedMemory <= gpu->totalMemory) {
            gpu->freeMemory = gpu->totalMemory - gpu->usedMemory;
            gpu->memoryUtilization = (static_cast<float>(gpu->usedMemory) / gpu->totalMemory) * 100.0f;
        }
        
        // Read GPU utilization if available
        QFile gpuBusyFile(devicePath + "/gpu_busy_percent");
        if (gpuBusyFile.open(QIODevice::ReadOnly)) {
            gpu->gpuUtilization = gpuBusyFile.readAll().trimmed().toFloat();
            gpuBusyFile.close();
        }

        appendEngineLoadIfValid(gpu, "Graphics", devicePath + "/gpu_busy_percent");
        appendDrmEngineLoads(gpu, devicePath);
        
        gpu->temperature = readTempFromHwmon(devicePath);
        const bool likelyIntegratedByName = isLikelyAmdIntegratedGpuName(gpu->name);
        gpu->isIntegrated = !hasDedicatedVramStats || likelyIntegratedByName;
        if (gpu->isIntegrated && gpu->busInfo.isEmpty())
            gpu->busInfo = "SoC/Platform";
        
        gpu->isAvailable = true;
        d->m_gpuList.append(gpu);
        
        qCDebug(app) << "AMD GPU" << gpu->index << ":" << gpu->name 
                     << "Utilization:" << gpu->gpuUtilization << "%"
                     << "Memory:" << gpu->usedMemory / (1024*1024) << "/" << gpu->totalMemory / (1024*1024) << "MiB";
    }
}

void GPUInfoSet::readIntelInfo()
{
    // For Intel GPUs, read from sysfs
    QDir drmDir("/sys/class/drm");
    if (!drmDir.exists())
        return;
    
    QStringList entries = drmDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    int gpuIndex = 0;
    
    for (const QString &entry : entries) {
        if (!entry.startsWith("card"))
            continue;
            
        QString devicePath = QString("/sys/class/drm/%1/device").arg(entry);
        QFile vendorFile(devicePath + "/vendor");
        if (!vendorFile.open(QIODevice::ReadOnly))
            continue;
            
        QString vendor = vendorFile.readAll().trimmed();
        vendorFile.close();
        
        if (vendor != "0x8086")  // Not Intel
            continue;
        
        GPUInfo gpu(new gpu_info_t());
        gpu->index = gpuIndex++;
        gpu->vendor = "Intel";
        gpu->name = pciDeviceNameFromDevicePath(devicePath);
        if (gpu->name.isEmpty())
            gpu->name = "Intel GPU";

        quint64 dedicatedTotal {0};
        quint64 dedicatedUsed {0};
        const bool hasDedicatedVramStats = detectDedicatedMemoryFromSysfs(devicePath, dedicatedTotal, dedicatedUsed);
        if (hasDedicatedVramStats) {
            gpu->totalMemory = dedicatedTotal;
            gpu->usedMemory = dedicatedUsed;
            gpu->freeMemory = gpu->totalMemory - gpu->usedMemory;
            gpu->memoryUtilization = gpu->totalMemory > 0 ? (static_cast<float>(gpu->usedMemory) / gpu->totalMemory) * 100.0f : 0.0f;
        }
        gpu->isIntegrated = !hasDedicatedVramStats;
        gpu->busInfo = pciBusLabelFromDevicePath(devicePath);
        if (gpu->busInfo.isEmpty())
            gpu->busInfo = "SoC/Platform";

        appendEngineLoadIfValid(gpu, "Render", devicePath + "/gt_busy_percent");
        appendDrmEngineLoads(gpu, devicePath);
        for (int i = 0; i < gpu->engineNames.size(); ++i) {
            if (gpu->engineNames.at(i) == "3D") {
                gpu->gpuUtilization = gpu->engineLoads.at(i);
                break;
            }
        }
        if (gpu->gpuUtilization <= 0.0f && !gpu->engineLoads.isEmpty())
            gpu->gpuUtilization = gpu->engineLoads.first();
        
        // Intel i915 driver provides some stats via debugfs or sysfs
        // Read from /sys/kernel/debug/dri if available
        
        // For now, just mark as available
        gpu->isAvailable = true;
        d->m_gpuList.append(gpu);
        
        qCDebug(app) << "Intel GPU" << gpu->index << "detected";
    }
}

void GPUInfoSet::readNpuInfo()
{
    if (!d->m_hasNpu)
        return;

    QDir devfreqDir("/sys/class/devfreq");
    int npuIndex {0};
    if (devfreqDir.exists()) {
        const QStringList entries = devfreqDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &entry : entries) {
            bool isNpu = isNpuLikeName(entry);
            if (!isNpu) {
                QFile typeFile(devfreqDir.path() + "/" + entry + "/device/of_node/name");
                if (typeFile.open(QIODevice::ReadOnly)) {
                    const QString ofName = QString::fromUtf8(typeFile.readAll()).trimmed();
                    typeFile.close();
                    isNpu = isNpuLikeName(ofName);
                }
            }
            if (!isNpu)
                continue;

            const QString path = devfreqDir.path() + "/" + entry;
            GPUInfo npu(new gpu_info_t());
            npu->index = npuIndex++;
            npu->isNpu = true;
            npu->isAvailable = true;
            npu->vendor = "NPU";
            npu->name = entry;
            npu->isIntegrated = true;
            npu->busInfo = "SoC/Platform";

            float util = readPercentFromFile(path + "/load");
            if (util < 0.0f)
                util = readPercentFromFile(path + "/utilization");
            if (util < 0.0f)
                util = readPercentFromFile(path + "/busy_percent");
            if (util < 0.0f)
                util = readPercentFromFile(path + "/device/load");
            if (util < 0.0f)
                util = 0.0f;
            npu->gpuUtilization = util;
            npu->engineNames.append("NPU Core");
            npu->engineLoads.append(util);

            QFile deviceLink(path + "/device");
            if (deviceLink.exists()) {
                const QString devicePath = QFileInfo(deviceLink.fileName()).symLinkTarget();
                if (!devicePath.isEmpty())
                    npu->temperature = readTempFromHwmon(devicePath);
            }

            d->m_gpuList.append(npu);
            qCDebug(app) << "NPU" << npu->index << ":" << npu->name << "Utilization:" << npu->gpuUtilization << "%";
        }
    }

    if (QFileInfo::exists("/sys/class/misc/apex_0")) {
        GPUInfo npu(new gpu_info_t());
        npu->index = npuIndex;
        npu->isNpu = true;
        npu->isAvailable = true;
        npu->vendor = "Google";
        npu->name = "Edge TPU";
        npu->isIntegrated = false;
        npu->busInfo = "PCIe/USB";
        npu->gpuUtilization = 0.0f;
        npu->engineNames.append("NPU Core");
        npu->engineLoads.append(0.0f);
        d->m_gpuList.append(npu);
    }
}

void GPUInfoSet::readSysfsInfo()
{
    // General sysfs reading for any DRM device
    QDir drmDir("/sys/class/drm");
    if (!drmDir.exists())
        return;
}

void GPUInfoSet::update()
{
    qCDebug(app) << "GPUInfoSet update started";
    
    // Clear existing data
    d->m_gpuList.clear();
    
    // Try reading from different sources
    if (d->m_hasNvidia) {
        readNvidiaInfo();
    }
    
    if (d->m_hasAmd) {
        readAmdInfo();
    }
    
    if (d->m_hasIntel) {
        readIntelInfo();
    }

    if (d->m_hasNpu) {
        readNpuInfo();
    }
    
    qCDebug(app) << "GPUInfoSet update finished, found" << d->m_gpuList.size() << "GPUs";
}

} // namespace system
} // namespace core
