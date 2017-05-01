#ifndef STATUSMONITOR_H
#define STATUSMONITOR_H

#include <QWidget>

class StatusMonitor : public QWidget
{
    Q_OBJECT
    
public:
    StatusMonitor(QWidget *parent = 0);
    
protected:
    void paintEvent(QPaintEvent *event);
};

#endif
