#ifndef SYSTEM_INFO_MODEL_H
#define SYSTEM_INFO_MODEL_H

#include <QString>
#include <QStandardItemModel>
#include <QMetaType>

class SystemInfoModel : public QObject
{
    Q_OBJECT
public:
    enum KernelLogLevel {
        kLogLevelEmergency = 0,
        kLogLevelAlert,
        kLogLevelCritical,
        kLogLevelError,
        kLogLevelWarning,
        kLogLevelNotice,
        kLogLevelInfo,
        kLogLevelDebug,

        KernelLogLevelMax = kLogLevelDebug + 1
    };
    Q_ENUM(KernelLogLevel)

    enum ClearCacheDepth {
        kNoClear = 0,
        kClearPageCache,
        kClearInodes,
        kClearAll,

        kClearCacheMax = kClearAll + 1
    };
    Q_ENUM(ClearCacheDepth)

    explicit SystemInfoModel(QObject *parent = nullptr);

    inline QString getHostName() const
    {
        return m_hostName;
    }
    inline void setHostName(const QString &hostName)
    {
        if (hostName != m_hostName) {
            m_hostName = hostName;
            Q_EMIT hostNameChanged(hostName);
        }
    }

    inline QString getOSVersion() const
    {
        return m_osVersion;
    }
    inline void setOSVersion(const QString &osVersion)
    {
        if (osVersion != m_osVersion) {
            m_osVersion = osVersion;
        }
    }

    inline QString getOSType() const
    {
        return m_osType;
    }
    inline void setOSType(const QString &osType)
    {
        if (osType != m_osType) {
            m_osType = osType;
        }
    }

    inline QString getKernelVersion() const
    {
        return m_kernelVersion;
    }
    inline void setKernelVersion(const QString &kernelVersion)
    {
        if (kernelVersion != m_kernelVersion) {
            m_kernelVersion = kernelVersion;
        }
    }

    inline QString getCPUModel() const
    {
        return m_cpuModel;
    }
    inline void setCPUModel(const QString &cpuModel)
    {
        if (cpuModel != m_cpuModel) {
            m_cpuModel = cpuModel;
        }
    }

    inline QString getMemory() const
    {
        return m_memory;
    }
    inline void setMemory(const QString &memory)
    {
        if (memory != m_memory) {
            m_memory = memory;
        }
    }

    inline QString getDiskSize() const
    {
        return m_diskSize;
    }
    inline void setDiskSize(const QString &diskSize)
    {
        if (diskSize != m_diskSize) {
            m_diskSize = diskSize;
        }
    }

    inline KernelLogLevel getKernelLogLevel() const
    {
        return m_logLevel;
    }
    inline void setKernelLogLevel(KernelLogLevel logLevel)
    {
        if (logLevel != m_logLevel) {
            m_logLevel = logLevel;
            Q_EMIT kernelLogLevelChanged(logLevel);
        }
    }

    inline ClearCacheDepth getClearCacheDepth() const
    {
        return m_clearCacheDepth;
    }
    inline void setClearCacheDepth(ClearCacheDepth depth)
    {
        if (depth != m_clearCacheDepth) {
            m_clearCacheDepth = depth;
            Q_EMIT clearCacheDepthChanged(depth);
        }
    }

    inline bool laptopModeEnabled() const
    {
        return m_laptopModeEnabled;
    }
    inline void setLaptopModeEnabled(bool enabled)
    {
        if (enabled != m_laptopModeEnabled) {
            m_laptopModeEnabled = enabled;
            Q_EMIT laptopModeEnabledChanged(enabled);
        }
    }

    inline bool coreDumpEnabled() const
    {
        return m_coreDumpEnabled;
    }
    inline void setCoreDumpEnabled(bool enabled)
    {
        if (enabled != m_coreDumpEnabled) {
            m_coreDumpEnabled = enabled;
            Q_EMIT coreDumpEnabledChanged(enabled);
        }
    }

    inline QStandardItemModel *getKernelLogLevelModel() const
    {
        return m_logLevelModel;
    }
    inline QStandardItemModel *getDropCacheModel() const
    {
        return m_dropCacheModel;
    }

Q_SIGNALS:
    void hostNameChanged(const QString &);
    void kernelLogLevelChanged(KernelLogLevel);
    void clearCacheDepthChanged(ClearCacheDepth);
    void laptopModeEnabledChanged(bool);
    void coreDumpEnabledChanged(bool);

public slots:

private:
    QString m_hostName          {};
    QString m_osVersion         {};
    QString m_osType            {};
    QString m_kernelVersion     {};
    QString m_cpuModel          {};
    QString m_memory            {};
    QString m_diskSize          {};

    KernelLogLevel  m_logLevel          {kLogLevelWarning};
    ClearCacheDepth m_clearCacheDepth   {kNoClear};

    bool m_laptopModeEnabled {true};
    bool m_coreDumpEnabled   {true};

    QStandardItemModel  *m_logLevelModel    {};
    QStandardItemModel  *m_dropCacheModel   {};
};

#endif // SYSTEM_INFO_MODEL_H
