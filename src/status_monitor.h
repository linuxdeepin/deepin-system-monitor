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
#include "network_traffic_filter.h"

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
    void updateCpuStatus(double cpuPercent);
    void updateMemoryStatus(long usedMemory, long totalMemory, long usedSwap, long totalSwap);
    void updateNetworkStatus(uint32_t totalRecvBytes, uint32_t totalSentBytes, float totalRecvKbs, float totalSentKbs);
    void updateProcessStatus(QList<ListItem*> items);

public slots:
    void updateStatus();

private:
    QVBoxLayout *layout;

    CpuMonitor *cpuMonitor;
    MemoryMonitor *memoryMonitor;
    NetworkMonitor *networkMonitor;

    QMap<QString, QPixmap> *processIconCache;
    QMap<int, uint32_t> *processSentBytes;
    QMap<int, uint32_t> *processRecvBytes;
    QMap<int, unsigned long> *processWriteKbs;
    QMap<int, unsigned long> *processReadKbs;
    QTimer *updateStatusTimer;
    storedProcType prevProcesses;
    unsigned long long totalCpuTime;

    uint32_t totalSentBytes;
    uint32_t totalRecvBytes;
    float totalSentKbs;
    float totalRecvKbs;
};

#endif
