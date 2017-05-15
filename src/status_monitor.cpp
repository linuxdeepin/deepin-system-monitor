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

    plottingData = nullptr;
    
    connect(this, &StatusMonitor::updateMemoryStatus, memoryMonitor, &MemoryMonitor::updateStatus, Qt::QueuedConnection);

    meminfo(); // have procps read the memory

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
    updateCpu();
    updateMemory();
    updateNetwork();
}

void StatusMonitor::updateCpu()
{
    std::vector<cpuStruct> cpuTimes = getCpuTimes();
    if (prevCpuTimes.size() != 0) {
        std::vector<double> cpuPercentages = calculateCpuPercentages(cpuTimes, prevCpuTimes);

        /*for(unsigned int i=0; i<cpuPercentages.size(); i++) {
          std::cout << cpuPercentages.at(i) << std::endl;
          }*/

        if (cpuPlotData.size() == 60) {
            cpuPlotData.pop_front();
        }
        cpuPlotData.push_back(cpuPercentages);
    }
    prevCpuTimes = cpuTimes;

    // construct the qvectors to use to plot
    if (plottingData != nullptr) {
        delete plottingData;
    }
    plottingData = new QVector<QVector<double>>();

    // The data is arranged in vectors but each vector has points that are intended
    // for multiple plots on multi core machines.
    // Seperate out the data by reading the number of doubles in the initial vector
    if (cpuPlotData.size() == 0) {
        return; // obviously we cant read the initial vector if it is blank
    }

    for(unsigned int i=0; i<cpuPlotData.at(0).size(); i++) {
        QVector<double> headingVector;
        headingVector.push_back(cpuPlotData.at(0).at(i));
        plottingData->push_back(headingVector);
    }

    // now that the initial qvectors are filled, we can append the rest of the data
    for(unsigned int i=1; i<cpuPlotData.size(); i++) {
        for(unsigned int j=0; j<cpuPlotData.at(i).size(); j++) {
            (*plottingData)[j].push_back(cpuPlotData.at(i).at(j));
        }
    }

    // there might not be enough data in each array so pad with 0s on the front if so
    for(int i=0; i<plottingData->size(); i++) {
        for(unsigned int j=60 - plottingData->at(i).size(); j>0; j--) {
            (*plottingData)[i].push_front((double)0);
        }
    }

    // qDebug() << *plottingData;
    // emit(updateCpuPlotSIG(plottingData));
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
