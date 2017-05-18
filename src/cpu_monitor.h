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
    void updateStatus(double cpuPercent);
    void render();
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QPainterPath cpuPath;
    QList<double> *cpuPercents;
    
    QTimer *timer;
    
    int pointsNumber = 24;
    
    int renderOffsetY = 150;
};

#endif    
