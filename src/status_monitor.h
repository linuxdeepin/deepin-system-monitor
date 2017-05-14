#ifndef STATUSMONITOR_H
#define STATUSMONITOR_H

#include <QWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <deque>

#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "network_monitor.h"

#include "cpu_tools.h"

class StatusMonitor : public QWidget
{
    Q_OBJECT
    
public:
    StatusMonitor(QWidget *parent = 0);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private slots:
    void updateStatus();
    
private:
    void updateCpu();
    void updateMemory();
    void updateNetwork();
    
    QVBoxLayout *layout; 
    
    CpuMonitor *cpuMonitor;
    MemoryMonitor *memoryMonitor;
    NetworkMonitor *networkMonitor;
    
    std::vector<cpuTools::cpuStruct> prevCpuTimes;
    std::deque<std::vector<double>> cpuPlotData;
    QVector<QVector<double>> *plottingData;
    
    QTimer *updateCpuTimer;
};

#endif
