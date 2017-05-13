#ifndef CpuMONITOR_H
#define CpuMONITOR_H

#include <QWidget>

class CpuMonitor : public QWidget
{
    Q_OBJECT
    
public:
    CpuMonitor(QWidget *parent = 0);
    
protected:
    void paintEvent(QPaintEvent *event);
};

#endif    
