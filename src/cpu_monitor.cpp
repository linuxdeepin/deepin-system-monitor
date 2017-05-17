#include "cpu_monitor.h"
#include <QPainter>
#include <QDebug>

#include "utils.h"
#include "smooth_curve_generator.h"

CpuMonitor::CpuMonitor(QWidget *parent) : QWidget(parent)
{
    setFixedSize(280, 300);

    cpuPoints = nullptr;
    cpuPaths = nullptr;

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(render()));
    timer->start(200);
    
    renderOffsetIndex = 0;
}

void CpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    Utils::setFontSize(painter, 20);
    painter.setPen(QPen(QColor("#aaaaaa")));

    painter.drawText(QRect(rect()), Qt::AlignLeft | Qt::AlignTop, "CPU");

    if (cpuPaths != nullptr) {
        painter.translate(0, height() / 2);
        painter.scale(1, -1);

        painter.setClipRect(QRectF(rect().x(), rect().y(), 220, rect().height()));

        const QString colourNames[] = {
            "#CC0000", "#E69138", "#6AA84F", "#3C78D8", "#45818E", "#3D85C6", "#A64D79", "#674EA7",
        };
        for (int i = 0; i < cpuPaths->size(); i++) {
            painter.setPen(QPen(QColor(colourNames[i % colourNames->size()]), 2));
            painter.drawPath(cpuPaths->at(i));
        }
    }
}

void CpuMonitor::initStatus(std::vector<double> cpuPercentages)
{
    cpuPoints = new QList<QList<double>*>();

    for (unsigned int i = 0; i < cpuPercentages.size(); i++) {
        QList<double> *list = new QList<double>();

        for (int j = 0; j < pointsNumber; j++) {
            list->append(0);
        }
        cpuPoints->append(list);
    }

    cpuPaths = new QList<QPainterPath>();
}

void CpuMonitor::updateStatus(std::vector<double> cpuPercentages)
{
    if (cpuPoints == nullptr) {
        initStatus(cpuPercentages);
    }

    for (unsigned int i = 0; i < cpuPercentages.size(); i++) {
        cpuPoints->at(i)->append(cpuPercentages.at(i));

        if (cpuPoints->at(i)->size() > pointsNumber) {
            cpuPoints->at(i)->pop_front();
        }
    }
}

void CpuMonitor::render()
{
    if (cpuPaths != nullptr) {
        cpuPaths->clear();
        
        if (renderOffsetIndex >= 10) {
            renderOffsetIndex = 0;
        }

        for (int i = 0; i < cpuPoints->size(); i++) {
            QList<QPointF> points;

            for (int j = 0; j < cpuPoints->at(i)->size(); j++) {
                points.append(QPointF(j * 10 - renderOffsetIndex, cpuPoints->at(i)->at(j)));
            }

            cpuPaths->append(SmoothCurveGenerator::generateSmoothCurve(points));
        }

        repaint();
        
        renderOffsetIndex++;
    }
}
