#include "cpu_monitor.h"
#include <QPainter>

#include "utils.h"

CpuMonitor::CpuMonitor(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    cpuPlot = new QCustomPlot();
    cpuPlot->setBackground(QBrush(QColor("#202020")));
    cpuPlot->setFixedSize(280, 100);
    cpuPlot->yAxis->setVisible(false);
    cpuPlot->xAxis->setVisible(false);
    
    layout->addWidget(cpuPlot);
}

void CpuMonitor::paintEvent(QPaintEvent *) 
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    Utils::setFontSize(painter, 20);
    painter.setPen(QPen(QColor("#aaaaaa")));
    
    painter.drawText(QRect(rect()), Qt::AlignLeft | Qt::AlignTop, "CPU");
}

void CpuMonitor::updateStatus(QVector<QVector<double>> *values)
{
    QVector<double> x(60); // initialize with entries 60..0
    for (int i=59; i>0; --i)
    {
      x[i] = i;
    }

    static bool previouslyPlotted = false;
    int size = values->size();
    if (size == 0) {
        return;
    }

    #define colourNamesLen 4
    const QString colourNames[] = {
        "#00B892", "#DE361E", "#8342F6", "#C7EB01", "#FF00FF", "#2CA7F8", "#D8FF00",
    };
    for(int i=0; i<size; i++) {
        if (!previouslyPlotted) {
            cpuPlot->addGraph();
        } else {
            cpuPlot->graph(i)->data()->clear();
            cpuPlot->graph(i)->setPen(QPen(QColor(colourNames[i % colourNamesLen])));
        }
        cpuPlot->graph(i)->setData(x, values->at(i));
    }
    previouslyPlotted = true;

    cpuPlot->xAxis->setRange(0, 60);
    cpuPlot->yAxis->setRange(0, 100);
    cpuPlot->replot();
}
