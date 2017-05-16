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
    void updateStatus(std::vector<double> cpuPercentages);
    void render();
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QList<QList<double>*> *cpuPoints;
    QList<QPainterPath> *cpuPaths;
    
    QTimer *timer;
    
    int pointsNumber = 40;
};

#endif    
