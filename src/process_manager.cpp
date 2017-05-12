#include "QVBoxLayout"
#include "list_view.h"
#include "process_item.h"
#include "process_manager.h"
#include "process_tools.h"
#include <QDebug>
#include <QList>
#include <proc/sysinfo.h>

using namespace processTools;

ProcessManager::ProcessManager(QWidget *parent) : QWidget(parent)
{
    // Init widget.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    processView = new ProcessView();
    layout->addWidget(processView);

    // Set sort algorithms.
    QList<SortFunctionPtr> *alorithms = new QList<SortFunctionPtr>();
    alorithms->append(&ProcessManager::sortByName);
    alorithms->append(&ProcessManager::sortByCPU);
    alorithms->append(&ProcessManager::sortByMemory);
    alorithms->append(&ProcessManager::sortByPid);
    processView->setSortAlgorithm(alorithms, 1, true);

    // Init process icon cache.
    processIconCache = new QMap<QString, QPixmap>();
    
    // Add timer to update process information.
    updateTimer = new QTimer();
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateProcesses()));
    updateTimer->start(2000);

    // Update process information when created.
    updateProcesses();
}

void ProcessManager::updateProcesses() {
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
    if (prevProcs.size()>0) {
        // we have previous proc info
        for(auto &newItr:processes) {
            for(auto &prevItr:prevProcs) {
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
            ProcessItem *item = new ProcessItem(&i.second, processIconCache);
            items << item;
        }
    }

    // Init items.
    processView->initItems(items);
    
    // Keep processes we've read for cpu calculations next cycle.
    prevProcs = processes;
}

bool ProcessManager::sortByName(const ListItem *item1, const ListItem *item2, bool descendingSort) {
    // Init.
    QString name1 = (static_cast<const ProcessItem*>(item1))->getName();
    QString name2 = (static_cast<const ProcessItem*>(item2))->getName();
    bool sortOrder;

    // Sort item with cpu if name is same.
    if (name1 == name2) {
        int cpu1 = static_cast<const ProcessItem*>(item1)->getCPU();
        int cpu2 = (static_cast<const ProcessItem*>(item2))->getCPU();
        
        sortOrder = cpu1 > cpu2;
    }
    // Otherwise sort by name.
    else {
        sortOrder = name1 > name2;
    }
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessManager::sortByCPU(const ListItem *item1, const ListItem *item2, bool descendingSort) {
    // Init.
    int cpu1 = (static_cast<const ProcessItem*>(item1))->getCPU();
    int cpu2 = (static_cast<const ProcessItem*>(item2))->getCPU();
    bool sortOrder;
    
    // Sort item with memory if cpu is same.
    if (cpu1 == cpu2) {
        int memory1 = static_cast<const ProcessItem*>(item1)->getMemory();
        int memory2 = (static_cast<const ProcessItem*>(item2))->getMemory();
        
        sortOrder = memory1 > memory2;
    }
    // Otherwise sort by cpu.
    else {
        sortOrder = cpu1 > cpu2;
    }
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessManager::sortByMemory(const ListItem *item1, const ListItem *item2, bool descendingSort) {
    // Init.
    int memory1 = (static_cast<const ProcessItem*>(item1))->getMemory();
    int memory2 = (static_cast<const ProcessItem*>(item2))->getMemory();
    bool sortOrder;
    
    // Sort item with cpu if memory is same.
    if (memory1 == memory2) {
        int cpu1 = static_cast<const ProcessItem*>(item1)->getCPU();
        int cpu2 = (static_cast<const ProcessItem*>(item2))->getCPU();
        
        sortOrder = cpu1 > cpu2;
    }
    // Otherwise sort by memory.
    else {
        sortOrder = memory1 > memory2;
    }
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessManager::sortByPid(const ListItem *item1, const ListItem *item2, bool descendingSort) {
    bool sortOrder = (static_cast<const ProcessItem*>(item1))->getPid() > (static_cast<const ProcessItem*>(item2))->getPid();
    
    return descendingSort ? sortOrder : !sortOrder;
}

