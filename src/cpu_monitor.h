#ifndef CpuMONITOR_H
#define CpuMONITOR_H

#include <QWidget>
#include <QList>
#include <QPointF>
#include <QVBoxLayout>

class CpuMonitor : public QWidget
{
    Q_OBJECT
    
public:
    CpuMonitor(QWidget *parent = 0);
    
public slots:
    void updateStatus(QList<QPointF> points);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QVBoxLayout *layout;
    
    QPainterPath smoothCurve;
};

#endif    
