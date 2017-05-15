#ifndef CpuMONITOR_H
#define CpuMONITOR_H

#include <QWidget>
#include <QVBoxLayout>
#include "qcustomplot.h"

class CpuMonitor : public QWidget
{
    Q_OBJECT
    
public:
    CpuMonitor(QWidget *parent = 0);
    
public slots:
    void updateStatus(QVector<QVector<double>> *values);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QCustomPlot *cpuPlot;
    QVBoxLayout *layout;
};

#endif    
