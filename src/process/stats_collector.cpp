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

#include "stats_collector.h"

#include "application.h"
#include "find_window_title.h"
#include "netif_monitor.h"
#include "process_entry.h"
#include "utils.h"

#include <DDesktopEntry>

#include <QDebug>
#include <QPixmap>
#include <QDateTime>
#include <QUrl>
#include <QTimer>
#include <QThread>
#include <QApplication>
#include <QMutableMapIterator>
#include "../common/whitelistconfig.h"

// disk sector size shift
#define SECTOR_SHIFT 9
// sector size (512 bytes)
#define SECTOR_SIZE (1 << SECTOR_SHIFT)

// unit in milliseconds
static const int kUpdateInterval = 2000;

// calculate cpu usage
auto calcCPUUsage = [](const CPUStat &prev, const CPUStat &cur) -> qreal
{
    qulonglong cur_tot {}, prev_tot {}, cur_idle {}, prev_idle {};

    if (!cur.isNull()) {
        // current total
        cur_tot = cur->user + // user time
                  cur->nice + // user time with low priority
                  cur->sys + // system time
                  cur->idle + // idle time
                  cur->iowait + // io wait time
                  cur->hardirq + // interrupt time
                  cur->softirq + // soft interrupt time
                  cur->steal; // stolen time
        // current idle
        cur_idle = cur->idle + cur->iowait; // idle time + io wait time
    }

    if (!prev.isNull()) {
        // previous total
        prev_tot = prev->user + // user time
                   prev->nice + // user time with low priority
                   prev->sys + // system time
                   prev->idle + // idle time
                   prev->iowait + // io wait time
                   prev->hardirq + // interrupt time
                   prev->softirq + // soft interrupt time
                   prev->steal; // stolen time
        // previous idle
        prev_idle = prev->idle + prev->iowait; // idle time + io wait time
    }

    // total time increment
    auto totald = (cur_tot > prev_tot) ? (cur_tot - prev_tot) : 0;
    // total idle time increment
    auto idled = (cur_idle > prev_idle) ? (cur_idle - prev_idle) : 0;

    // cpu usage
    auto cpuPecent = (totald - idled) * 1. / totald;
    return cpuPecent;
};

// constructor
StatsCollector::StatsCollector(QObject *parent) :
    QObject(parent),
    m_wm(new FindWindowTitle{})
{
    qRegisterMetaType<QList<ProcessEntry>>("ProcessEntryList");
    qRegisterMetaType<QHash<QString, DesktopEntry>>("DesktopEntryCache");
    m_timer = nullptr;
    m_cpuStat[kLastStat] = CPUStat(new cpu_stat {});
    m_cpuStat[kCurrentStat] = CPUStat(new cpu_stat {});

    // fill shell list
    [ = ] {
        FILE *fp;
        fp = fopen("/etc/shells", "r");
        if (fp)
        {
            char buf[128] {};
            char *s;
            while ((s = fgets(buf, 128, fp))) {
                if (s[0] == '/') {
                    auto sh = QLatin1String(basename(s));
                    if (sh.endsWith('\n'))
                        sh.chop(1);
                    if (!m_shellList.contains(sh)) {
                        m_shellList << sh;
                    }
                }
            }
        }
    }();

    // fill scripting lang list (!!far from complete)
    m_scriptingList << "/usr/bin/python";
    m_scriptingList << "/usr/bin/perl";
    m_scriptingList << "/usr/bin/php";

    // fill environment path
    auto binPaths = qgetenv("PATH");
    auto pathList = binPaths.split(':');
    if (pathList.size() > 0) {
        for (auto p : pathList) {
            m_envPathList << p;
        }
    } else {
        // use default path
        m_envPathList << "/usr/bin";
    }
}

// destructor
StatsCollector::~StatsCollector()
{
    // quit network interface monitor instance
    m_netifMonitor->requestQuit();
    m_netifMonitor->quit();
    m_netifMonitor->wait();
}

// start stat job
void StatsCollector::start()
{
    if (m_timer) {
        delete m_timer;
        m_timer = nullptr;
    }
    // Start timer.
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateStatus()));

    // get effective user id
    m_euid = geteuid();

    // get default icon from theme
    m_defaultIcon = QIcon::fromTheme("application-x-executable");

    // prepare desktop entry cache
    m_desktopEntryStat = new DesktopEntryStat();
    m_desktopEntryStat->moveToThread(&m_cacheThread);
    // start desktop entry stat job when thread started
    connect(&m_cacheThread, &QThread::started, m_desktopEntryStat, &DesktopEntryStat::start);
    // delete desktop entry stat instance when thread finished
    connect(&m_cacheThread, &QThread::finished, m_desktopEntryStat, &QObject::deleteLater);
    // start stat collector job when desktop entry cache updated
    connect(m_desktopEntryStat, &DesktopEntryStat::cacheUpdated, this, [ = ](DesktopEntryCache cache) {
        m_desktopEntryCache = cache;
        if (!m_timer->isActive()) {
            updateStatus();
            m_timer->start(kUpdateInterval);
        }
    });
    // start desktop entry cache thread
    m_cacheThread.start();

    // start netif monitor instance
    m_netifMonitor = new NetifMonitor(this);
    connect(m_netifMonitor, &QThread::finished, m_netifMonitor, &QObject::deleteLater);
    m_netifMonitor->start();
}

// process stat callback
void readProcStatsCallback(ProcStat &ps, void *context)
{
    if (!context)
        return;

    // get stat collector instance
    auto *ctx = static_cast<StatsCollector *>(context);
    if (!ctx)
        return;

    if (ps) {
        // append current pid to pid list
        ctx->m_pidList << ps->pid;
        // cache current process stat info
        ctx->m_procMap[StatsCollector::kCurrentStat][ps->pid] = ps;

        // make pid & parent pid mapping
        ctx->m_pidCtoPMapping.insert(ps->pid, ps->ppid);
        ctx->m_pidPtoCMapping.insert(ps->ppid, ps->pid);

        // cache gio desktop file info
        if (ps->environ.contains("GIO_LAUNCHED_DESKTOP_FILE")) {
            ctx->m_gioDesktopMapping[ps->pid] = ps->environ["GIO_LAUNCHED_DESKTOP_FILE"];
        }
        // cache gio desktop pid info
        if (ps->environ.contains("GIO_LAUNCHED_DESKTOP_FILE_PID")) {
            bool ok = false;
            auto pid = ps->environ["GIO_LAUNCHED_DESKTOP_FILE_PID"].toInt(&ok);
            if (ok) {
                ctx->m_gioPIDMapping.insert(ps->pid, pid);
                ctx->m_gioRevPIDMapping.insert(pid, ps->pid);
            }
        }

        ProcessEntry proc {};

        // set pid
        proc.setPID(ps->pid);
        // set uid
        proc.setUID(ps->uid);
        // set gid
        proc.setGID(ps->gid);
        // set priority
        proc.setPriority(ps->nice);
        // set state
        proc.setState(ps->state);

        // current uptime
        auto curUptm = ctx->m_uptime[StatsCollector::kCurrentStat];
        // last uptime
        auto lastUptm = ctx->m_uptime[StatsCollector::kLastStat];
        // interval between stats
        qulonglong interval = (curUptm > lastUptm) ? (curUptm - lastUptm) : 1;

        // set cpu usage
        proc.setCPU(ctx->calcProcCPUStats(
                        ctx->m_procMap[StatsCollector::kLastStat][ps->pid],
                        ctx->m_procMap[StatsCollector::kCurrentStat][ps->pid],
                        ctx->m_cpuStat[StatsCollector::kLastStat],
                        ctx->m_cpuStat[StatsCollector::kCurrentStat],
                        interval,
                        ctx->m_statCtx.hz));
        // set memory
        proc.setMemory(ps->rss - ps->shm);

        // process name
        proc.setName(Utils::normalizeProcName(ps->proc_name, ps->cmdline));

        // set displayName & icon
        setProcDisplayNameAndIcon(*ctx, proc, ps);

        // set cmdline
        proc.setCmdline(QUrl::fromPercentEncoding(ps->cmdline.join(' ')));
        // set start time of process
        proc.setStartTime(ctx->m_btime + time_t(ps->start_time / ctx->m_statCtx.hz));

        // set user name
        if (ctx->m_uidCache.contains(ps->euid)) {
            proc.setUserName(ctx->m_uidCache[ps->euid]);
        } else {
            QString euname = SystemStat::getUserName(ps->euid);
            ctx->m_uidCache[ps->euid] = euname;
            proc.setUserName(euname);
        }

        // calculate disk io usage
        auto rwbps = ctx->calcProcDiskIOStats(
                         ctx->m_procMap[StatsCollector::kLastStat][ps->pid],
                         ctx->m_procMap[StatsCollector::kCurrentStat][ps->pid],
                         interval);
        // set read speed
        proc.setReadBps(rwbps.first);
        // set write speed
        proc.setWriteBps(rwbps.second);

        // process net io stat
        // backup last stat
        if (ctx->m_procNetIOAgg.contains(ps->pid)) {
            ctx->m_procNetIOAgg[ps->pid].first = ctx->m_procNetIOAgg[ps->pid].second;
        } else {
            auto stub = QSharedPointer<struct proc_net_io_agg_t>::create();
            ctx->m_procNetIOAgg[ps->pid].first = stub;
            ctx->m_procNetIOAgg[ps->pid].second = stub;
        }

        // accumulated network io usage
        auto curSockIOAgg = QSharedPointer<struct proc_net_io_agg_t>::create();
        curSockIOAgg->recvBytes = ctx->m_procNetIOAgg[ps->pid].second->recvBytes;
        curSockIOAgg->sentBytes = ctx->m_procNetIOAgg[ps->pid].second->sentBytes;
        // sum up current io usage (each inode's io)
        for (auto ino : ps->sockInodes) {
            SockIOStat sockIOStat;
            bool ok = ctx->m_netifMonitor->getSockIOStatByInode(ino, sockIOStat);
            if (ok) {
                curSockIOAgg->recvBytes += sockIOStat->rx_bytes;
                curSockIOAgg->sentBytes += sockIOStat->tx_bytes;
            }
        }
        // cache updated io usage as current
        ctx->m_procNetIOAgg[ps->pid].second = curSockIOAgg;

        // calculate current process's io usage
        auto procNetIOStat = ctx->calcProcNetIOStats(ctx->m_procNetIOAgg[ps->pid].first,
                                                     ctx->m_procNetIOAgg[ps->pid].second,
                                                     interval);
        ctx->m_procNetIOStat[ps->pid] = procNetIOStat;
        // set sent bytes
        proc.setSentBytes(procNetIOStat->sentBytes);
        // set recv bytes
        proc.setRecvBytes(procNetIOStat->recvBytes);
        // set sent speed
        proc.setSentBps(procNetIOStat->sentBps);
        // set recv speed
        proc.setRecvBps(procNetIOStat->recvBps);

        // cache process entry
        ctx->m_procEntryMap[ps->pid] = proc;
    }
}

// update status routine
void StatsCollector::updateStatus()
{
    bool b;
    qulonglong uptime {}; // uptime
    CPUStat cpuStat {}; // cpu stat
    CPUStatMap cpuStatMap {}; // logical cpu stats
    MemStat memStat {}; // memory stat
    DiskIOStat iostat {}; // block dev stat sum
    DiskIOStatMap iostatMap {}; // block dev stat
    NetIFStat ifstat {}; // netif stat sum
    NetIFStatMap ifstatMap {}; // netif stat

    m_napps = m_nprocs = 0;

    // get stat context
    ProcessStat::getStatContext(m_statCtx);

    if (!m_btime) {
        b = SystemStat::readBootTime(m_btime);
    }

    // read system uptime
    b = SystemStat::readUpTime(uptime);
    if (b) {
        m_uptime[kLastStat] = m_uptime[kCurrentStat];
        m_uptime[kCurrentStat] = uptime;
        m_interval = (m_uptime[kCurrentStat] - m_uptime[kLastStat]) / 100.;
        if (m_interval <= 1) {
            m_interval = 2.;
        }
    }

    // get cpu stats
    b = SystemStat::readCPUStats(cpuStat, cpuStatMap);
    if (b) {
        m_cpuStat[kLastStat] = m_cpuStat[kCurrentStat];
        m_cpuStat[kCurrentStat] = cpuStat;
        m_cpuStatMap[kLastStat] = m_cpuStatMap[kCurrentStat];
        m_cpuStatMap[kCurrentStat] = cpuStatMap;

        // total usage
        auto cpuPecent = calcCPUUsage(m_cpuStat[kLastStat], m_cpuStat[kCurrentStat]);

        QList<qreal> cpuPecents;
        // each logical cpu usage
        for (auto i = 0; i < cpuStatMap.size(); i++) {
            auto &cur = m_cpuStatMap[kCurrentStat][i];
            auto &prev = m_cpuStatMap[kLastStat][i];
            auto cpp = calcCPUUsage(prev, cur);
            cpuPecents << cpp;
        }

        // emit cpu stat update signal
        Q_EMIT cpuStatInfoUpdated(cpuPecent * 100., cpuPecents);
    }

    // get memory & swap stat
    b = SystemStat::readMemStats(memStat);
    if (b) {
        // emit memory stat update signal
        if (memStat->swap_total_kb > 0) {
            // swap enabled case
            Q_EMIT memStatInfoUpdated(
                memStat->mem_total_kb - memStat->mem_avail_kb,
                memStat->mem_total_kb,
                memStat->swap_total_kb - memStat->swap_free_kb,
                memStat->swap_total_kb);
        } else {
            // swap disabled case
            Q_EMIT memStatInfoUpdated(
                memStat->mem_total_kb - memStat->mem_avail_kb,
                memStat->mem_total_kb,
                0,
                0);
        }
    }

    // get disk io usage
    b = SystemStat::readDiskIOStats(iostat, iostatMap);
    if (b) {
        m_ioStat[kLastStat] = m_ioStat[kCurrentStat];
        m_ioStat[kCurrentStat] = iostat;

        qulonglong cr {}, cw {}, cd {}, pr {}, pw {}, pd {};
        if (!m_ioStat[kCurrentStat].isNull()) {
            // current
            cr = m_ioStat[kCurrentStat]->read_sectors;
            cw = m_ioStat[kCurrentStat]->write_sectors;
            cd = m_ioStat[kCurrentStat]->discard_sectors;
        }
        if (!m_ioStat[kLastStat].isNull()) {
            // previous
            pr = m_ioStat[kLastStat]->read_sectors;
            pw = m_ioStat[kLastStat]->write_sectors;
            pd = m_ioStat[kLastStat]->discard_sectors;
        }

        // read increment between interval
        auto rdiff = (cr > pr) ? (cr - pr) : 0;
        // write increment between interval
        auto wdiff = (cw > pw) ? (cw - pw) : 0;
        // discarded increment between interval
        auto ddiff = (cd > pd) ? (cd - pd) : 0;

        // calculate actual size
        auto rsize = rdiff * SECTOR_SIZE;
        auto wsize = (wdiff + ddiff) * SECTOR_SIZE;

        // emit disk stat update signal
        Q_EMIT diskStatInfoUpdated(rsize / m_interval, wsize / m_interval);
    }

    // get network interface stat
    b = SystemStat::readNetIfStats(ifstat, ifstatMap);
    if (b) {
        m_ifStat[kLastStat] = m_ifStat[kCurrentStat];
        m_ifStat[kCurrentStat] = ifstat;

        qulonglong prxb {}, ptxb{}, crxb {}, ctxb {};
        if (!m_ifStat[kCurrentStat].isNull()) {
            crxb = m_ifStat[kCurrentStat]->rx_bytes;
            ctxb = m_ifStat[kCurrentStat]->tx_bytes;
        }
        if (!m_ifStat[kLastStat].isNull()) {
            prxb = m_ifStat[kLastStat]->rx_bytes;
            ptxb = m_ifStat[kLastStat]->tx_bytes;
        }

        // receive increment between interval
        auto rxdiff = (crxb > prxb) ? (crxb - prxb) : 0;
        // transfer increment between interval
        auto txdiff = (ctxb > ptxb) ? (ctxb - ptxb) : 0;

        // emit network stat signal
        Q_EMIT networkStatInfoUpdated(
            crxb,
            ctxb,
            rxdiff / m_interval,   // Bps
            txdiff / m_interval);  // Bps
    }

    // clear id/name map cache in every cycle in case user change uid/gid on the fly
    m_uidCache.clear();
    m_gidCache.clear();

    m_procMap[kLastStat] = m_procMap[kCurrentStat];
    m_procEntryMap.clear();
    m_pidList.clear();

    m_pidCtoPMapping.clear();
    m_pidPtoCMapping.clear();

    m_gioPIDMapping.clear();
    m_gioDesktopMapping.clear();
    m_gioRevPIDMapping.clear();

    m_trayPIDToWndMap.clear();
    m_guiPIDList.clear();
    m_appList.clear();

    m_procNetIOStat.clear();

    m_wm->updateWindowInfos();
    m_guiPIDList = m_wm->getWindowPids();

    // get processes which has tray windows
    auto trayProcessXids = Utils::getTrayWindows();
    for (auto xid : trayProcessXids) {
        auto pid = m_wm->getWindowPid(xid);
        m_trayPIDToWndMap[pid] = xid;
    }

    // m_trayPIDToWndMap & m_guiPIDList diff
    QMutableListIterator<pid_t> diffIter(m_guiPIDList);
    while (diffIter.hasNext()) {
        diffIter.next();
        if (m_trayPIDToWndMap.contains(diffIter.value()))
            diffIter.remove();
    }

    // get process info
    b = ProcessStat::readProcStats(readProcStatsCallback, this);

    /// clean process maps

    // remove no more exists process
    QMutableMapIterator<pid_t, ProcStat> procIter(m_procMap[kCurrentStat]);
    while (procIter.hasNext()) {
        procIter.next();
        if (!m_pidList.contains(procIter.key())) {
            procIter.remove();
        }
    }

    // and their traffic
    QMutableMapIterator<pid_t, ProcNetIOStat> netIOStatIter(m_procNetIOStat);
    while (netIOStatIter.hasNext()) {
        netIOStatIter.next();
        if (!m_pidList.contains(netIOStatIter.key())) {
            netIOStatIter.remove();
        }
    }
    QMutableMapIterator<pid_t, ProcNetIOAggHist> netIOAggIter(m_procNetIOAgg);
    while (netIOAggIter.hasNext()) {
        netIOAggIter.next();
        if (!m_pidList.contains(netIOAggIter.key())) {
            netIOAggIter.remove();
        }
    }

    // filter duplicate processes with same name (eg: dman, uos browser like apps with chrome engine biltin)
    // within appList if current display mode is onlyGUI
    decltype(m_appList) filteredAppList {};
    std::function<bool(pid_t ppid)> anyRootIsGuiProc;
    // find if any ancestor processes is gui application
    anyRootIsGuiProc = [&](pid_t ppid) -> bool {
        bool b;
        b = m_guiPIDList.contains(ppid) || m_trayPIDToWndMap.contains(ppid);
        if (!b && m_pidCtoPMapping.contains(ppid))
        {
            b = anyRootIsGuiProc(m_pidCtoPMapping[ppid]);
        }
        return b;
    };
    for (auto app : m_appList) {
        if (m_guiPIDList.contains(app) || m_trayPIDToWndMap.contains(app))
            continue;

        // check if cmd is a shell or scripting language
        auto isCmdInList = [ = ](QByteArray cmd) {
            bool bb = false;

            auto subCmd = cmd.mid(cmd.lastIndexOf('/') + 1);
            for (auto s : m_shellList) {
                if (subCmd.startsWith(s.toLocal8Bit())) {
                    bb = true;
                    return bb;
                }
            }
            for (auto s : m_scriptingList) {
                if (cmd.startsWith(s.toLocal8Bit())) {
                    bb = true;
                    return bb;
                }
            }
            return bb;
        };
        // get cmdline of current process from cache
        auto cmd = m_procMap[kCurrentStat][app]->cmdline[0];
        bool bb = false;
        if (cmd[0] == '/') {
            // cmd starts with full path
            bb = isCmdInList(cmd);
        } else {
            // cmd starts with raw name
            for (auto p : m_envPathList) {
                p = p.append('/').append(cmd); // e.g. /usr/bin/xxx2.7

                bb = isCmdInList(p);
                if (bb) {
                    break;
                }
            }
        }
        if (bb) {
            continue;
        }

        // only if no ancestor process is gui app we keep this process
        if (m_pidCtoPMapping.contains(app) &&
                anyRootIsGuiProc(m_pidCtoPMapping[app])) {
            continue;
        }

        filteredAppList << app;
    }
    int count = 0;
    // check filterType
    QList<ProcessEntry> filteredList{};
    for (auto &pe : m_procEntryMap.values()) {
        ++m_nprocs;

        auto pid = pe.getPID();
        auto uid = pe.getUID();
        bool need = false;
        if (m_filterType == SystemMonitor::AllProcess) {
            need = true;
        } else if (m_filterType == SystemMonitor::OnlyMe && m_euid == uid) {
            need = true;
        } else if (m_filterType == SystemMonitor::OnlyGUI
                   //                   && m_euid == uid
                   && (m_trayPIDToWndMap.contains(pid)
                       || m_guiPIDList.contains(pid)
                       || filteredAppList.contains(pid))) {
            need = true;

            // =================================================================
            // transfer all children(eg: chrome...) & wineserver.real(winapps) traffic into this process
            // =================================================================
            if (m_gioPIDMapping.contains(pid) && !m_trayPIDToWndMap.contains(pid)) {
                // traffic from wineserver.real
                auto mpid = m_gioPIDMapping[pid];
                auto plist = m_gioRevPIDMapping.values(mpid);
                for (auto p : plist) {
                    auto xpe = m_procEntryMap[p];
                    if (xpe.getName().contains("wineserver.real")) {
                        pe.setRecvBps(pe.getRecvBps() + xpe.getRecvBps());
                        pe.setRecvBytes(pe.getRecvBytes() + xpe.getRecvBytes());
                        pe.setSentBps(pe.getSentBps() + xpe.getSentBps());
                        pe.setSentBytes(pe.getSentBytes() + xpe.getSentBytes());

                        xpe.setRecvBps(0);
                        xpe.setRecvBytes(0);
                        xpe.setSentBps(0);
                        xpe.setSentBytes(0);
                    }
                }
            } else {
                // traffic from children
                ProcNetIOStat sumIO = QSharedPointer<struct proc_net_io_stat_t>::create();

                mergeSubProcNetIO(pid, sumIO);

                pe.setRecvBps(sumIO->recvBps);
                pe.setRecvBytes(sumIO->recvBytes);
                pe.setSentBps(sumIO->sentBps);
                pe.setSentBytes(sumIO->sentBytes);
            }
        }
        // 判断是否要在进程列表中显示
        if (need && ((m_guiPIDList.contains(pe.getPID()) && WhiteListConfig::noShowProcess(pe.getName())) || WhiteListConfig::isHpidPrcoress(pe.getPID(), m_pidCtoPMapping))) { // 判断是否是GUI进程
            need = false;
            count++; //  要减去不显示的进程数量
        }
        if (need) {
            filteredList << pe;
        }
    }

    if (b && filteredList.size() > 0) {
        // emit process list update signal
        Q_EMIT processListUpdated(filteredList);
    }
    if (m_procEntryMap.size() > 0) {
        m_napps = m_guiPIDList.size() + filteredAppList.size() + m_trayPIDToWndMap.size() - count;
        // emit process summary update signal
        Q_EMIT processSummaryUpdated(m_napps, m_nprocs - m_napps);
    }
}

// set process filter type
void StatsCollector::setFilterType(SystemMonitor::FilterType type)
{
    if (m_filterType != type) {
        m_filterType = type;
        m_timer->stop();
        updateStatus();
        m_timer->start(kUpdateInterval);
    }
}

// calculate process cpu usage
qreal StatsCollector::calcProcCPUStats(const ProcStat &prev,
                                       const ProcStat &cur,
                                       const CPUStat &prevCPU,
                                       const CPUStat &curCPU,
                                       qulonglong interval,
                                       unsigned long hz)
{
    qulonglong cur_jiff {}, prev_jiff {};
    if (!curCPU.isNull()) {
        // total current cpu time in jiff
        cur_jiff = curCPU->user + // user time
                   curCPU->nice + // user time with low priority
                   curCPU->sys + // system time
                   curCPU->idle + // idle time
                   curCPU->iowait + // io wait time
                   curCPU->hardirq + // interrupt time
                   curCPU->softirq + // soft interrupt time
                   curCPU->steal; // stolen time
    }

    if (!prevCPU.isNull()) {
        // total previous cpu time in jiff
        prev_jiff = prevCPU->user + // user time
                    prevCPU->nice + // user time with low priority
                    prevCPU->sys + // system time
                    prevCPU->idle + // idle time
                    prevCPU->iowait + // io wait time
                    prevCPU->hardirq + // interrupt time
                    prevCPU->softirq + // soft interrupt time
                    prevCPU->steal; // stolen time
    }

    // increment between stats
    auto delta = cur_jiff - prev_jiff;
    if (delta <= 0) {
        delta = 1;
    }
    // current kernel time + user time used by this process
    qulonglong ctm = cur.isNull() ? 0 : (cur->stime + cur->utime);
    // previous kernel time + user time used by this process
    qulonglong ptm = prev.isNull() ? 0 : (prev->stime + prev->utime);

    // TODO: better use a global flag to control cpu% display mode
#ifdef USE_IRIX_MODE
    return qreal(ctm < ptm ? 0 : (ctm - ptm)) / (interval * hz / 100) * 100;
#else
    Q_UNUSED(interval)
    Q_UNUSED(hz)
    return qreal(ctm < ptm ? 0 : (ctm - ptm)) / delta * 100;
#endif
}

// calculate process disk usage
QPair<qreal, qreal> StatsCollector::calcProcDiskIOStats(const ProcStat &prev,
                                                        const ProcStat &cur,
                                                        qulonglong interval)
{
    // read speed, write speed
    qreal rdio {}, wrio {};
    // current read/write/cancelled, previous read/write/cancelled
    qulonglong crb{}, cwb{}, ccwb{}, prb{}, pwb{}, pcwb{};

    if (!prev.isNull()) {
        // previous read bytes, write bytes, cancelled write bytes
        prb = prev->read_bytes;
        pwb = prev->write_bytes;
        pcwb = prev->cancelled_write_bytes;
    }
    if (!cur.isNull()) {
        // current read bytes, write bytes, cancelled write bytes
        crb = cur->read_bytes;
        cwb = cur->write_bytes;
        ccwb = cur->cancelled_write_bytes;
    }

    // read/write/cancelled write increments between stats
    auto rdiff = (crb < prb) ? 0 : (crb - prb);
    auto wdiff = (cwb < pwb) ? 0 : (cwb - pwb);
    auto cwdiff = (ccwb < pcwb) ? 0 : (ccwb - pcwb);
    // calculate read/write speed
    rdio = qreal(rdiff) / interval * 100;
    wrio = qreal(wdiff - cwdiff) / interval * 100;

    return {rdio, wrio};
}

// set process display name & icon
void setProcDisplayNameAndIcon(StatsCollector &ctx, ProcessEntry &proc, const ProcStat &ps)
{
    DesktopEntry de {};
    bool nameSet{false}, iconSet{false};
    // empty cmdline usually means kernel space process, so use default name & icon
    if (!ps->cmdline.isEmpty()) {
        if (ctx.m_trayPIDToWndMap.contains(proc.getPID())) {
            // process with tray window
            auto xid = ctx.m_trayPIDToWndMap[proc.getPID()];
            // grab window title with xcb call
            auto title = ctx.m_wm->getWindowName(xid);
            if (!title.isEmpty()) {
                nameSet = true;
                proc.setDisplayName(QString("%1: %2")
                                    .arg(QApplication::translate("Process.Table", "Tray"))
                                    .arg(title));
            } else if (ps->environ.contains("GIO_LAUNCHED_DESKTOP_FILE")) {
                // can't grab window title, try use desktop file instead
                auto desktopFile = ps->environ["GIO_LAUNCHED_DESKTOP_FILE"];
                auto de1 = DesktopEntryStat::createDesktopEntry(desktopFile);
                if (de1 && !de1->displayName.isEmpty()) {
                    nameSet = true;
                    proc.setDisplayName(QString("%1: %2")
                                        .arg(QApplication::translate("Process.Table", "Tray"))
                                        .arg(de1->displayName));
                }
                // use default icon if no desktop file found
                if (de1 && !de1->icon.isNull()) {
                    iconSet = true;
                    proc.setIcon(de1->icon);
                }
            }
        } else if (ctx.m_guiPIDList.contains(ps->pid)) {
            // gui apps case, grab title with xcb call
            auto title = ctx.m_wm->getWindowTitle(ps->pid);
            if (!title.isEmpty()) {
                if (ps->cmdline.size() > 1) {
                    auto url = QUrl(ps->cmdline[ps->cmdline.size() - 1]);
                    auto finfo = QFileInfo(url.fileName());
                    auto rname = url.fileName();
                    rname.chop(finfo.completeSuffix().length() + 1);
                    // check if commandline ends with file name, if so prepend it before process name
                    if (url.isValid()
                            && (url.isLocalFile() || !url.host().isEmpty())
                            && !title.contains(rname)) {
                        nameSet = true;
                        proc.setDisplayName(QString("%1 - %2").arg(rname).arg(title));
                    } else {
                        nameSet = true;
                        proc.setDisplayName(title);
                    }
                }
                if (!nameSet) {
                    nameSet = true;
                    proc.setDisplayName(title);
                }
            }

            if (ctx.m_desktopEntryCache.contains(proc.getName())) {
                de = ctx.m_desktopEntryCache[proc.getName()];
                if (de && de->icon.isNull()) {
                    de->icon = ctx.m_defaultIcon;
                }
                if (!iconSet) {
                    iconSet = true;
                    proc.setIcon(de->icon);
                }
            } else {
                if (ps->environ.contains("GIO_LAUNCHED_DESKTOP_FILE") &&
                        ps->environ.contains("GIO_LAUNCHED_DESKTOP_FILE_PID") &&
                        ps->environ["GIO_LAUNCHED_DESKTOP_FILE_PID"].toInt() == ps->pid) {
                    auto desktopFile = ps->environ["GIO_LAUNCHED_DESKTOP_FILE"];
                    if (ctx.m_desktopEntryCache.contains(proc.getName())) {
                        de = ctx.m_desktopEntryCache[proc.getName()];
                    } else {
                        de = DesktopEntryStat::createDesktopEntry(desktopFile);
                        // rare case, if .desktopFile contains no icon, we need use default icon,
                        // otherwise cached desktop entry's icon will be null
                        if (!de->icon.isNull()) {
                            ctx.m_desktopEntryCache[de->name] = de;
                        }
                    }
                    if (de && !de->icon.isNull()) {
                        iconSet = true;
                        proc.setIcon(de->icon);
                    }
                } else {
                    QIcon icon;
                    auto scale = qApp->devicePixelRatio();
                    auto p24 = ctx.m_wm->getWindowIcon(
                                   ctx.m_wm->getWindow(ps->pid),
                                   int(24 * scale));
                    p24.setDevicePixelRatio(scale);
                    // 80p pixmaps are used in process attribute dialog
                    auto p80 = ctx.m_wm->getWindowIcon(
                                   ctx.m_wm->getWindow(ps->pid),
                                   int(80 * scale));
                    p80.setDevicePixelRatio(scale);
                    icon.addPixmap(p24);
                    icon.addPixmap(p80);
                    if (!p24.isNull()) {
                        iconSet = true;
                        proc.setIcon(icon);
                    }
                }
            }
        } else if (ctx.m_desktopEntryCache.contains(proc.getName())) {
            // found desktop entry
            de = ctx.m_desktopEntryCache[proc.getName()];
            if (de && !de->displayName.isEmpty()) {
                if (ps->cmdline.size() > 1) {
                    // check if last arg of cmdline is url, if so take it's filename
                    auto url = QUrl(ps->cmdline[ps->cmdline.size() - 1]);
                    if (url.isValid() && (url.isLocalFile() || !url.host().isEmpty())) {
                        nameSet = true;
                        proc.setDisplayName(QString("%1 - %2")
                                            .arg(url.fileName())
                                            .arg(de->displayName));
                    }
                }
                if (!nameSet) {
                    nameSet = true;
                    proc.setDisplayName(de->displayName);
                }
            }
            if (de && !de->icon.isNull()) {
                iconSet = true;
                proc.setIcon(de->icon);

                if (!ctx.m_guiPIDList.contains(ps->pid)) {
                    ctx.m_appList << ps->pid;
                }
            }
            if (de && !de->startup_wm_class.isEmpty()) {
                proc.setName(de->startup_wm_class);
            }
        } else {
            // is shell?
            if (ctx.m_shellList.contains(proc.getName())) {
                proc.setDisplayName(proc.getName());
                proc.setIcon(QIcon::fromTheme("terminal"));
                nameSet = iconSet = true;
                auto arrb = ps->cmdline.join(' ');
                QString buf{arrb};
                proc.setDisplayName(buf);
            } else if (ps->environ.contains("GIO_LAUNCHED_DESKTOP_FILE") &&
                       ps->environ.contains("GIO_LAUNCHED_DESKTOP_FILE_PID") &&
                       ps->environ["GIO_LAUNCHED_DESKTOP_FILE_PID"].toInt() == ps->pid) {
                // has gio info set in environment
                auto desktopFile = ps->environ["GIO_LAUNCHED_DESKTOP_FILE"];
                if (ctx.m_desktopEntryCache.contains(proc.getName())) {
                    de = ctx.m_desktopEntryCache[proc.getName()];
                } else {
                    de = DesktopEntryStat::createDesktopEntry(desktopFile);
                    if (de && !de->icon.isNull()) {
                        ctx.m_desktopEntryCache[de->name] = de;
                    }
                }
                if (de && !de->displayName.isEmpty()) {
                    nameSet = true;
                    proc.setDisplayName(de->displayName);
                }
                if (de && !de->icon.isNull()) {
                    iconSet = true;
                    proc.setIcon(de->icon);
                    if (!ctx.m_guiPIDList.contains(ps->pid)) {
                        ctx.m_appList << ps->pid;
                    }
                }
            } else if (ps->cmdline[0].startsWith("/opt")) {
                // special case for google chrome that crap .desktop file & bin name
                QString fname = QFileInfo(QString(ps->cmdline[0]).split(' ')[0]).fileName();
                auto it = ctx.m_desktopEntryCache.constBegin();
                while (it != ctx.m_desktopEntryCache.constEnd()) {
                    if (it.key().contains(fname)) {
                        de = it.value();
                        if (de && !de->displayName.isEmpty()) {
                            nameSet = true;
                            proc.setDisplayName(de->displayName);
                        }
                        if (de && !de->icon.isNull()) {
                            iconSet = true;
                            proc.setIcon(de->icon);
                        }
                        break;
                    }
                    ++it;
                }
            }
        }
    }
    if (!nameSet) {
        proc.setDisplayName(proc.getName());
    }
    if (!iconSet) {
        proc.setIcon(ctx.m_defaultIcon);
    }
}

// merge child process's network stat into parent's (recusively)
void StatsCollector::mergeSubProcNetIO(pid_t ppid, ProcNetIOStat &sum)
{
    auto ppe = m_procEntryMap[ppid];
    auto it = m_pidPtoCMapping.find(ppid);
    while (it != m_pidPtoCMapping.end() && it.key() == ppid) {
        mergeSubProcNetIO(it.value(), sum);
        ++it;
    }
    sum->recvBps += ppe.getRecvBps();
    sum->recvBytes += ppe.getRecvBytes();
    sum->sentBps += ppe.getSentBps();
    sum->sentBytes += ppe.getSentBytes();
}

// calculate process's network stat
ProcNetIOStat StatsCollector::calcProcNetIOStats(const ProcNetIOAgg &prev,
                                                 const ProcNetIOAgg &cur,
                                                 qulonglong interval)
{
    auto stat = QSharedPointer<struct proc_net_io_stat_t>::create();

    stat->sentBytes = cur->sentBytes - prev->sentBytes;
    stat->recvBytes = cur->recvBytes - prev->recvBytes;
    // inbound/outbound bandwidth usage
    stat->sentBps = stat->sentBytes / interval * 100;
    stat->recvBps = stat->recvBytes / interval * 100;

    return stat;
}
