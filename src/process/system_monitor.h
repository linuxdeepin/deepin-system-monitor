#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <proc/readproc.h>
#include <proc/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#include <mutex>
#include <thread>

#include <DApplication>
#include <QMap>
#include <QPointF>
#include <QTimer>

#include "common/error_context.h"
#include "find_window_title.h"
#include "network_traffic_filter.h"
#include "utils.h"

using namespace Utils;
DWIDGET_USE_NAMESPACE

class ProcessEntry;

class SystemMonitor : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SystemMonitor)

public:
    typedef std::map<int, proc_t> StoredProcType;

    enum FilterType { OnlyGUI, OnlyMe, AllProcess };

    enum ProcessPriority {
        kInvalidPriority = INT_MAX,
        kVeryHighPriority = -20,  // default veryhigh priority
        kHighPriority = -5,       // default high priority
        kNormalPriority = 0,
        kLowPriority = 5,       // default low priority
        kVeryLowPriority = 19,  // default verylow priority
        kCustomPriority
    };
    static const int kVeryHighPriorityMax = -20;
    static const int kVeryHighPriorityMin = -11;
    static const int kHighPriorityMax = -10;
    static const int kHighPriorityMin = -1;
    static const int kLowPriorityMax = 1;
    static const int kLowPriorityMin = 10;
    static const int kVeryLowPriorityMax = 11;
    static const int kVeryLowPriorityMin = 19;

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

    inline static QString getPriorityName(int prio)
    {
        const static QMap<ProcessPriority, QString> priorityMap = {
            {kVeryHighPriority, DApplication::translate("Process.Priority", "Very high")},
            {kHighPriority, DApplication::translate("Process.Priority", "High")},
            {kNormalPriority, DApplication::translate("Process.Priority", "Normal")},
            {kLowPriority, DApplication::translate("Process.Priority", "Low")},
            {kVeryLowPriority, DApplication::translate("Process.Priority", "Very low")},
            {kCustomPriority, DApplication::translate("Process.Priority", "Custom")},
            {kInvalidPriority, DApplication::translate("Process.Priority", "Invalid")}};

        ProcessPriority p = kInvalidPriority;
        if (prio == kVeryHighPriority || prio == kHighPriority || prio == kNormalPriority ||
            prio == kLowPriority || prio == kVeryLowPriority) {
            p = ProcessPriority(prio);
        } else if (prio >= kVeryHighPriorityMax && prio <= kVeryLowPriorityMin) {
            p = kCustomPriority;
        }

        return priorityMap[p];
    }

    inline ProcessPriority getProcessPriorityStub(int prio) const
    {
        if (prio == 0) {
            return kNormalPriority;
        } else if (prio == kVeryHighPriority || prio == kHighPriority || prio == kLowPriority ||
                   prio == kVeryLowPriority) {
            return ProcessPriority(prio);
        } else if (prio <= kVeryLowPriorityMin && prio >= kVeryHighPriorityMax) {
            return kCustomPriority;
        } else {
            return kInvalidPriority;
        }
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
    void processPriorityChanged(pid_t pid, int priority);
    void priorityPromoteResultReady(const ErrorContext &ec);
    void processControlResultReady(const ErrorContext &ec);
    void initialSysInfoLoaded();

public Q_SLOTS:
    void updateStatus();
    void setFilterType(FilterType type);
    void endProcess(pid_t pid);
    void pauseProcess(pid_t pid);
    void resumeProcess(pid_t pid);
    void killProcess(pid_t pid);
    inline bool isSelfProcess(pid_t pid)
    {
        pid_t cur = getpid();
        return pid == cur;
    }
    ErrorContext setProcessPriority(pid_t pid, int priority);
    void sendSignalToProcess(pid_t pid, int signal);

private:
    SystemMonitor(QObject *parent = nullptr);
    ~SystemMonitor() = default;

    void getProcessDiskStatus(int pid, DiskStatus &ds);
    void mergeItemInfo(ProcessEntry &item, qreal childCpu, qulonglong childMemory,
                       const DiskStatus &childDiskStatus, const NetworkStatus &childNetworkStatus);
    void updateProcessPriority(QList<ProcessEntry> &list);

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
    QMap<int, unsigned long> m_procTotalRead {};
    QMap<int, unsigned long> m_procTotalWrite {};
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
