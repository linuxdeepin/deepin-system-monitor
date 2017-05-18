#ifndef STATUSMONITOR_H
#define STATUSMONITOR_H

#include <QWidget>
#include <QPointF>
#include <QTimer>
#include <QMap>
#include <QVBoxLayout>
#include <deque>
#include <proc/readproc.h>

#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "network_monitor.h"
#include "process_item.h"
#include <proc/sysinfo.h>
#include "cpu_tools.h"

class StatusMonitor : public QWidget
{
    Q_OBJECT
    
    typedef std::map<int, proc_t> storedProcType;
    
public:
    StatusMonitor(QWidget *parent = 0);
    ~StatusMonitor();
    
protected:
    void paintEvent(QPaintEvent *event);
                                       
signals:
    void updateCpuStatus(std::vector<double> cpuPercentages);
    void updateMemoryStatus(long usedMemory, long totalMemory, long usedSwap, long totalSwap);
    void updateNetworkStatus();
    void updateProcessStatus(QList<ListItem*> items);
    
public slots:
    void updateStatus();
    
private:
    QVBoxLayout *layout; 
    
    CpuMonitor *cpuMonitor;
    MemoryMonitor *memoryMonitor;
    NetworkMonitor *networkMonitor;
    
    std::vector<cpuTools::cpuStruct> prevCpuTimes;
    
    QMap<QString, QPixmap> *processIconCache;
    QTimer *updateStatusTimer;
    storedProcType prevProcesses;
    unsigned long long totalCpuTime;
};

#endif
