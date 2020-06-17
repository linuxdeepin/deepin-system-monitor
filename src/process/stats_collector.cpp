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

#include <QDebug>
#include <DDesktopEntry>
#include <QPixmap>
#include <QDateTime>

#include "stats_collector.h"
#include "process_entry.h"
#include "utils.h"

#define SECTOR_SHIFT 9
#define SECTOR_SIZE (1 << SECTOR_SHIFT)

DCORE_USE_NAMESPACE

// unit in milliseconds
static const int kUpdateInterval = 2000;

auto calcCPUUsage = [](const CPUStat &prev, const CPUStat &cur) -> qreal
{
    qulonglong cur_tot {}, prev_tot {}, cur_idle {}, prev_idle {};

    if (!cur.isNull()) {
        cur_tot = cur->user +
                  cur->nice +
                  cur->sys +
                  cur->idle +
                  cur->iowait +
                  cur->hardirq +
                  cur->softirq +
                  cur->steal;
        cur_idle = cur->idle + cur->iowait;
    }

    if (!prev.isNull()) {
        prev_tot = prev->user +
                   prev->nice +
                   prev->sys +
                   prev->idle +
                   prev->iowait +
                   prev->hardirq +
                   prev->softirq +
                   prev->steal;
        prev_idle = prev->idle + prev->iowait;
    }

    auto totald = (cur_tot > prev_tot) ? (cur_tot - prev_tot) : 0;
    auto idled = (cur_idle > prev_idle) ? (cur_idle - prev_idle) : 0;

    auto cpuPecent = (totald - idled) * 1. / totald;
    return cpuPecent;
};

StatsCollector::StatsCollector(QObject *parent) :
    QObject(parent),
    m_wm(new FindWindowTitle{})
{
    qRegisterMetaType<QList<ProcessEntry>>("ProcessEntryList");
    qRegisterMetaType<QHash<QString, DesktopEntry>>("DesktopEntryCache");

    m_cpuStat[kLastStat] = CPUStat(new cpu_stat {});
    m_cpuStat[kCurrentStat] = CPUStat(new cpu_stat {});

    // fill shell list
    [ = ] {
        FILE *fp;
        char buf[128] {};
        fp = fopen("/etc/shells", "r");
        char *s;
        if (fp)
        {
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

void StatsCollector::start()
{
    (void)QApplication::instance();
    // Start timer.
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateStatus()));

    m_euid = geteuid();

    m_defaultIcon = QIcon::fromTheme("application-x-executable");

    // prepare cache
    m_desktopEntryStat = new DesktopEntryStat();
    m_desktopEntryStat->moveToThread(&m_cacheThread);
    connect(&m_cacheThread, &QThread::started, m_desktopEntryStat, &DesktopEntryStat::start);
    connect(&m_cacheThread, &QThread::finished, m_desktopEntryStat, &QObject::deleteLater);
    connect(m_desktopEntryStat, &DesktopEntryStat::cacheUpdated, this, [ = ](DesktopEntryCache cache) {
        m_desktopEntryCache = cache;
        if (!m_timer->isActive()) {
            updateStatus();
            m_timer->start(kUpdateInterval);
        }
    });
    m_cacheThread.start();
}

void readProcStatsCallback(ProcStat &ps, void *context)
{
    if (!context)
        return;

    auto *ctx = static_cast<StatsCollector *>(context);
    if (!ctx)
        return;

    if (ps) {
        ctx->m_procMap[StatsCollector::kCurrentStat][ps->pid] = ps;

        ctx->m_pidCtoPMapping.insert(ps->pid, ps->ppid);
        ctx->m_pidPtoCMapping.insert(ps->ppid, ps->pid);

        if (ps->environ.contains("GIO_LAUNCHED_DESKTOP_FILE")) {
            ctx->m_gioDesktopMapping[ps->pid] = ps->environ["GIO_LAUNCHED_DESKTOP_FILE"];
        }
        if (ps->environ.contains("GIO_LAUNCHED_DESKTOP_FILE_PID")) {
            bool ok = false;
            auto pid = ps->environ["GIO_LAUNCHED_DESKTOP_FILE_PID"].toInt(&ok);
            if (ok) {
                ctx->m_gioPIDMapping.insert(ps->pid, pid);
                ctx->m_gioRevPIDMapping.insert(pid, ps->pid);
            }
        }

        ProcessEntry proc {};

        proc.setPID(ps->pid);
        proc.setUID(ps->uid);
        proc.setGID(ps->gid);
        proc.setPriority(ps->nice);
        proc.setState(ps->state);

        auto curUptm = ctx->m_uptime[StatsCollector::kCurrentStat];
        auto lastUptm = ctx->m_uptime[StatsCollector::kLastStat];
        qulonglong interval = (curUptm > lastUptm) ? (curUptm - lastUptm) : 1;

        proc.setCPU(ctx->calcProcCPUStats(
                        ctx->m_procMap[StatsCollector::kLastStat][ps->pid],
                        ctx->m_procMap[StatsCollector::kCurrentStat][ps->pid],
                        ctx->m_cpuStat[StatsCollector::kLastStat],
                        ctx->m_cpuStat[StatsCollector::kCurrentStat],
                        interval,
                        ctx->m_statCtx.hz));
        proc.setMemory(ps->rss - ps->shm);

        // process name
        proc.setName(Utils::normalizeProcName(ps->proc_name, ps->cmdline));

        // set displayName & icon
        setProcDisplayNameAndIcon(*ctx, proc, ps);

        proc.setCmdline(QUrl::fromPercentEncoding(ps->cmdline.join(' ')));
        proc.setStartTime(ctx->m_btime + time_t(ps->start_time / ctx->m_statCtx.hz));

        if (ctx->m_uidCache.contains(ps->euid)) {
            proc.setUserName(ctx->m_uidCache[ps->euid]);
        } else {
            QString euname = SystemStat::getUserName(ps->euid);
            ctx->m_uidCache[ps->euid] = euname;
            proc.setUserName(euname);
        }

        auto rwbps = ctx->calcProcDiskIOStats(
                         ctx->m_procMap[StatsCollector::kLastStat][ps->pid],
                         ctx->m_procMap[StatsCollector::kCurrentStat][ps->pid],
                         interval);
        proc.setReadBps(rwbps.first);
        proc.setWriteBps(rwbps.second);

        if (ctx->m_procNetIO.contains(ps->pid)) {
            auto pidIO = ctx->m_procNetIO[ps->pid];
            proc.setSentBytes(pidIO->sentBytes);
            proc.setRecvBytes(pidIO->recvBytes);
            proc.setSentBps(pidIO->sentBps);
            proc.setRecvBps(pidIO->recvBps);
        }

        ctx->m_procEntryMap[ps->pid] = proc;
    }
}

void StatsCollector::updateStatus()
{
    bool b;
    qulonglong uptime {};
    CPUStat cpuStat {};
    CPUStatMap cpuStatMap {};
    MemStat memStat {};
    DiskIOStat iostat {};
    DiskIOStatMap iostatMap {};
    NetIFStat ifstat {};
    NetIFStatMap ifstatMap {};

    m_napps = m_nprocs = 0;

    ProcessStat::getStatContext(m_statCtx);

    if (!m_btime) {
        b = SystemStat::readBootTime(m_btime);
    }

    b = SystemStat::readUpTime(uptime);
    if (b) {
        m_uptime[kLastStat] = m_uptime[kCurrentStat];
        m_uptime[kCurrentStat] = uptime;
        m_interval = (m_uptime[kCurrentStat] - m_uptime[kLastStat]) / 100.;
        if (m_interval <= 1) {
            m_interval = 2.;
        }
    }

    b = SystemStat::readCPUStats(cpuStat, cpuStatMap);
    if (b) {
        m_cpuStat[kLastStat] = m_cpuStat[kCurrentStat];
        m_cpuStat[kCurrentStat] = cpuStat;
        m_cpuStatMap[kLastStat] = m_cpuStatMap[kCurrentStat];
        m_cpuStatMap[kCurrentStat] = cpuStatMap;

        auto cpuPecent = calcCPUUsage(m_cpuStat[kLastStat], m_cpuStat[kCurrentStat]);

        QList<qreal> cpuPecents;
        for (auto i = 0; i < cpuStatMap.size(); i++) {
            auto &cur = m_cpuStatMap[kCurrentStat][i];
            auto &prev = m_cpuStatMap[kLastStat][i];
            auto cpp = calcCPUUsage(prev, cur);
            cpuPecents << cpp;
        }

        Q_EMIT cpuStatInfoUpdated(cpuPecent * 100., cpuPecents);
    }

    b = SystemStat::readMemStats(memStat);
    if (b) {
        if (memStat->swap_total_kb > 0) {
            Q_EMIT memStatInfoUpdated(
                memStat->mem_total_kb - memStat->mem_avail_kb,
                memStat->mem_total_kb,
                memStat->swap_total_kb - memStat->swap_free_kb,
                memStat->swap_total_kb);
        } else {
            Q_EMIT memStatInfoUpdated(
                memStat->mem_total_kb - memStat->mem_avail_kb,
                memStat->mem_total_kb,
                0,
                0);
        }
    }

    b = SystemStat::readDiskIOStats(iostat, iostatMap);
    if (b) {
        m_ioStat[kLastStat] = m_ioStat[kCurrentStat];
        m_ioStat[kCurrentStat] = iostat;

        qulonglong cr {}, cw {}, cd {}, pr {}, pw {}, pd {};
        if (!m_ioStat[kCurrentStat].isNull()) {
            cr = m_ioStat[kCurrentStat]->read_sectors;
            cw = m_ioStat[kCurrentStat]->write_sectors;
            cd = m_ioStat[kCurrentStat]->discard_sectors;
        }
        if (!m_ioStat[kLastStat].isNull()) {
            pr = m_ioStat[kLastStat]->read_sectors;
            pw = m_ioStat[kLastStat]->write_sectors;
            pd = m_ioStat[kLastStat]->discard_sectors;
        }

        auto rdiff = (cr > pr) ? (cr - pr) : 0;
        auto wdiff = (cw > pw) ? (cw - pw) : 0;
        auto ddiff = (cd > pd) ? (cd - pd) : 0;

        auto rsize = rdiff * SECTOR_SIZE;
        auto wsize = (wdiff + ddiff) * SECTOR_SIZE;

        Q_EMIT diskStatInfoUpdated(rsize / m_interval, wsize / m_interval);
    }

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

        auto rxdiff = (crxb > prxb) ? (crxb - prxb) : 0;
        auto txdiff = (ctxb > ptxb) ? (ctxb - ptxb) : 0;

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

    m_pidCtoPMapping.clear();
    m_pidPtoCMapping.clear();

    m_gioPIDMapping.clear();
    m_gioDesktopMapping.clear();
    m_gioRevPIDMapping.clear();

    m_trayPIDToWndMap.clear();
    m_guiPIDList.clear();
    m_appList.clear();

    m_wm->updateWindowInfos();
    m_guiPIDList = m_wm->getWindowPids();

    auto trayProcessXids = Utils::getTrayWindows();
    for (auto xid : trayProcessXids) {
        auto pid = m_wm->getWindowPid(xid);
        m_trayPIDToWndMap[pid] = xid;
    }

    // Update process's network status.
    NetworkTrafficFilter::Update update {};
    m_procNetIO.clear();

    while (NetworkTrafficFilter::getRowUpdate(update)) {
        if (update.action != NETHOGS_APP_ACTION_REMOVE) {
            auto pidIO = NetIO(new net_io{});
            pidIO->recvBps = qreal(update.record.recv_kbs) * 1024;
            pidIO->sentBps = qreal(update.record.sent_kbs) * 1024;
            pidIO->recvBytes = update.record.recv_bytes;
            pidIO->sentBytes = update.record.sent_bytes;

            m_procNetIO[update.record.pid] = pidIO;
        }
    }

    b = ProcessStat::readProcStats(readProcStatsCallback, this);

    // filter duplicate processes with same name (eg: dman, uos browser like apps with chrome engine biltin)
    // within appList if current display mode is onlyGUI
    decltype(m_appList) filteredAppList {};
    std::function<bool(pid_t ppid)> anyRootIsGuiProc;
    anyRootIsGuiProc = [&](pid_t ppid) -> bool {
        bool b;
        b = m_guiPIDList.contains(ppid);
        if (!b && m_pidCtoPMapping.contains(ppid))
        {
            b = anyRootIsGuiProc(m_pidCtoPMapping[ppid]);
        }
        return b;
    };
    for (auto app : m_appList) {
        if (m_guiPIDList.contains(app))
            continue;

        auto isCmdInList = [ = ](QByteArray cmd) {
            bool b = false;

            auto subCmd = cmd.mid(cmd.lastIndexOf('/') + 1);
            for (auto s : m_shellList) {
                if (subCmd.startsWith(s.toLocal8Bit())) {
                    b = true;
                    return b;
                }
            }
            for (auto s : m_scriptingList) {
                if (cmd.startsWith(s.toLocal8Bit())) {
                    b = true;
                    return b;
                }
            }
            return b;
        };
        auto cmd = m_procMap[kCurrentStat][app]->cmdline[0];
        bool b = false;
        if (cmd[0] == '/') {
            // cmd starts with full path
            b = isCmdInList(cmd);
        } else {
            // cmd starts with raw name
            for (auto p : m_envPathList) {
                p = p.append('/').append(cmd); // e.g. /usr/bin/xxx2.7

                b = isCmdInList(p);
                if (b) {
                    break;
                }
            }
        }
        if (b) {
            continue;
        }

        if (m_pidCtoPMapping.contains(app) &&
                anyRootIsGuiProc(m_pidCtoPMapping[app])) {
            continue;
        }

        filteredAppList << app;
    }

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
                   && m_euid == uid
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
                NetIO sumIO = NetIO(new net_io{});

                mergeSubProcNetIO(pid, sumIO);

                pe.setRecvBps(sumIO->recvBps);
                pe.setRecvBytes(sumIO->recvBytes);
                pe.setSentBps(sumIO->sentBps);
                pe.setSentBytes(sumIO->sentBytes);
            }
        }

        if (need) {
            filteredList << pe;
        }
    }

    if (b && filteredList.size() > 0) {
        Q_EMIT processListUpdated(filteredList);
    }
    if (m_procEntryMap.size() > 0) {
        m_napps = m_guiPIDList.size() + filteredAppList.size() + m_trayPIDToWndMap.size();
        Q_EMIT processSummaryUpdated(m_napps, m_nprocs - m_napps);
    }
}

void StatsCollector::setFilterType(SystemMonitor::FilterType type)
{
    if (m_filterType != type) {
        m_filterType = type;
        m_timer->stop();
        updateStatus();
        m_timer->start(kUpdateInterval);
    }
}

qreal StatsCollector::calcProcCPUStats(ProcStat &prev,
                                       ProcStat &cur,
                                       CPUStat &prevCPU,
                                       CPUStat &curCPU,
                                       qulonglong interval,
                                       unsigned long hz)
{
    qulonglong cur_jiff {}, prev_jiff {};
    if (!curCPU.isNull()) {
        cur_jiff = curCPU->user +
                   curCPU->nice +
                   curCPU->sys +
                   curCPU->idle +
                   curCPU->iowait +
                   curCPU->hardirq +
                   curCPU->softirq +
                   curCPU->steal;
    }

    if (!prevCPU.isNull()) {
        prev_jiff = prevCPU->user +
                    prevCPU->nice +
                    prevCPU->sys +
                    prevCPU->idle +
                    prevCPU->iowait +
                    prevCPU->hardirq +
                    prevCPU->softirq +
                    prevCPU->steal;
    }

    auto delta = cur_jiff - prev_jiff;
    if (delta <= 0) {
        delta = 1;
    }
    qulonglong ctm = cur.isNull() ? 0 : (cur->stime + cur->utime);
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

QPair<qreal, qreal> StatsCollector::calcProcDiskIOStats(ProcStat &prev,
                                                        ProcStat &cur,
                                                        qulonglong interval)
{
    qreal rdio {}, wrio {};
    // current read/write/cancelled, previous read/write/cancelled
    qulonglong crb{}, cwb{}, ccwb{}, prb{}, pwb{}, pcwb{};

    if (!prev.isNull()) {
        prb = prev->read_bytes;
        pwb = prev->write_bytes;
        pcwb = prev->cancelled_write_bytes;
    }
    if (!cur.isNull()) {
        crb = cur->read_bytes;
        cwb = cur->write_bytes;
        ccwb = cur->cancelled_write_bytes;
    }

    auto rdiff = (crb < prb) ? 0 : (crb - prb);
    auto wdiff = (cwb < pwb) ? 0 : (cwb - pwb);
    auto cwdiff = (ccwb < pcwb) ? 0 : (ccwb - pcwb);
    rdio = qreal(rdiff) / interval * 100;
    wrio = qreal(wdiff - cwdiff) / interval * 100;

    return {rdio, wrio};
}

void setProcDisplayNameAndIcon(StatsCollector &ctx, ProcessEntry &proc, const ProcStat &ps)
{
    // =====================================================================
    // displayName & icon kinda tricky here
    // =====================================================================
    DesktopEntry de {};
    bool nameSet{false}, iconSet{false};
    // empty cmdline usually means kernel space process, so use default name & icon
    if (!ps->cmdline.isEmpty()) {
        if (ctx.m_trayPIDToWndMap.contains(proc.getPID())) {
            auto xid = ctx.m_trayPIDToWndMap[proc.getPID()];
            auto title = ctx.m_wm->getWindowName(xid);
            if (!title.isEmpty()) {
                nameSet = true;
                proc.setDisplayName(QString("%1: %2")
                                    .arg(DApplication::translate("Process.Table", "Tray"))
                                    .arg(title));
            } else if (ps->environ.contains("GIO_LAUNCHED_DESKTOP_FILE")) {
                // can't grab window title, try use desktop file instead
                auto desktopFile = ps->environ["GIO_LAUNCHED_DESKTOP_FILE"];
                auto de = DesktopEntryStat::createDesktopEntry(desktopFile);
                if (!de->displayName.isEmpty()) {
                    nameSet = true;
                    proc.setDisplayName(QString("%1: %2")
                                        .arg(DApplication::translate("Process.Table", "Tray"))
                                        .arg(de->displayName));
                }
                if (!de->icon.isNull()) {
                    iconSet = true;
                    proc.setIcon(de->icon);
                }
            }
        } else if (ctx.m_guiPIDList.contains(ps->pid)) {
            auto title = ctx.m_wm->getWindowTitle(ps->pid);
            if (!title.isEmpty()) {
                if (ps->cmdline.size() > 1) {
                    auto url = QUrl(ps->cmdline[ps->cmdline.size() - 1]);
                    auto finfo = QFileInfo(url.fileName());
                    auto rname = url.fileName();
                    rname.chop(finfo.completeSuffix().length() + 1);
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
                        ctx.m_desktopEntryCache[de->name] = de;
                    }
                    if (!de->icon.isNull()) {
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
            de = ctx.m_desktopEntryCache[proc.getName()];
            if (!de->displayName.isEmpty()) {
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
            if (!de->icon.isNull()) {
                iconSet = true;
                proc.setIcon(de->icon);

                if (!ctx.m_guiPIDList.contains(ps->pid)) {
                    ctx.m_appList << ps->pid;
                }
            }
            if (!de->startup_wm_class.isEmpty()) {
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
                auto desktopFile = ps->environ["GIO_LAUNCHED_DESKTOP_FILE"];
                if (ctx.m_desktopEntryCache.contains(proc.getName())) {
                    de = ctx.m_desktopEntryCache[proc.getName()];
                } else {
                    de = DesktopEntryStat::createDesktopEntry(desktopFile);
                    ctx.m_desktopEntryCache[de->name] = de;
                }
                if (!de->displayName.isEmpty()) {
                    nameSet = true;
                    proc.setDisplayName(de->displayName);
                }
                if (!de->icon.isNull()) {
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
                        if (!de->displayName.isEmpty()) {
                            nameSet = true;
                            proc.setDisplayName(de->displayName);
                        }
                        if (!de->icon.isNull()) {
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

void StatsCollector::mergeSubProcNetIO(pid_t ppid, NetIO &sum)
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

#if 0
QPair<qreal, qreal> StatsCollector::calcProcNetIOStats(ProcStat &prev, ProcStat &cur, qulonglong interval)
{
    return {};
}
#endif
