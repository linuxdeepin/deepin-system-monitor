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

    processView->addItems(items);

    // keep processes we've read for cpu calculations next cycle
    prevProcs = processes;
}
