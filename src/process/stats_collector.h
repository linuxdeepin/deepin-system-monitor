/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef STATS_COLLECTOR_H
#define STATS_COLLECTOR_H

#include "system_stat.h"
#include "process_stat.h"
#include "desktop_entry_stat.h"
#include "system_monitor.h"
#include "process_entry.h"

#include <QList>
#include <QMap>
#include <QIcon>
#include <QHash>

class NetifMonitor;
class FindWindowTitle;
class QTimer;
class QThread;

/**
 * @brief Callback function for process stat
 * @param ps Process stat info
 * @param context Stat context
 */
void readProcStatsCallback(ProcStat &ps, void *context);
/**
 * @brief Set process display name & icon
 * @param ctx Stats collector
 * @param proc Process entry
 * @param ps Process stat info
 */
void setProcDisplayNameAndIcon(StatsCollector &ctx, ProcessEntry &proc, const ProcStat &ps);

/**
 * @brief Process network io stat
 */
struct proc_net_io_stat_t {
    qulonglong  sentBytes;  // total sent bytes between interval
    qulonglong  recvBytes;  // total recv bytes between interval
    qreal sentBps; // oubout transfer speed
    qreal recvBps; // inbound transfer speed
};
/**
 * @brief Process network io summary
 */
struct proc_net_io_agg_t {
    qulonglong sentBytes; // total sent
    qulonglong recvBytes; // total recv
};
using ProcNetIOStat = QSharedPointer<struct proc_net_io_stat_t>;
using ProcNetIOAgg  = QSharedPointer<struct proc_net_io_agg_t>;

/**
 * @brief Stats collector
 */
class StatsCollector : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit StatsCollector(QObject *parent = nullptr);
    /**
     * @brief Destructor
     */
    ~StatsCollector();

Q_SIGNALS:
    /**
     * @brief CPU stat update signal
     * @param cpuPercent Total cpu usage
     * @param cpuPercents Logical cpu usage
     */
    void cpuStatInfoUpdated(qreal cpuPercent, const QList<qreal> cpuPercents);
    /**
     * @brief Memory stat update signal
     * @param usedMemory Memory used
     * @param totalMemory Total memory
     * @param usedSwap  Swap used
     * @param totalSwap Total swap
     */
    void memStatInfoUpdated(qulonglong usedMemory,
                            qulonglong totalMemory,
                            qulonglong usedSwap,
                            qulonglong totalSwap);
    /**
     * @brief Disk stat update signal
     * @param readBps Disk read speed (Bytes per second)
     * @param writeBps Disk write speed (Bytes per second)
     */
    void diskStatInfoUpdated(qreal readBps, qreal writeBps);
    /**
     * @brief Network stat update signal
     * @param totalRecvBytes Network total received bytes
     * @param totalSentBytes Network total sent bytes
     * @param recvBps Network inbound transfer speed (Bytes per second)
     * @param sentBps Network outbound transfer speed  (Bytes per second)
     */
    void networkStatInfoUpdated(qulonglong totalRecvBytes,
                                qulonglong totalSentBytes,
                                qreal recvBps,
                                qreal sentBps);

    /**
     * @brief Process summary update signal
     * @param napps Number of apps running
     * @param nprocs Total number of processes running
     */
    void processSummaryUpdated(int napps, int nprocs);
    /**
     * @brief Process entry list update signal
     * @param procList Process entry list
     */
    void processListUpdated(const QList<ProcessEntry> procList);

    /**
     * @brief Initial system info loaded signal
     */
    void initialSysInfoLoaded();    // simply put here to help defer initialize some of the widgets

public Q_SLOTS:
    /**
     * @brief start Start stat collector job
     */
    void start();
    /**
     * @brief updateStatus Update status
     */
    void updateStatus();
    /**
     * @brief setFilterType Set process filter type
     * @param type Filter type
     */
    void setFilterType(SystemMonitor::FilterType type);

private:
    /**
     * @brief Calculate process cpu usage
     * @param prev Previous stat
     * @param cur Current stat
     * @param prevCPU Previous cpu stat
     * @param curCPU Current cpu stat
     * @param interval Interval between 2 stats within 1/100th a second
     * @param hz Clock ticks
     * @return CPU usage
     */
    inline qreal calcProcCPUStats(const ProcStat &prev,
                                  const ProcStat &cur,
                                  const CPUStat &prevCPU,
                                  const CPUStat &curCPU,
                                  qulonglong interval,
                                  unsigned long hz);
    /**
     * @brief Calculate process disk io usage
     * @param prev Previous stat
     * @param cur Current stat
     * @param interval Interval between 2 stats
     * @return Disk <read, write> usage
     */
    inline QPair<qreal, qreal> calcProcDiskIOStats(const ProcStat &prev,
                                                   const ProcStat &cur,
                                                   qulonglong interval);
    /**
     * @brief Calculate process network io usage
     * @param prev Previous stat
     * @param cur Current stat
     * @param interval Interval between 2 stats
     * @return Network io stat
     */
    inline ProcNetIOStat calcProcNetIOStats(const ProcNetIOAgg &prev,
                                            const ProcNetIOAgg &cur,
                                            qulonglong interval);
    /**
     * @brief Merge child process's network io into parent's
     * @param ppid Parent pid
     * @param sum Merged network io stat
     */
    inline void mergeSubProcNetIO(pid_t ppid, ProcNetIOStat &sum);

    inline void mergeSubProcCpu(pid_t ppid, qreal &cpu);

private:
    /**
     * @brief Stat index enum
     */
    enum StatIndex { kLastStat = 0, kCurrentStat = 1, kStatCount = kCurrentStat + 1 };

    // filter type
    int m_filterType = -1;
    // effective user id
    uid_t m_euid;

    int m_napps {}; // number of running apps
    int m_nprocs {}; // total number of running processes

    qreal m_interval {2.}; // interval between stats
    QTimer *m_timer {nullptr}; // timer to update stat periodically

    NetifMonitor *m_netifMonitor {}; // netif monitor instance

    struct stat_context m_statCtx {
    }; // stat context

    time_t m_btime {}; // elapsed time since system boot

    // helper utility to help find all gui apps
    QScopedPointer<FindWindowTitle> m_wm {}; // find window title instance
    QMap<pid_t, xcb_window_t> m_trayPIDToWndMap {}; // pid:wnd mapping of tray processes
    QList<pid_t> m_guiPIDList {}; // list of gui pids
    QList<pid_t> m_appList {}; // list of apps pids

    QMap<pid_t, ProcNetIOStat> m_procNetIOStat {}; // pid:stat mapping, network io stat of process
    using ProcNetIOAggHist = QPair<ProcNetIOAgg, ProcNetIOAgg>;
    QMap<pid_t, ProcNetIOAggHist> m_procNetIOAgg {}; // pid:agg mapping, accumulatted network io of process

    qulonglong m_uptime[kStatCount] {{}, {}}; // system uptime
    CPUStat m_cpuStat[kStatCount] {{}, {}}; // cpu stats
    CPUStatMap m_cpuStatMap[kStatCount] {{}, {}}; // logical cpu stats
    NetIFStat m_ifStat[kStatCount] {{}, {}}; // netif stats
    DiskIOStat m_ioStat[kStatCount] {{}, {}}; // disk io stats

    QMap<pid_t, ProcStat> m_procMap[kStatCount] {}; // pid to process stat mapping
    QMap<pid_t, ProcessEntry> m_procEntryMap {}; // pid to process entry mapping
    QList<pid_t> m_pidList {}; // list of pids

    QMap<pid_t, pid_t> m_pidCtoPMapping {}; // child to parent pid mapping
    QMultiMap<pid_t, pid_t> m_pidPtoCMapping {}; // parent to child pid mapping

    // pid <==> pid launched from
    QMap<pid_t, pid_t>      m_gioPIDMapping     {};
    // (reverse)
    QMultiMap<pid_t, pid_t> m_gioRevPIDMapping  {};
    // pid <==> launched desktop file
    QMap<pid_t, QString>    m_gioDesktopMapping {};

    QMap<uid_t, QString> m_uidCache {}; // uid to name mapping
    QMap<gid_t, QString> m_gidCache {}; // gid to name mapping

    DesktopEntryCache m_desktopEntryCache {}; // desktop entry cache
    QThread m_cacheThread {}; // thread to update desktop entry cache
    DesktopEntryStat *m_desktopEntryStat {}; // desktop entry stat manager
    QList<QString> m_shellList {}; // list of shell commands
    QList<QString> m_scriptingList {}; // list of script commands
    QList<QByteArray> m_envPathList {}; // list of environment paths

    QIcon m_defaultIcon {}; // default icon for processes if no desktop icon defined

    friend void readProcStatsCallback(ProcStat &ps, void *context);
    friend void setProcDisplayNameAndIcon(StatsCollector &ctx, ProcessEntry &proc, const ProcStat &ps);
    friend class SystemMonitor;
};

#endif // STATS_COLLECTOR_H
