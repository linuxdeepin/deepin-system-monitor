#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <proc/readproc.h>
#include <proc/sysinfo.h>
#include <mutex>
#include <thread>

#include <QMap>
#include <QPointF>
#include <QTimer>

#include "find_window_title.h"
#include "network_traffic_filter.h"
#include "utils.h"

using namespace Utils;

class ProcessEntry;

class SystemMonitor : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SystemMonitor)

public:
    typedef std::map<int, proc_t> StoredProcType;

    enum FilterType { OnlyGUI, OnlyMe, AllProcess };

    struct ChildPidInfo {
        qreal cpu;
        qulonglong memory;
        DiskStatus diskStatus;
        NetworkStatus networkStatus;
    };

    inline static SystemMonitor *instance()
    {
        SystemMonitor *sin = m_instance.load();
        if (!sin) {
            std::lock_guard<std::mutex> lock(m_mutex);
            sin = m_instance.load();
            if (!sin) {
                sin = new SystemMonitor();
                m_instance.store(sin);
            }
        }
        return sin;
    }

Q_SIGNALS:
    void cpuStatInfoUpdated(qreal cpuPercent, const QVector<qreal> &cpuPercents);
    void memStatInfoUpdated(qulonglong usedMemory, qulonglong totalMemory, qulonglong usedSwap,
                            qulonglong totalSwap);
    void diskStatInfoUpdated(qreal totalReadKbs, qreal totalWriteKbs);
    void networkStatInfoUpdated(qulonglong totalRecvBytes, qulonglong totalSentBytes,
                                qreal totalRecvKbs, qreal totalSentKbs);
    void processSummaryUpdated(int napps, int nprocs);
    void processListUpdated(const QList<ProcessEntry> &procList);
    void processEnded(pid_t pid);
    void processPaused(pid_t pid, char state);
    void processResumed(pid_t pid, char state);
    void processKilled(pid_t pid);

public Q_SLOTS:
    void updateStatus();
    void setFilterType(FilterType type);
    void endProcess(pid_t pid);
    void pauseProcess(pid_t pid);
    void resumeProcess(pid_t pid);
    void killProcess(pid_t pid);

private:
    SystemMonitor(QObject *parent = nullptr);
    ~SystemMonitor() = default;

    DiskStatus getProcessDiskStatus(int pid);
    void mergeItemInfo(ProcessEntry &item, qreal childCpu, qulonglong childMemory,
                       const DiskStatus &childDiskStatus, const NetworkStatus &childNetworkStatus);

private:
    FilterType filterType {OnlyGUI};

    int updateDuration {2000};
    qreal updateSeconds {0.0};
    QTimer *m_updateStatusTimer;

    QString m_euname;  // effective user name

    QScopedPointer<FindWindowTitle> m_findWindowTitle;

    QMap<QString, int> m_wineApplicationDesktopMaps {};
    QMap<int, QString> m_wineServerDesktopMaps {};
    QMap<int, double> m_processCpuPercents {};
    QMap<int, long> m_processRecvBytes {};
    QMap<int, long> m_processSentBytes {};
    QMap<int, unsigned long> m_processReadKbs {};
    QMap<int, unsigned long> m_processWriteKbs {};
    StoredProcType m_prevProcesses {};
    qulonglong prevTotalRecvBytes {};
    qulonglong prevTotalSentBytes {};
    qulonglong totalRecvBytes {};
    qulonglong totalSentBytes {};
    qulonglong prevTotalCpuTime {};
    qulonglong prevWorkCpuTime {};
    qulonglong totalCpuTime {};
    qulonglong workCpuTime {};

    QVector<CpuStruct> prevCpuTimes {};

    static std::atomic<SystemMonitor *> m_instance;
    static std::mutex m_mutex;
};

#endif  // SYSTEM_MONITOR_H
