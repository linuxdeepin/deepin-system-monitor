#include "memory_monitor.h"
#include <QPainter>

#include "utils.h"

MemoryMonitor::MemoryMonitor(QWidget *parent) : QWidget(parent)
{
}

void MemoryMonitor::paintEvent(QPaintEvent *) 
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    Utils::setFontSize(painter, 20);
    painter.setPen(QPen(QColor("#aaaaaa")));
    
    painter.drawText(QRect(rect()), Qt::AlignLeft | Qt::AlignTop, "内存");
}
