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
    
public slots:
    void updateProcesses();
    
private:
    ProcessView *processView;
    
    storedProcType prevProcs;
    unsigned long long totalCpuTime;
    
    QTimer *updateTimer;
};

#endif
