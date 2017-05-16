#include "status_monitor.h"
#include <QPainter>

#include <proc/sysinfo.h>
#include <QDebug>

using namespace cpuTools;

StatusMonitor::StatusMonitor(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(300);

    layout = new QVBoxLayout(this);

    cpuMonitor = new CpuMonitor();
    memoryMonitor = new MemoryMonitor();
    networkMonitor = new NetworkMonitor();

    layout->addWidget(cpuMonitor);
    layout->addWidget(memoryMonitor);
    layout->addWidget(networkMonitor);

    connect(this, &StatusMonitor::updateMemoryStatus, memoryMonitor, &MemoryMonitor::updateStatus, Qt::QueuedConnection);
    connect(this, &StatusMonitor::updateCpuStatus, cpuMonitor, &CpuMonitor::updateStatus, Qt::QueuedConnection);

    updateCpuTimer = new QTimer();
    connect(updateCpuTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    updateCpuTimer->start(2000);
    
    updateStatus();
}

void StatusMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
}

void StatusMonitor::updateStatus()
{
    meminfo(); // have procps read the memory

    updateCpu();
    updateMemory();
    updateNetwork();
}

void StatusMonitor::updateCpu()
{
    std::vector<cpuStruct> cpuTimes = getCpuTimes();
    if (prevCpuTimes.size() != 0) {
        std::vector<double> cpuPercentages = calculateCpuPercentages(cpuTimes, prevCpuTimes);
        
        updateCpuStatus(cpuPercentages);
    }
    
    prevCpuTimes = cpuTimes;
}

void StatusMonitor::updateMemory()
{
    if (kb_swap_total > 0.0)  {
        updateMemoryStatus(kb_main_used * 1024, kb_main_total * 1024, kb_swap_used * 1024, kb_swap_total * 1024);
    } else {
        updateMemoryStatus(kb_main_used * 1024, kb_main_total * 1024, 0, 0);
    }
}

void StatusMonitor::updateNetwork()
{
}
