#ifndef STATUSMONITOR_H
#define STATUSMONITOR_H

#include <QWidget>
#include <QVBoxLayout>

#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "network_monitor.h"

class StatusMonitor : public QWidget
{
    Q_OBJECT
    
public:
    StatusMonitor(QWidget *parent = 0);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QVBoxLayout *layout; 
    
    CpuMonitor *cpuMonitor;
    MemoryMonitor *memoryMonitor;
    NetworkMonitor *networkMonitor;
};

#endif
