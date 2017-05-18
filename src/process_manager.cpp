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
    QList<SortAlgorithm> *alorithms = new QList<SortAlgorithm>();
    alorithms->append(&ProcessManager::sortByName);
    alorithms->append(&ProcessManager::sortByCPU);
    alorithms->append(&ProcessManager::sortByMemory);
    alorithms->append(&ProcessManager::sortByPid);
    processView->setColumnSortingAlgorithms(alorithms, 1, true);
}

ProcessManager::~ProcessManager()
{
    delete processView;
}

void ProcessManager::updateStatus(QList<ListItem*> items)
{
    processView->refreshItems(items);
}

bool ProcessManager::sortByName(const ListItem *item1, const ListItem *item2, bool descendingSort) 
{
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

bool ProcessManager::sortByCPU(const ListItem *item1, const ListItem *item2, bool descendingSort) 
{
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

bool ProcessManager::sortByMemory(const ListItem *item1, const ListItem *item2, bool descendingSort) 
{
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

bool ProcessManager::sortByPid(const ListItem *item1, const ListItem *item2, bool descendingSort) 
{
    bool sortOrder = (static_cast<const ProcessItem*>(item1))->getPid() > (static_cast<const ProcessItem*>(item2))->getPid();
    
    return descendingSort ? sortOrder : !sortOrder;
}

