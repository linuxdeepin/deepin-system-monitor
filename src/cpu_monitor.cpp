#include "cpu_monitor.h"
#include <QPainter>
#include <QDebug>

#include "utils.h"
#include "smooth_curve_generator.h"

using namespace Utils;

CpuMonitor::CpuMonitor(QWidget *parent) : QWidget(parent)
{
    setFixedSize(280, 250);

    cpuPercents = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        cpuPercents->append(0);
    }

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(render()));
    timer->start(30);
}

void CpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    setFontSize(painter, 22);
    painter.setPen(QPen(QColor("#aaaaaa")));

    painter.drawText(QRect(rect().x(),
                           rect().y() + titleRenderOffsetY,
                           rect().width(),
                           30
                         ), Qt::AlignCenter, "CPU");

    double percent = (cpuPercents->at(cpuPercents->size() - 2) + easeInOut(animationIndex / animationFrames) * (cpuPercents->last() - cpuPercents->at(cpuPercents->size() - 2)));

    setFontSize(painter, 15);
    painter.setPen(QPen(QColor("#aaaaaa")));
    painter.drawText(QRect(rect().x(),
                           rect().y() + percentRenderOffsetY,
                           rect().width(),
                           30
                         ), Qt::AlignCenter, QString("%1%").arg(QString::number(percent, 'f', 1)));

    drawLoadingRing(
        painter,
        rect().x() + rect().width() / 2,
        rect().y() + ringRenderOffsetY,
        ringRadius,
        ringWidth,
        300,
        150,
        "#8442FB",
        0.2,
        percent / 100
        );

    painter.translate(waveformsRenderOffsetX, waveformsRenderOffsetY);
    painter.scale(1, -1);

    painter.setPen(QPen(QColor("#8442FB"), 1.2));
    painter.drawPath(cpuPath);
}

void CpuMonitor::updateStatus(double cpuPercent)
{
    cpuPercents->append(cpuPercent);

    if (cpuPercents->size() > pointsNumber) {
        cpuPercents->pop_front();
    }

    QList<QPointF> points;

    double cpuMaxHeight = 0;
    for (int i = 0; i < cpuPercents->size(); i++) {
        if (cpuPercents->at(i) > cpuMaxHeight) {
            cpuMaxHeight = cpuPercents->at(i);
        }
    }

    for (int i = 0; i < cpuPercents->size(); i++) {
        if (cpuMaxHeight < cpuRenderMaxHeight) {
            points.append(QPointF(i * 5, cpuPercents->at(i)));
        } else {
            points.append(QPointF(i * 5, cpuPercents->at(i) * 45 / cpuMaxHeight));
        }
    }

    cpuPath = SmoothCurveGenerator::generateSmoothCurve(points);

    if (cpuPercents->last() != cpuPercents->at(cpuPercents->size() - 2)) {
        animationIndex = 0;
        timer->start(30);
    }
}

void CpuMonitor::render()
{
    if (animationIndex < animationFrames) {
        animationIndex++;

        repaint();
    } else {
        timer->stop();
    }
}
