#include "cpu_monitor.h"
#include <QPainter>
#include <QDebug>

#include "utils.h"
#include "smooth_curve_generator.h"

CpuMonitor::CpuMonitor(QWidget *parent) : QWidget(parent)
{
    setFixedSize(280, 250);

    cpuPercents = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        cpuPercents->append(0);
    }

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(render()));
    timer->start(200);
}

void CpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    Utils::setFontSize(painter, 22);
    painter.setPen(QPen(QColor("#aaaaaa")));

    painter.drawText(QRect(rect().x(),
                           rect().y() + titleRenderOffsetY,
                           rect().width(),
                           30
                         ), Qt::AlignCenter, "CPU");

    Utils::setFontSize(painter, 15);
    painter.setPen(QPen(QColor("#aaaaaa")));
    painter.drawText(QRect(rect().x(),
                           rect().y() + percentRenderOffsetY,
                           rect().width(),
                           30
                         ), Qt::AlignCenter, QString("%1%").arg(QString::number(cpuPercents->last(), 'f', 1)));
    
    Utils::drawLoadingRing(
        painter,
        rect().x() + rect().width() / 2,
        rect().y() + ringRenderOffsetY,
        ringRadius,
        ringWidth,
        300,
        150,
        "#8442FB",
        0.2,
        cpuPercents->last() / 100
        );

    painter.translate(waveformsRenderOffsetX, waveformsRenderOffsetY);
    painter.scale(1, -1);

    painter.setPen(QPen(QColor("#8442FB"), 2));
    painter.drawPath(cpuPath);
}

void CpuMonitor::updateStatus(double cpuPercent)
{
    cpuPercents->append(cpuPercent);

    if (cpuPercents->size() > pointsNumber) {
        cpuPercents->pop_front();
    }
}

void CpuMonitor::render()
{
    QList<QPointF> points;
    for (int i = 0; i < cpuPercents->size(); i++) {
        points.append(QPointF(i * 5, cpuPercents->at(i)));
    }

    cpuPath = SmoothCurveGenerator::generateSmoothCurve(points);

    repaint();
}
