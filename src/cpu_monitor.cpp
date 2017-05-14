#include "cpu_monitor.h"
#include <QPainter>

#include "utils.h"

CpuMonitor::CpuMonitor(QWidget *parent) : QWidget(parent)
{
}

void CpuMonitor::paintEvent(QPaintEvent *) 
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    Utils::setFontSize(painter, 20);
    painter.setPen(QPen(QColor("#aaaaaa")));
    
    painter.drawText(QRect(rect()), Qt::AlignLeft | Qt::AlignTop, "CPU");
}
