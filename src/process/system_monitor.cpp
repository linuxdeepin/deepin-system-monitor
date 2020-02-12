#include <errno.h>
#include <proc/sysinfo.h>
#include <pwd.h>
#include <sched.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <DApplication>
#include <QDebug>
#include <QIcon>

#include "common/error_context.h"
#include "constant.h"
#include "priority_controller.h"
#include "process/process_entry.h"
#include "process_controller.h"
#include "process_tree.h"
#include "system_monitor.h"
#include "utils.h"

std::atomic<SystemMonitor *> SystemMonitor::m_instance;
std::mutex SystemMonitor::m_mutex;

DWIDGET_USE_NAMESPACE
using namespace Utils;

static bool initial_loading = true;

void SystemMonitor::updateStatus()
{
    // Read the list of open processes information.
    PROCTAB *proc =
        openproc(PROC_FILLMEM |   // memory status: read information from /proc/#pid/statm
                 PROC_FILLSTAT |  // cpu status: read information from /proc/#pid/stat
                 PROC_FILLUSR     // user status: resolve user ids to names via /etc/passwd
                );
    static proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_t));

    StoredProcType processes;
    while (readproc(proc, &proc_info) != nullptr) {
        processes[proc_info.tid] = proc_info;
    }
    closeproc(proc);

    // Fill in CPU.
    prevWorkCpuTime = workCpuTime;
    prevTotalCpuTime = totalCpuTime;
    totalCpuTime = getTotalCpuTime(workCpuTime);

    m_processCpuPercents.clear();
    if (m_prevProcesses.size() > 0) {
        // we have previous proc info
        for (auto &newItr : processes) {
            for (auto &prevItr : m_prevProcesses) {
                if (newItr.first == prevItr.first) {
                    // PID matches, calculate the cpu
                    m_processCpuPercents[newItr.second.tid] = calculateCPUPercentage(
                                                                  &prevItr.second, &newItr.second, prevTotalCpuTime, totalCpuTime);

                    break;
                }
            }
        }
    }

    // Read tray icon process.
    QList<int> trayProcessXids = Utils::getTrayWindows();
    QMap<int, int> trayProcessMap;

    for (auto xid : trayProcessXids) {
        trayProcessMap[m_findWindowTitle->getWindowPid(static_cast<xcb_window_t>(xid))] = xid;
    }

    // Fill gui chlid process information when filterType is OnlyGUI.
    m_findWindowTitle->updateWindowInfos();
    ProcessTree *processTree = new ProcessTree();
    processTree->scanProcesses(processes);
    QMap<int, ChildPidInfo> childInfoMap;
    if (filterType == OnlyGUI) {
        QList<int> guiPids = m_findWindowTitle->getWindowPids();

        // Tray pid also need add in gui pids list.
        for (int pid : trayProcessMap.keys()) {
            if (!guiPids.contains(pid)) {
                guiPids << pid;
            }
        }

        for (int guiPid : guiPids) {
            QList<int> childPids;
            childPids = processTree->getAllChildPids(guiPid);

            for (int childPid : childPids) {
                DiskStatus dStatus = {0, 0};
                NetworkStatus nStatus = {0, 0, 0, 0};
                ChildPidInfo childPidInfo;

                childPidInfo.cpu = 0;
                childPidInfo.memory = 0;
                childPidInfo.diskStatus = dStatus;
                childPidInfo.networkStatus = nStatus;

                childInfoMap[childPid] = childPidInfo;
            }
        }
    }

    // Read processes information.
    int guiProcessNumber = 0;
    int systemProcessNumber = 0;
    QList<ProcessEntry> procList;

    m_wineApplicationDesktopMaps.clear();
    m_wineServerDesktopMaps.clear();

    qreal diskReadTotalKbs = 0;
    qreal diskWriteTotalKbs = 0;

    for (auto &i : processes) {
        int pid = (&i.second)->tid;
        QString cmdline = Utils::getProcessCmdline(pid);
        bool isWineProcess = cmdline.startsWith("c:\\");
        QString name = getProcessName(&i.second, cmdline);
        QString user = (&i.second)->euser;
        double cpu = m_processCpuPercents[pid];

        std::string desktopFile = getProcessDesktopFile(pid, name, cmdline, trayProcessMap);
        QString title = m_findWindowTitle->getWindowTitle(pid);
        bool isGui = trayProcessMap.contains(pid) || (title != "");

        // Record wine application and wineserver.real desktop file.
        // We need transfer wineserver.real network traffic to the corresponding wine program.
        if (name == "wineserver.real") {
            // Insert pid<->desktopFile to map to search in all network process list.
            QString gioDesktopFile =
                Utils::getProcessEnvironmentVariable(pid, "GIO_LAUNCHED_DESKTOP_FILE");
            if (gioDesktopFile != "") {
                m_wineServerDesktopMaps[pid] = gioDesktopFile;
            }
        } else {
            // Insert desktopFile<->pid to map to search in all network process list.
            // If title is empty, it's just a wine program, but not wine GUI window.
            if (isWineProcess && title != "") {
                m_wineApplicationDesktopMaps[QString::fromStdString(desktopFile)] = pid;
            }
        }

        if (isGui) {
            guiProcessNumber++;
        } else {
            systemProcessNumber++;
        }

        bool appendItem = false;
        QString currentUsername {m_euname};
        if (filterType == OnlyGUI) {
            appendItem = (user == currentUsername && isGui);
        } else if (filterType == OnlyMe) {
            appendItem = (user == currentUsername);
        } else if (filterType == AllProcess) {
            appendItem = true;
        }

        ProcessEntry item;
        if (appendItem) {
            if (title == "") {
                if (isWineProcess) {
                    // If wine process's window title is blank, it's not GUI window process.
                    // Title use process name instead.
                    title = name;
                } else {
                    title = getDisplayNameFromName(name, desktopFile);
                }
            }

            // Add tray prefix in title if process is tray process.
            if (trayProcessMap.contains(pid)) {
                title = QString("%1: %2")
                        .arg(DApplication::translate("Process.Table", "Tray"))
                        .arg(title);
            }

            QString displayName;
            //            if (filterType == AllProcess) {
            //                displayName = QString("[%1] %2").arg(user).arg(title);
            //            } else {
            displayName = title;
            //            }

            long memory = getProcessMemory(cmdline, (&i.second)->resident, (&i.second)->share);
            QPixmap icon = getProcessIcon(pid, desktopFile, m_findWindowTitle, 24);
            item.setIcon(icon);
            item.setName(name);
            item.setDisplayName(displayName);
            item.setCPU(cpu);
            item.setMemory(qulonglong(memory));
            item.setPID(pid);
            item.setUserName(user);
            item.setState((&i.second)->state);
            procList << item;
        } else {
            // Fill GUI processes information for continue merge action.
            if (filterType == OnlyGUI) {
                if (childInfoMap.contains(pid)) {
                    long memory =
                        getProcessMemory(cmdline, (&i.second)->resident, (&i.second)->share);

                    childInfoMap[pid].cpu = cpu;
                    childInfoMap[pid].memory = static_cast<qulonglong>(memory);
                }
            }
        }

        // Calculate disk IO kbs.
        DiskStatus ds {0, 0};
        getProcessDiskStatus(pid, ds);
        diskReadTotalKbs += ds.readKbs;
        diskWriteTotalKbs += ds.writeKbs;

        item.setReadKbs(ds.readKbs);
        item.setWriteKbs(ds.writeKbs);
    }

    // Remove dead process from network status maps.
    for (auto pid : m_processSentBytes.keys()) {
        bool foundProcess = false;
        for (auto &i : processes) {
            if ((&i.second)->tid == pid) {
                foundProcess = true;
                break;
            }
        }

        if (!foundProcess) {
            m_processSentBytes.remove(pid);
        }
    }
    for (auto pid : m_processRecvBytes.keys()) {
        bool foundProcess = false;
        for (auto &i : processes) {
            if ((&i.second)->tid == pid) {
                foundProcess = true;
                break;
            }
        }

        if (!foundProcess) {
            m_processRecvBytes.remove(pid);
        }
    }

    // Read memory information.
    meminfo();

    // Update memory status.
    if (kb_swap_total > 0) {
        memStatInfoUpdated((kb_main_total - kb_main_available) * 1024, kb_main_total * 1024,
                           kb_swap_used * 1024, kb_swap_total * 1024);
    } else {
        memStatInfoUpdated((kb_main_total - kb_main_available) * 1024, kb_main_total * 1024, 0, 0);
    }

    // Update process's network status.
    NetworkTrafficFilter::Update update;

    QMap<int, NetworkStatus> networkStatusSnapshot;

    while (NetworkTrafficFilter::getRowUpdate(update)) {
        if (update.action != NETHOGS_APP_ACTION_REMOVE) {
            m_processSentBytes[update.record.pid] = update.record.sent_bytes;
            m_processRecvBytes[update.record.pid] = update.record.recv_bytes;

            NetworkStatus status = {update.record.sent_bytes, update.record.recv_bytes,
                                    qreal(update.record.sent_kbs), qreal(update.record.recv_kbs)
                                   };

            (networkStatusSnapshot)[update.record.pid] = status;
        }
    }

    // Transfer wineserver.real network traffic to the corresponding wine program.
    QMap<int, NetworkStatus>::iterator i;
    for (i = networkStatusSnapshot.begin(); i != networkStatusSnapshot.end(); ++i) {
        if (m_wineServerDesktopMaps.contains(i.key())) {
            QString wineDesktopFile = m_wineServerDesktopMaps[i.key()];

            if (m_wineApplicationDesktopMaps.contains(wineDesktopFile)) {
                // Transfer wineserver.real network traffic to the corresponding wine program.
                int wineApplicationPid = m_wineApplicationDesktopMaps[wineDesktopFile];
                networkStatusSnapshot[wineApplicationPid] = networkStatusSnapshot[i.key()];

                // Reset wineserver network status to zero.
                NetworkStatus networkStatus = {0, 0, 0, 0};
                networkStatusSnapshot[i.key()] = networkStatus;
            }
        }
    }

    // Update ProcessItem's network status.
    for (auto &item : procList) {
        if (networkStatusSnapshot.contains(item.getPID())) {
            auto nstat = networkStatusSnapshot.value(item.getPID());
            item.setSentBytes(nstat.sentBytes);
            item.setSentKbs(nstat.sentKbs);
            item.setRecvBytes(nstat.recvBytes);
            item.setRecvKbs(nstat.recvKbs);
        }
    }

    // update priority status
    updateProcessPriority(procList);

    for (int childPid : childInfoMap.keys()) {
        // Update network status.
        if (networkStatusSnapshot.contains(childPid)) {
            childInfoMap[childPid].networkStatus = networkStatusSnapshot.value(childPid);
        }

        // Update disk status.
        getProcessDiskStatus(childPid, childInfoMap[childPid].diskStatus);
    }

    // Update cpu status.
    QVector<CpuStruct> cpuTimes = getCpuTimes();
    if (prevWorkCpuTime != 0 && prevTotalCpuTime != 0) {
        QVector<qreal> cpuPercentages = calculateCpuPercentages(cpuTimes, prevCpuTimes);

        cpuStatInfoUpdated(
            (workCpuTime - prevWorkCpuTime) * 100.0 / (totalCpuTime - prevTotalCpuTime),
            cpuPercentages);
    } else {
        QVector<qreal> cpuPercentages;

        long int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
        for (long int i = 0; i < numCPU; i++) {
            cpuPercentages.push_back(0);
        }

        cpuStatInfoUpdated(0, cpuPercentages);
    }
    prevCpuTimes = cpuTimes;

    // Merge child process when filterType is OnlyGUI.
    if (filterType == OnlyGUI) {
        for (auto &item : procList) {
            QList<int> childPids;
            childPids = processTree->getAllChildPids(item.getPID());

            for (int childPid : childPids) {
                if (childInfoMap.contains(childPid)) {
                    ChildPidInfo info = childInfoMap[childPid];

                    mergeItemInfo(item, info.cpu, info.memory, info.diskStatus, info.networkStatus);
                } else {
                    qDebug() << QString("IMPOSSIBLE: process %1 not exist in childInfoMap")
                             .arg(childPid);
                }
            }
        }
    }
    delete processTree;

    // Update process number.
    processSummaryUpdated(guiProcessNumber, systemProcessNumber);

    // Update process list
    processListUpdated(procList);

    // Update network status.
    if (prevTotalRecvBytes == 0) {
        prevTotalRecvBytes = totalRecvBytes;
        prevTotalSentBytes = totalSentBytes;

        Utils::getNetworkBandWidth(totalRecvBytes, totalSentBytes);
        networkStatInfoUpdated(totalRecvBytes, totalSentBytes, 0, 0);
    } else {
        prevTotalRecvBytes = totalRecvBytes;
        prevTotalSentBytes = totalSentBytes;

        Utils::getNetworkBandWidth(totalRecvBytes, totalSentBytes);
        networkStatInfoUpdated(totalRecvBytes, totalSentBytes,
                               ((totalRecvBytes - prevTotalRecvBytes) / 1024.0) / updateSeconds,
                               ((totalSentBytes - prevTotalSentBytes) / 1024.0) / updateSeconds);
    }

    // Update disk status.
    diskStatInfoUpdated(diskReadTotalKbs, diskWriteTotalKbs);

    // Keep processes we've read for cpu calculations next cycle.
    m_prevProcesses = processes;

    if (initial_loading) {
        initial_loading = false;
        Q_EMIT initialSysInfoLoaded();
    }
}

void SystemMonitor::setFilterType(SystemMonitor::FilterType type)
{
    filterType = type;
    m_updateStatusTimer->stop();
    updateStatus();
    m_updateStatusTimer->start(updateDuration);
}

void SystemMonitor::endProcess(pid_t pid)
{
    sendSignalToProcess(pid, SIGTERM);
}

void SystemMonitor::pauseProcess(pid_t pid)
{
    sendSignalToProcess(pid, SIGSTOP);
}

void SystemMonitor::resumeProcess(pid_t pid)
{
    sendSignalToProcess(pid, SIGCONT);
}

void SystemMonitor::killProcess(pid_t pid)
{
    sendSignalToProcess(pid, SIGKILL);
}

ErrorContext SystemMonitor::setProcessPriority(pid_t pid, int priority)
{
    sched_param param {};
    ErrorContext ec {};

    auto errfmt = [ = ](int err, ErrorContext & errorContext) -> ErrorContext & {
        errorContext.setCode(ErrorContext::kErrorTypeSystem);
        errorContext.setSubCode(err);
        errorContext.setErrorName(
            DApplication::translate("Process.Priority", "Failed to change process priority"));
        QString errmsg = QString("PID: %1, Error: [%2] %3").arg(pid).arg(err).arg(strerror(err));
        errorContext.setErrorMessage(errmsg);
        return errorContext;
    };

    errno = 0;
    int rc = sched_getparam(pid, &param);
    if (rc == -1) {
        return errfmt(errno, ec);
    }
    if (param.sched_priority == 0) {
        // dynamic priority
        if (priority > kVeryLowPriorityMin)
            priority = kVeryLowPriorityMin;
        else if (priority < kVeryHighPriorityMax)
            priority = kVeryHighPriorityMax;

        errno = 0;
        rc = setpriority(PRIO_PROCESS, id_t(pid), priority);
        if (rc == -1 && errno != 0) {
            if (errno == EACCES || errno == EPERM) {
                // call pkexec to change priority
                PriorityController *ctrl = new PriorityController(pid, priority, this);
                connect(ctrl, &PriorityController::resultReady, this, [ = ](int code) {
                    if (code == 0) {
                        Q_EMIT processPriorityChanged(pid, priority);
                    } else {
                        ErrorContext ec {};
                        ec.setCode(ErrorContext::kErrorTypeSystem);
                        ec.setSubCode(code);
                        ec.setErrorName(DApplication::translate("Process.Priority",
                                                                "Failed to change process priority"));
                        ec.setErrorMessage(
                            DApplication::translate("Process.Priority", "PID: %1, Error: [%2] %3")
                            .arg(pid)
                            .arg(code)
                            .arg(strerror(code)));
                        Q_EMIT priorityPromoteResultReady(ec);
                    }
                });
                connect(ctrl, &PriorityController::finished, ctrl, &QObject::deleteLater);
                ctrl->start();
                return {};
            } else {
                return errfmt(errno, ec);
            }
        } else {
            Q_EMIT processPriorityChanged(pid, priority);
            return ec;
        }
    } else {
        // static priority
        // TODO: do nothing at this moment, call sched_setparam to change static priority when
        // needed
    }
    return ec;
}

void SystemMonitor::sendSignalToProcess(pid_t pid, int signal)
{
    int rc = 0;
    ErrorContext ec = {};
    auto errfmt = [ = ](decltype(errno) err, const QString & title, int p, int sig,
    ErrorContext & ectx) -> ErrorContext & {
        ectx.setCode(ErrorContext::kErrorTypeSystem);
        ectx.setSubCode(err);
        ectx.setErrorName(title);
        QString errmsg = QString("PID: %1, Signal: [%2], Error: [%3] %4")
        .arg(p)
        .arg(sig)
        .arg(err)
        .arg(strerror(err));
        ectx.setErrorMessage(errmsg);
        return ectx;
    };
    auto emitSignal = [ = ](int signal) {
        if (signal == SIGTERM) {
            Q_EMIT processEnded(pid);
        } else if (signal == SIGSTOP) {
            Q_EMIT processPaused(pid, 'T');
        } else if (signal == SIGCONT) {
            Q_EMIT processResumed(pid, 'R');
        } else if (signal == SIGKILL) {
            Q_EMIT processKilled(pid);
        } else {
            qDebug() << "Unexpected signal in this case:" << signal;
        }
    };
    auto fmsg = [ = ](int signal) -> QString {
        if (signal == SIGTERM)
        {
            return DApplication::translate("Process.Signal", "Failed to end proces");
        } else if (signal == SIGSTOP)
        {
            return DApplication::translate("Process.Signal", "Failed to pause process");
        } else if (signal == SIGCONT)
        {
            return DApplication::translate("Process.Signal", "Failed to resume process");
        } else if (signal == SIGKILL)
        {
            return DApplication::translate("Process.Signal", "Failed to kill process");
        } else
        {
            return DApplication::translate("Process.Signal", "Unknown error");
        }
    };
    auto pctl = [ = ](pid_t pid, int signal) {
        // call pkexec to promote
        auto *ctrl = new ProcessController(pid, signal, this);
        connect(ctrl, &ProcessController::resultReady, this,
        [this, errfmt, emitSignal, pid, signal, fmsg](int code) {
            if (code != 0) {
                ErrorContext ec = {};
                ec = errfmt(code, fmsg(signal), pid, signal, ec);
                Q_EMIT processControlResultReady(ec);
            } else {
                emitSignal(signal);
            }
        });
        connect(ctrl, &PriorityController::finished, ctrl, &QObject::deleteLater);
        ctrl->start();
    };
    auto pkill = [this, pctl, errfmt, emitSignal, fmsg](pid_t pid, int signal) {
        int rc = 0;
        errno = 0;
        ErrorContext ec = {};
        rc = kill(pid, signal);
        if (rc == -1 && errno != 0) {
            if (errno == EPERM) {
                pctl(pid, signal);
            } else {
                ec = errfmt(errno, fmsg(signal), pid, signal, ec);
                Q_EMIT processControlResultReady(ec);
                return;
            }
        } else {
            emitSignal(signal);
        }
    };

    if (signal == SIGTERM || signal == SIGKILL) {
        ec = {};
        errno = 0;
        // send SIGCONT first, otherwise signal will hang
        rc = kill(pid, SIGCONT);
        if (rc == -1 && errno != 0) {
            // not authorized, use pkexec instead
            if (errno == EPERM) {
                pctl(pid, signal);
            } else {
                ec = errfmt(
                         errno,
                         DApplication::translate("Process.Signal", "Failed sending signal to process"),
                         pid, SIGCONT, ec);
                Q_EMIT processControlResultReady(ec);
                return;
            }
        } else {
            pkill(pid, signal);
        }
    } else {
        pkill(pid, signal);
    }
}

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent)
    , m_findWindowTitle(new FindWindowTitle)
{
    updateSeconds = updateDuration / 1000.0;

    uid_t euid = geteuid();
    struct passwd *pwd = getpwuid(euid);
    if (pwd) {
        m_euname = QString(pwd->pw_name);
    } else {
        m_euname = getenv("USER");
    }

    // Start timer.
    m_updateStatusTimer = new QTimer(this);
    connect(m_updateStatusTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    m_updateStatusTimer->start(updateDuration);
}

void SystemMonitor::getProcessDiskStatus(int pid, DiskStatus &ds)
{
    ProcPidIO pidIO;
    getProcPidIO(pid, pidIO);

    if (m_procTotalWrite.contains(pid) && pidIO.write_bytes > 0) {
        ds.writeKbs =
            (pidIO.write_bytes - m_procTotalWrite.value(pid)) * 1.0 / 1024 / updateSeconds;
    }
    m_procTotalWrite[pid] = pidIO.write_bytes;

    if (m_procTotalRead.contains(pid) && pidIO.read_bytes > 0) {
        ds.readKbs = (pidIO.read_bytes - m_procTotalRead.value(pid)) * 1.0 / 1024 / updateSeconds;
    }
    m_procTotalRead[pid] = pidIO.read_bytes;
}

void SystemMonitor::updateProcessPriority(QList<ProcessEntry> &list)
{
    for (ProcessEntry &item : list) {
        // get process priority
        sched_param param {};
        errno = 0;
        pid_t pid = item.getPID();
        int rc = sched_getparam(pid, &param);
        if (errno != 0 && rc < 0) {
            qDebug() << "sched_getparam failed: [" << errno << "]:" << strerror(errno);
            continue;
        }
        if (param.sched_priority == 0) {
            // static priority == 0 means non-realtime process (SCHED_OTHER || SCHED_BATCH ||
            // SCHED_IDLE)
            errno = 0;
            int prio = getpriority(PRIO_PROCESS, id_t(pid));
            if (prio == -1 && errno != 0) {
                qDebug() << "getpriority failed: [" << errno << "]:" << strerror(errno);
                continue;
            }
            // dynamic priority = nice
            item.setPriority(prio);
        } else {
            // static priority > 0 means realtime process (SCHED_FIFO || SCHED_RR ||
            // SCHED_DEADLINE...), set as normal priority for the current moment, just as
            // (gnome-system-monitor) did
            item.setPriority(kNormalPriority);
        }
    }
}

void SystemMonitor::mergeItemInfo(ProcessEntry &item, double childCpu, qulonglong childMemory,
                                  const DiskStatus &childDiskStatus,
                                  const NetworkStatus &childNetworkStatus)
{
    item.setCPU(item.getCPU() + childCpu);
    item.setMemory(item.getMemory() + childMemory);
    item.setReadKbs(item.getReadKbs() + childDiskStatus.readKbs);
    item.setWriteKbs(item.getWriteKbs() + childDiskStatus.writeKbs);
    item.setSentBytes(item.getSentBytes() + childNetworkStatus.sentBytes);
    item.setRecvBytes(item.getRecvBytes() + childNetworkStatus.recvBytes);
    item.setSentKbs(item.getSentKbs() + childNetworkStatus.sentKbs);
    item.setRecvKbs(item.getRecvKbs() + childNetworkStatus.recvKbs);
}
