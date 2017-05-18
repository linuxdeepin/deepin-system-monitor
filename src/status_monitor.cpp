#include "status_monitor.h"
#include <QPainter>

#include "process_item.h"
#include "process_tools.h"
#include <proc/sysinfo.h>
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
    for(auto &i:processes) {
        QString user = (&i.second)->euser;

        if (user == username) {
            QString name = getProcessName(&i.second);
            int cpu = (&i.second)->pcpu;
            int pid = (&i.second)->tid;
            int memory = ((&i.second)->resident - (&i.second)->share) * sysconf(_SC_PAGESIZE);
            QPixmap icon = getProcessIconFromName(name, processIconCache);
            ProcessItem *item = new ProcessItem(icon, name, cpu, memory, pid, user);
            items << item;
        }
    }

    // Init items.
    updateProcessStatus(items);
    
    // Keep processes we've read for cpu calculations next cycle.
    prevProcesses = processes;
    
    // Have procps read the memory。
    meminfo();                  

    // Update cpu status.
    std::vector<cpuStruct> cpuTimes = getCpuTimes();
    if (prevCpuTimes.size() != 0) {
        std::vector<double> cpuPercentages = calculateCpuPercentages(cpuTimes, prevCpuTimes);
        
        updateCpuStatus(cpuPercentages);
    }
    
    prevCpuTimes = cpuTimes;
    
    // Update memory status.
    if (kb_swap_total > 0.0)  {
        updateMemoryStatus(kb_main_used * 1024, kb_main_total * 1024, kb_swap_used * 1024, kb_swap_total * 1024);
    } else {
        updateMemoryStatus(kb_main_used * 1024, kb_main_total * 1024, 0, 0);
    }
}
