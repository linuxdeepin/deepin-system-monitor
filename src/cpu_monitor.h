#ifndef CpuMONITOR_H
#define CpuMONITOR_H

#include <QWidget>
#include <QList>
#include <QTimer>
#include <QPointF>
#include <QVBoxLayout>

class CpuMonitor : public QWidget
{
    Q_OBJECT
    
public:
    CpuMonitor(QWidget *parent = 0);
    
public slots:
    void updateStatus(int cpuPercent);
    void render();
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QPainterPath cpuPath;
    QList<int> *cpuPercents;
    
    QTimer *timer;
    
    int pointsNumber = 24;
    
    int renderOffsetIndex;
    
    int renderOffsetY = 150;
};

#endif    
