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
    
    int waveformsRenderOffsetX = 80;
    int waveformsRenderOffsetY = 110;
    int ringRenderOffsetY = 100;
    int ringRadius = 90;
    int ringWidth = 9;
    
    int titleRenderOffsetY = 150;
    int percentRenderOffsetY = 180;
    
    int animationIndex = 0;
    double animationFrames = 20;
    int cpuRenderMaxHeight = 45;
};

#endif    
