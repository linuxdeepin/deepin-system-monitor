#include "process_manager.h"
#include "QVBoxLayout"
#include <QDebug>
#include <proc/sysinfo.h>
#include "process_tools.h"

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
                    newItr.second.pcpu = (unsigned int)calculateCPUPercentage(&prevItr.second,&newItr.second,totalCpuTime);
                    break;
                }
            }
        }
    }
    // update the cpu time for next loop
    totalCpuTime = getTotalCpuTime();
    
    // Read processes information.
    unsigned int index = 0;
    for(auto &i:processes) {
        proc_t* p = (&i.second);
        QString processName = getProcessName(p);
        // QTableWidgetItem* processNameTableItem = new QTableWidgetItem(processName);
        // processNameTableItem->setToolTip(getProcessCmdline(p->tid)); 
        // processNameTableItem->setIcon(getProcessIconFromName(processName,processIconCache));
        // processesTable->setItem(index,0,processNameTableItem);
        QString user = p->euser;
        // processesTable->setItem(index,1,new QTableWidgetItem(user));
        //std::cout << p->pcpu << std::endl;
        QString cpu = QString::number(p->pcpu);
        // processesTable->setItem(index,2,new TableNumberItem(cpu));
        QString id = QString::number(p->tid);
        // processesTable->setItem(index,3,new TableNumberItem(id));
        // gnome-system-monitor measures memory as RSS - shared
        // shared memory is only given as # pages, sysconf(_SC_PAGES) is in bytes
        // so do, (#pages RSS - #pages Share) * _SC_PAGES
        // memoryEntry memory = convertMemoryUnit((p->resident - p->share)*sysconf(_SC_PAGESIZE),memoryUnit::b);
        // processesTable->setItem(index,4,new TableMemoryItem(memory.unit,truncateDouble(memory.id,1)));
        // processesTable->showRow(index);

        // if (shouldHideProcess(p->euid)) {
        //     // hide this row
        //     // processesTable->hideRow(index);
        // }

        // update the row selection to reflect the row that was previously selected
        // if (selectedRowInfoID>0) {
        //     if (selectedRowInfoID == p->tid) {
        //         // this is the same process
        //         // processesTable->selectRow(index);
        //     }
        // }
        
        qDebug() << processName << p->tid << cpu;

        index++;
    }
    
    // keep processes we've read for cpu calculations next cycle
    prevProcs = processes;
}
