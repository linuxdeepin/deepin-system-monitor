#ifndef MemoryMONITOR_H
#define MemoryMONITOR_H

#include <QWidget>

class MemoryMonitor : public QWidget
{
    Q_OBJECT
    
public:
    MemoryMonitor(QWidget *parent = 0);
    
protected:
    void paintEvent(QPaintEvent *event);
};

#endif    
