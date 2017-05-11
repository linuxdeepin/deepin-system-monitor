#include "process_manager.h"
#include "QVBoxLayout"
#include <QDebug>
#include <QList>
#include <proc/sysinfo.h>
#include "process_tools.h"
#include "list_view.h"
#include "process_item.h"

using namespace processTools;

ProcessManager::ProcessManager(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    processIconCache = new QMap<QString, QPixmap> ();
    
    processView = new ProcessView();

    QList<SortFunctionPtr> *alorithms = new QList<SortFunctionPtr>();
    alorithms->append(&ProcessManager::sortByName);
    alorithms->append(&ProcessManager::sortByCPU);
    alorithms->append(&ProcessManager::sortByMemory);
    alorithms->append(&ProcessManager::sortByPid);
    processView->setSortAlgorithm(alorithms, 1, true);

    layout->addWidget(processView);

    updateTimer = new QTimer();
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateProcesses()));
    updateTimer->start(2000);

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

    // fill in cpu%
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
    // update the cpu time for next loop
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
    
    // keep processes we've read for cpu calculations next cycle
    prevProcs = processes;
}

bool ProcessManager::sortByName(const ListItem *item1, const ListItem *item2, bool descendingSort) {
    QString name1 = (static_cast<const ProcessItem*>(item1))->name;
    QString name2 = (static_cast<const ProcessItem*>(item2))->name;
    
    bool sortOrder;
    if (name1 == name2) {
        int cpu1 = static_cast<const ProcessItem*>(item1)->cpu;
        int cpu2 = (static_cast<const ProcessItem*>(item2))->cpu;
        
        sortOrder = cpu1 > cpu2;
    } else {
        sortOrder = name1 > name2;
    }
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessManager::sortByCPU(const ListItem *item1, const ListItem *item2, bool descendingSort) {
    int cpu1 = (static_cast<const ProcessItem*>(item1))->cpu;
    int cpu2 = (static_cast<const ProcessItem*>(item2))->cpu;
    
    bool sortOrder;
    if (cpu1 == cpu2) {
        int memory1 = static_cast<const ProcessItem*>(item1)->memory;
        int memory2 = (static_cast<const ProcessItem*>(item2))->memory;
        
        sortOrder = memory1 > memory2;
    } else {
        sortOrder = cpu1 > cpu2;
    }
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessManager::sortByMemory(const ListItem *item1, const ListItem *item2, bool descendingSort) {
    int memory1 = (static_cast<const ProcessItem*>(item1))->memory;
    int memory2 = (static_cast<const ProcessItem*>(item2))->memory;
    
    bool sortOrder;
    if (memory1 == memory2) {
        int cpu1 = static_cast<const ProcessItem*>(item1)->cpu;
        int cpu2 = (static_cast<const ProcessItem*>(item2))->cpu;
        
        sortOrder = cpu1 > cpu2;
    } else {
        sortOrder = memory1 > memory2;
    }
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessManager::sortByPid(const ListItem *item1, const ListItem *item2, bool descendingSort) {
    bool sortOrder = (static_cast<const ProcessItem*>(item1))->pid > (static_cast<const ProcessItem*>(item2))->pid;
    
    return descendingSort ? sortOrder : !sortOrder;
}

