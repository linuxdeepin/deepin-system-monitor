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
#include "desktop_entry_stat.h"
#include "system_monitor.h"
#include "find_window_title.h"
#include "network_traffic_filter.h"

void readProcStatsCallback(ProcStat &ps, void *context);

struct net_io {
    qulonglong  sentBytes;
    qulonglong  recvBytes;
    qreal       sentBps;
    qreal       recvBps;
};
using NetIO = QSharedPointer<struct net_io>;

class StatsCollector : public QObject
{
    Q_OBJECT

public:
    explicit StatsCollector(QObject *parent = nullptr);

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
    qreal calcProcCPUStats(ProcStat &prev,
                           ProcStat &cur,
                           CPUStat &prevCPU,        // prev total cpu stat
                           CPUStat &curCPU,         // cur total cpu stat
                           qulonglong interval,     // interval of time in 1/100th of a second
                           unsigned long hz);
    QPair<qreal, qreal> calcProcDiskIOStats(ProcStat &prev,
                                            ProcStat &cur,
                                            qulonglong interval);
#if 0
    QPair<qreal, qreal> calcProcNetIOStats(ProcStat &prev,
                                           ProcStat &cur,
                                           qulonglong interval);
#endif
    void mergeSubProcNetIO(pid_t ppid, NetIO &sum);

private:
    enum StatIndex { kLastStat = 0, kCurrentStat = 1, kStatCount = kCurrentStat + 1 };

    SystemMonitor::FilterType m_filterType {SystemMonitor::OnlyGUI};
    uid_t m_euid;

    int m_napps {};     // current running gui apps)
    int m_nprocs {};    // current running total procs

    qreal       m_interval {2.};
    QTimer     *m_timer {nullptr};

    struct stat_context m_statCtx {};

    // helper utility to help find all gui apps
    QScopedPointer<FindWindowTitle> m_wm            {};
    QMap<pid_t, xcb_window_t>   m_trayPIDToWndMap   {};
    QList<pid_t>                m_guiPIDList        {};
    QList<pid_t>                m_appList           {};

    QMap<pid_t, NetIO>          m_procNetIO         {};

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

    DesktopEntryCache       m_desktopEntryCache     {};
    QThread                 m_cacheThread           {};
    DesktopEntryStat       *m_desktopEntryStat      {};
    QList<QString>          m_shellList             {};

    friend void readProcStatsCallback(ProcStat &ps, void *context);
    friend class SystemMonitor;
};

#endif // STATS_COLLECTOR_H
