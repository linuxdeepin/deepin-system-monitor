#ifndef CpuMONITOR_H
#define CpuMONITOR_H

#include <QList>
#include <QPointF>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class CpuMonitor : public QWidget
{
    Q_OBJECT
    
public:
    CpuMonitor(QWidget *parent = 0);
    
public slots:
    void render();
    void updateStatus(double cpuPercent);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QImage iconImage;
    QList<double> *cpuPercents;
    QPainterPath cpuPath;
    QTimer *timer;
    double animationFrames = 20;
    int animationIndex = 0;
    int cpuRenderMaxHeight = 45;
    int iconPadding = 0;
    int iconRenderOffsetY = 195;
    int percentRenderOffsetY = 157;
    int pointsNumber = 24;
    int ringRadius = 90;
    int ringRenderOffsetY = 100;
    int ringWidth = 8;
    int titleAreaPaddingX = 5;
    int titleRenderOffsetY = 190;
    int waveformsRenderOffsetX = 80;
    int waveformsRenderOffsetY = 110;
};

#endif    
