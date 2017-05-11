#include "process_manager.h"
#include "QVBoxLayout"
#include <QDebug>
#include <QList>
#include <proc/sysinfo.h>
#include "process_tools.h"
#include "process_item.h"

using namespace processTools;

ProcessManager::ProcessManager(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    processView = new ProcessView();

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
            ProcessItem *item = new ProcessItem(&i.second);
            items << item;
        }
    }

    // Save selection process pids.
    QList<int> *selectionProcessPids = new QList<int>();
    for (ListItem *item:*(processView->selectionItems)) {
        selectionProcessPids->append((static_cast<ProcessItem*>(item))->pid);
    }
    int lastSelectProcessPid = -1;
    if (processView->lastSelectItem != NULL) {
        lastSelectProcessPid = ((static_cast<ProcessItem*>(processView->lastSelectItem)))->pid;
    }
    int renderOffset = processView->renderOffset;
    
    // Update processes.
    processView->clearItems();
    processView->addItems(items);
    
    // Restore selection status with pid.
    QList<ListItem*> *selectionItems = new QList<ListItem*>();
    for (ListItem *item:*(processView->listItems)) {
        int pid = (static_cast<ProcessItem*>(item))->pid;
        if (selectionProcessPids->contains(pid)) {
            selectionItems->append(item);
        }
        
        if (lastSelectProcessPid != -1 && pid == lastSelectProcessPid) {
            processView->lastSelectItem = item;
        }
    }
    processView->clearSelections();
    processView->addSelections(*selectionItems);
    processView->renderOffset = processView->adjustRenderOffset(renderOffset);
    
    processView->repaint();

    // keep processes we've read for cpu calculations next cycle
    prevProcs = processes;
}
