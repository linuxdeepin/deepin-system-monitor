#include "status_monitor.h"
#include <QPainter>

StatusMonitor::StatusMonitor(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(300);
}

void StatusMonitor::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
}
