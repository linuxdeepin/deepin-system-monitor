#include "status_monitor.h"
#include <QPainter>

#include "process_item.h"
#include "utils.h"
#include "process_tools.h"
#include <proc/sysinfo.h>
#include <thread>
#include <QDebug>

using namespace cpuTools;
using namespace processTools;

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

    // Init process icon cache.
    processIconCache = new QMap<QString, QPixmap>();

    connect(this, &StatusMonitor::updateMemoryStatus, memoryMonitor, &MemoryMonitor::updateStatus, Qt::QueuedConnection);
    connect(this, &StatusMonitor::updateCpuStatus, cpuMonitor, &CpuMonitor::updateStatus, Qt::QueuedConnection);

    updateStatusTimer = new QTimer();
    connect(updateStatusTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    updateStatusTimer->start(2000);
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

    int cpuNumber = getCpuTimes().size();
    double totalCpuPercent = 0;

    for(auto &i:processes) {
        QString user = (&i.second)->euser;

        double cpu = (&i.second)->pcpu;

        if (user == username) {
            QString name = getProcessName(&i.second);
            int pid = (&i.second)->tid;
            int memory = ((&i.second)->resident - (&i.second)->share) * sysconf(_SC_PAGESIZE);
            QPixmap icon = getProcessIconFromName(name, processIconCache);
            ProcessItem *item = new ProcessItem(icon, name, cpu / cpuNumber, memory, pid, user);
            items << item;
        }

        totalCpuPercent += cpu;
    }

    // Update cpu status.
    updateCpuStatus(totalCpuPercent / cpuNumber);

    // Init process status.
    updateProcessStatus(items);

    // Keep processes we've read for cpu calculations next cycle.
    prevProcesses = processes;

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

    qDebug() << NetworkTrafficFilter::getNetHogsMonitorStatus() << NETHOGS_STATUS_OK;

    NetworkTrafficFilter::Update update;

    qDebug() << "!!!!!!!!!!!!!!!!!!!!";

    sent_kbs = 0;
    recv_kbs = 0;
    while (NetworkTrafficFilter::getRowUpdate(update)) {
        if (update.action != NETHOGS_APP_ACTION_REMOVE) {
            qDebug() << "######## " << update.record.name << update.record.pid << update.record.device_name << update.record.sent_bytes << update.record.recv_bytes << update.record.sent_kbs << update.record.recv_kbs;

            sent_kbs += update.record.sent_kbs;
            recv_kbs += update.record.recv_kbs;
            
            sent_bytes += update.record.sent_bytes;
            recv_bytes += update.record.recv_bytes;
        }
    }
    
    qDebug() << Utils::formatByteCount(sent_bytes) << Utils::formatByteCount(recv_bytes) << Utils::formatBandwidth(sent_kbs) << Utils::formatBandwidth(recv_kbs);
}
