#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QWidget>
#include "process_view.h"
#include <proc/readproc.h>

class ProcessManager : public QWidget
{
    typedef std::map<int, proc_t> storedProcType;
    
    Q_OBJECT
    
public:
    ProcessManager(QWidget *parent = 0);
                                       
    static bool sortByName(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByCPU(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByMemory(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByPid(const ListItem *item1, const ListItem *item2, bool descendingSort);
    
public slots:
    void updateProcesses();
    
private:
    ProcessView *processView;
    
    storedProcType prevProcs;
    unsigned long long totalCpuTime;
    
    QTimer *updateTimer;
};

#endif
