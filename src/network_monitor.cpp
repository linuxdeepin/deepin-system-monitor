#include "network_monitor.h"
#include <QPainter>

#include "utils.h"

NetworkMonitor::NetworkMonitor(QWidget *parent) : QWidget(parent)
{
}

void NetworkMonitor::paintEvent(QPaintEvent *) 
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    Utils::setFontSize(painter, 20);
    painter.setPen(QPen(QColor("#aaaaaa")));
    
    painter.drawText(QRect(rect()), Qt::AlignLeft | Qt::AlignTop, "网络");
}
