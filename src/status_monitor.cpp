#include "status_monitor.h"
#include <QPainter>

#include <unistd.h>
#include "process_item.h"
#include "utils.h"
#include <proc/sysinfo.h>
#include <thread>
#include <QDebug>

using namespace Utils;

StatusMonitor::StatusMonitor(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(300);

    layout = new QVBoxLayout(this);

    cpuMonitor = new CpuMonitor();
    memoryMonitor = new MemoryMonitor();
    networkMonitor = new NetworkMonitor();

    layout->addWidget(cpuMonitor);
    layout->addWidget(memoryMonitor);
    layout->addWidget(networkMonitor);

    totalSentBytes = 0;
    totalRecvBytes = 0;
    totalSentKbs = 0;
    totalRecvKbs = 0;

    findWindowTitle = new FindWindowTitle();

    // Init process icon cache.
    processIconCache = new QMap<QString, QPixmap>();

    processSentBytes = new QMap<int, uint32_t>();
    processRecvBytes = new QMap<int, uint32_t>();

    processWriteKbs = new QMap<int, unsigned long>();
    processReadKbs = new QMap<int, unsigned long>();

    connect(this, &StatusMonitor::updateMemoryStatus, memoryMonitor, &MemoryMonitor::updateStatus, Qt::QueuedConnection);
    connect(this, &StatusMonitor::updateCpuStatus, cpuMonitor, &CpuMonitor::updateStatus, Qt::QueuedConnection);
    connect(this, &StatusMonitor::updateNetworkStatus, networkMonitor, &NetworkMonitor::updateStatus, Qt::QueuedConnection);

    updateStatusTimer = new QTimer();
    connect(updateStatusTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    updateStatusTimer->start(updateDuration);
}

StatusMonitor::~StatusMonitor()
{
    delete layout;
    delete cpuMonitor;
    delete memoryMonitor;
    delete networkMonitor;
    delete processIconCache;
    delete updateStatusTimer;
}

void StatusMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
}

void StatusMonitor::updateStatus()
{
    // Read the list of open processes information.
    PROCTAB* proc = openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLUSR | PROC_FILLCOM);
    static proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_t));

    storedProcType processes;
    while (readproc(proc, &proc_info) != NULL) {
        processes[proc_info.tid]=proc_info;
    }
    closeproc(proc);

    // Fill in CPU.
    if (prevProcesses.size()>0) {
        // we have previous proc info
        for(auto &newItr:processes) {
            for(auto &prevItr:prevProcesses) {
                if (newItr.first == prevItr.first) {
                    // PID matches, calculate the cpu
                    newItr.second.pcpu = (unsigned int) calculateCPUPercentage(&prevItr.second, &newItr.second, totalCpuTime);
                    break;
                }
            }
        }
    }

    // Update the cpu time for next loop.
    totalCpuTime = getTotalCpuTime();

    // Read processes information.
    QList<ListItem*> items;
    QString username = qgetenv("USER");

    int cpuNumber = sysconf(_SC_NPROCESSORS_ONLN);
    double totalCpuPercent = 0;

    findWindowTitle->updateWindowInfos();

    for(auto &i:processes) {
        QString user = (&i.second)->euser;

        double cpu = (&i.second)->pcpu;

        if (user == username) {
            QString name = getProcessName(&i.second);
            int pid = (&i.second)->tid;
            QString displayName;

            QString title = findWindowTitle->findWindowTitle(pid);
            if (title != "") {
                displayName = title;
            } else {
                displayName = getDisplayNameFromName(name);
            }
            int memory = ((&i.second)->resident - (&i.second)->share) * sysconf(_SC_PAGESIZE);
            QPixmap icon = getProcessIconFromName(name, processIconCache);
            ProcessItem *item = new ProcessItem(icon, name, displayName, cpu / cpuNumber, memory, pid, user);
            items << item;
        }

        totalCpuPercent += cpu;
    }

    // Remove dead process from network status maps.
    for (auto pid : processSentBytes->keys()) {
        bool foundProcess = false;
        for (ListItem *item : items) {
            if ((static_cast<ProcessItem*>(item))->getPid() == pid) {
                foundProcess = true;
                break;
            }
        }

        if (!foundProcess) {
            processSentBytes->remove(pid);

            qDebug() << QString("Remove %1 from sent bytes map.").arg(pid);
        }
    }
    for (auto pid : processRecvBytes->keys()) {
        bool foundProcess = false;
        for (ListItem *item : items) {
            if ((static_cast<ProcessItem*>(item))->getPid() == pid) {
                foundProcess = true;
                break;
            }
        }

        if (!foundProcess) {
            processRecvBytes->remove(pid);

            qDebug() << QString("Remove %1 from recv bytes map.").arg(pid);
        }
    }

    // Have procps read the memory。
    meminfo();

    // Update memory status.
    if (kb_swap_total > 0.0)  {
        updateMemoryStatus(kb_main_used * 1024, kb_main_total * 1024, kb_swap_used * 1024, kb_swap_total * 1024);
    } else {
        updateMemoryStatus(kb_main_used * 1024, kb_main_total * 1024, 0, 0);
    }

    if (NetworkTrafficFilter::getNetHogsMonitorStatus() != NETHOGS_STATUS_OK) {
        qDebug() << "Failed to access network device(s).";
    }

    // Update network status.
    NetworkTrafficFilter::Update update;

    QMap<int, NetworkStatus> *networkStatusSnapshot = new QMap<int, NetworkStatus>();
    totalSentKbs = 0;
    totalRecvKbs = 0;
    while (NetworkTrafficFilter::getRowUpdate(update)) {
        if (update.action != NETHOGS_APP_ACTION_REMOVE) {
            uint32_t prevSentBytes = 0;
            uint32_t prevRecvBytes = 0;

            if (processSentBytes->contains(update.record.pid)) {
                prevSentBytes = processSentBytes->value(update.record.pid);
            }
            if (processRecvBytes->contains(update.record.pid)) {
                prevRecvBytes = processRecvBytes->value(update.record.pid);
            }

            totalSentKbs += update.record.sent_kbs;
            totalRecvKbs += update.record.recv_kbs;

            totalSentBytes += (update.record.sent_bytes - prevSentBytes);
            totalRecvBytes += (update.record.recv_bytes - prevRecvBytes);

            (*processSentBytes)[update.record.pid] = update.record.sent_bytes;
            (*processRecvBytes)[update.record.pid] = update.record.recv_bytes;

            NetworkStatus status = {
                update.record.sent_bytes,
                update.record.recv_bytes,
                update.record.sent_kbs,
                update.record.recv_kbs
            };

            (*networkStatusSnapshot)[update.record.pid] = status;
        }
    }

    // Update ProcessItem's network status.
    for (ListItem *item : items) {
        ProcessItem *processItem = static_cast<ProcessItem*>(item);
        if (networkStatusSnapshot->contains(processItem->getPid())) {
            processItem->setNetworkStatus(networkStatusSnapshot->value(processItem->getPid()));
        }

        ProcPidIO pidIO;
        getProcPidIO(processItem->getPid(), pidIO);

        DiskStatus status = {0, 0};

        if (processWriteKbs->contains(processItem->getPid())) {
            status.writeKbs = (pidIO.wchar - processWriteKbs->value(processItem->getPid())) / (updateDuration / 1000.0);
        }
        (*processWriteKbs)[processItem->getPid()] = pidIO.wchar;

        if (processReadKbs->contains(processItem->getPid())) {
            status.readKbs = (pidIO.rchar - processReadKbs->value(processItem->getPid())) / (updateDuration / 1000.0);
        }
        (*processReadKbs)[processItem->getPid()] = pidIO.rchar;

        processItem->setDiskStatus(status);
    }

    // Update cpu status.
    updateCpuStatus(totalCpuPercent / cpuNumber);

    // Update process status.
    updateProcessStatus(items);

    // Update network status.
    updateNetworkStatus(totalRecvBytes, totalSentBytes, totalRecvKbs, totalSentKbs);

    // Keep processes we've read for cpu calculations next cycle.
    prevProcesses = processes;
}
