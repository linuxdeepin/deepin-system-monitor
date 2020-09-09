/*
 * Copyright (C) 2019 ~ 2019 Union Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@uniontech.com>
 *
 * Maintainer: zccrs <zhangjide@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef STATS_COLLECTOR_H
#define STATS_COLLECTOR_H

#include <QList>
#include <QMap>
#include <QIcon>
#include <QTimer>
#include <QHash>
#include <QThread>

#include "system_stat.h"
#include "process_stat.h"
#include "process_entry.h"
#include "system_monitor.h"
#include "netif_monitor.h"
#include "desktop_entry_cache.h"

void readProcStatsCallback(ProcStat &ps, void *context);
void setProcDisplayNameAndIcon(StatsCollector &ctx, ProcessEntry &proc, const ProcStat &ps);

struct proc_net_io_stat_t {
    qulonglong  sentBytes;  // total sent bytes between interval
    qulonglong  recvBytes;  // total recv bytes between interval
    qreal       sentBps;
    qreal       recvBps;
};
struct proc_net_io_agg_t {
    qulonglong  sentBytes; // total sent
    qulonglong  recvBytes; // total recv
};
using ProcNetIOStat = QSharedPointer<struct proc_net_io_stat_t>;
using ProcNetIOAgg  = QSharedPointer<struct proc_net_io_agg_t>;

class StatsCollector : public QObject
{
    Q_OBJECT

public:
    explicit StatsCollector(QObject *parent = nullptr);
    ~StatsCollector();

Q_SIGNALS:
    void cpuStatInfoUpdated(qreal cpuPercent, const QList<qreal> cpuPercents);
    void memStatInfoUpdated(qulonglong usedMemory,
                            qulonglong totalMemory,
                            qulonglong usedSwap,
                            qulonglong totalSwap);
    void diskStatInfoUpdated(qreal readBps, qreal writeBps);
    void networkStatInfoUpdated(qulonglong totalRecvBytes,
                                qulonglong totalSentBytes,
                                qreal recvBps,
                                qreal sentBps);

    void processSummaryUpdated(int napps, int nprocs);
    void processListUpdated(const QList<ProcessEntry> procList);

    void initialSysInfoLoaded();    // simply put here to help defer initialize some of the widgets

public Q_SLOTS:
    void start();
    void updateStatus();
    void setFilterType(SystemMonitor::FilterType type);

private:
    inline qreal calcProcCPUStats(const ProcStat &prev,
                                  const ProcStat &cur,
                                  const CPUStat &prevCPU,        // prev total cpu stat
                                  const CPUStat &curCPU,         // cur total cpu stat
                                  qulonglong interval,     // interval of time in 1/100th of a second
                                  unsigned long hz);
    inline QPair<qreal, qreal> calcProcDiskIOStats(const ProcStat &prev,
                                                   const ProcStat &cur,
                                                   qulonglong interval);
    inline ProcNetIOStat calcProcNetIOStats(const ProcNetIOAgg &prev,
                                            const ProcNetIOAgg &cur,
                                            qulonglong interval);
    inline void mergeSubProcNetIO(pid_t ppid, ProcNetIOStat &sum);

private:
    enum StatIndex { kLastStat = 0, kCurrentStat = 1, kStatCount = kCurrentStat + 1 };

    SystemMonitor::FilterType m_filterType {SystemMonitor::OnlyGUI};
    uid_t m_euid;

    int m_napps {};     // current running gui apps)
    int m_nprocs {};    // current running total procs

    qreal   m_interval {2.};
    QTimer *m_timer {nullptr};

    NetifMonitor *m_netifMonitor {};

    struct stat_context m_statCtx {};

    time_t m_btime {};

    QList<pid_t> m_appList {};

    QMap<pid_t, ProcNetIOStat>  m_procNetIOStat {}; // help merge child proc's stat
    using ProcNetIOAggHist = QPair<ProcNetIOAgg, ProcNetIOAgg>;
    QMap<pid_t, ProcNetIOAggHist> m_procNetIOAgg  {};

    qulonglong  m_uptime[kStatCount]         {{}, {}};
    CPUStat     m_cpuStat[kStatCount]        {{}, {}};
    CPUStatMap  m_cpuStatMap[kStatCount]     {{}, {}};
    NetIFStat   m_ifStat[kStatCount]         {{}, {}};
    DiskIOStat  m_ioStat[kStatCount]         {{}, {}};

    QMap<pid_t, ProcStat>       m_procMap[kStatCount]   {};
    QMap<ino_t, pid_t>          m_inodeToPID            {};
    QMap<pid_t, ProcessEntry>   m_procEntryMap          {};

    QMap<pid_t, pid_t>      m_pidCtoPMapping {};
    QMultiMap<pid_t, pid_t> m_pidPtoCMapping {};

    // pid <==> pid launched from
    QMap<pid_t, pid_t>      m_gioPIDMapping     {};
    // (reverse)
    QMultiMap<pid_t, pid_t> m_gioRevPIDMapping  {};
    // pid <==> launched desktop file
    QMap<pid_t, QString>    m_gioDesktopMapping {};

    QMap<uid_t, QString>    m_uidCache              {};
    QMap<gid_t, QString>    m_gidCache              {};

    DesktopEntryCache *m_desktopEntryCache {};

    QList<QString>          m_shellList             {};
    QList<QString>          m_scriptingList         {};
    QList<QByteArray>       m_envPathList           {};

    QIcon m_defaultIcon {};

    friend void readProcStatsCallback(ProcStat &ps, void *context);
    friend void setProcDisplayNameAndIcon(StatsCollector &ctx, ProcessEntry &proc, const ProcStat &ps);
    friend class SystemMonitor;
};

#endif // STATS_COLLECTOR_H
