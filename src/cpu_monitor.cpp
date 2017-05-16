#include "cpu_monitor.h"
#include <QPainter>
#include <QDebug>

#include "utils.h"
#include "smooth_curve_generator.h"

CpuMonitor::CpuMonitor(QWidget *parent) : QWidget(parent)
{
    setFixedSize(280, 300);
}

void CpuMonitor::paintEvent(QPaintEvent *) 
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    Utils::setFontSize(painter, 20);
    painter.setPen(QPen(QColor("#aaaaaa")));
    
    painter.drawText(QRect(rect()), Qt::AlignLeft | Qt::AlignTop, "CPU");
    
    painter.setPen(QPen(QColor("#ff0000"), 2));
    
    painter.translate(0, height() / 2);
    painter.scale(1, -1);
    
    painter.drawPath(smoothCurve);
}

void CpuMonitor::updateStatus(QList<QPointF> points)
{
    qDebug() << points;
    
    smoothCurve = SmoothCurveGenerator::generateSmoothCurve(points);
    
    repaint();
}
