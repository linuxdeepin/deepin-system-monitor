#ifndef SYSTEM_CTL_H
#define SYSTEM_CTL_H

#include <mutex>

#include "model/system_info_model.h"

class SystemCtl : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SystemCtl)

public:
    inline static SystemCtl *instance()
    {
        SystemCtl *sin = m_instance.load();
        if (!sin) {
            std::lock_guard<std::mutex> lock(m_mutex);
            sin = m_instance.load();
            if (!sin) {
                sin = new SystemCtl();
                m_instance.store(sin);
            }
        }
        return sin;
    }

    void getSystemInfo();

Q_SIGNALS:
    void sysInfoModelUpdated(QSharedPointer<SystemInfoModel> model);

private:
    SystemCtl(QObject *parent = nullptr);

    SystemInfoModel::KernelLogLevel getKernelLogLevel();
    SystemInfoModel::ClearCacheDepth readSysDropCache();
    bool getSplashModeEnabledStatus();
    bool getCoreDumpEnabledStatus();

    static std::atomic<SystemCtl *> m_instance;
    static std::mutex m_mutex;
};

#endif // SYSTEM_CTL_H
