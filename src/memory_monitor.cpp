#include "memory_monitor.h"
#include <QPainter>
#include <QtMath>

#include "utils.h"

#include <QDebug>

using namespace Utils;

MemoryMonitor::MemoryMonitor(QWidget *parent) : QWidget(parent)
{
    usedMemory = 0;
    totalMemory = 0;
    usedSwap = 0;
    totalSwap = 0;
    
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(render()));
    timer->start();
    
    setFixedHeight(120);
}

void MemoryMonitor::updateStatus(long uMemory, long tMemory, long uSwap, long tSwap)
{
    if ((convertSizeUnit(uMemory, "") != convertSizeUnit(usedMemory, "")) || 
        (convertSizeUnit(uSwap, "") != convertSizeUnit(usedSwap, ""))) {
        prevUsedMemory = usedMemory;
        prevUsedSwap = usedSwap;
        
        usedMemory = uMemory;
        totalMemory = tMemory;
        usedSwap = uSwap;
        totalSwap = tSwap;

        animationIndex = 0;
        timer->start(30);
    }
}

void MemoryMonitor::render()
{
    if (animationIndex < animationFrames) {
        animationIndex++;

        repaint();
    } else {
        timer->stop();
    }
}

void MemoryMonitor::paintEvent(QPaintEvent *)
{
    // Init.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    double memoryPercent = (prevUsedMemory + easeInOut(animationIndex / animationFrames) * (usedMemory - prevUsedMemory)) * 1.0 / totalMemory;
    double swapPercent;
    if (totalSwap == 0) {
        swapPercent = 0;
    } else {
        swapPercent = (prevUsedSwap + easeInOut(animationIndex / animationFrames) * (usedSwap - prevUsedSwap)) * 1.0 / totalSwap;
    }

    // Draw title.
    setFontSize(painter, titleRenderSize);
    painter.setPen(QPen(QColor("#aaaaaa")));
    painter.drawText(QRect(rect()), Qt::AlignLeft | Qt::AlignTop, "内存");

    // Draw memory summary.
    painter.setPen(QPen(QColor(memoryColor)));
    painter.setBrush(QBrush(QColor(memoryColor)));
    painter.drawEllipse(QPointF(rect().x() + pointerRenderPaddingX, rect().y() + memoryRenderPaddingY + pointerRenderPaddingY), pointerRadius, pointerRadius);

    setFontSize(painter, memoryRenderSize);
    painter.setPen(QPen(QColor("#666666")));
    painter.drawText(QRect(rect().x() + memoryRenderPaddingX,
                           rect().y() + memoryRenderPaddingY,
                           rect().width(),
                           rect().height()),
                     Qt::AlignLeft | Qt::AlignTop,
                     QString("内存(%1%) %2/%3").arg(static_cast<int>(memoryPercent * 100)).arg(convertSizeUnit(usedMemory, "")).arg(convertSizeUnit(totalMemory, "")));

    // Draw swap summary.
    painter.setPen(QPen(QColor(swapColor)));
    painter.setBrush(QBrush(QColor(swapColor)));
    painter.drawEllipse(QPointF(rect().x() + pointerRenderPaddingX, rect().y() + swapRenderPaddingY + pointerRenderPaddingY), pointerRadius, pointerRadius);

    setFontSize(painter, swapRenderSize);
    painter.setPen(QPen(QColor("#666666")));
    QString swapString;
    if (usedSwap == 0 && totalSwap == 0) {
        swapString = "交换空间 (未使用)";
    } else {
        swapString = QString("交换空间 (%1%) %2/%3").arg(static_cast<int>(swapPercent * 100)).arg(convertSizeUnit(usedSwap, "")).arg(convertSizeUnit(totalSwap, ""));
    }
    painter.drawText(QRect(rect().x() + swapRenderPaddingX,
                           rect().y() + swapRenderPaddingY,
                           rect().width(),
                           rect().height()),
                     Qt::AlignLeft | Qt::AlignTop,
                     swapString);

    // Draw memory ring.
    drawLoadingRing(
        painter,
        rect().x() + ringCenterPointerX,
        rect().y() + ringCenterPointerY,
        outsideRingRadius,
        ringWidth,
        270,
        270,
        memoryColor,
        0.2,
        memoryPercent
        );
    
    // Draw swap ring.
    drawLoadingRing(
        painter,
        rect().x() + ringCenterPointerX,
        rect().y() + ringCenterPointerY,
        insideRingRadius,
        ringWidth,
        270,
        270,
        swapColor,
        0.2,
        swapPercent
        );
    

    // Draw percent text.
    setFontSize(painter, memoryPercentRenderSize);
    painter.setPen(QPen(QColor("#aaaaaa")));
    painter.drawText(QRect(rect().x() + ringCenterPointerX - insideRingRadius, rect().y() + ringCenterPointerY - insideRingRadius, insideRingRadius * 2, insideRingRadius * 2),
                     Qt::AlignCenter,
                     QString("%1%").arg(static_cast<int>(memoryPercent * 100)));
}

QPointF MemoryMonitor::getEndPointerCoordinate(double percent, int r)
{
    int angle = 360 - (270 * percent);

    double sinValue = qSin((angle / 360.0) * 2 * M_PI);
    double cosValue = qCos((angle / 360.0) * 2 * M_PI);
    
    int pointerX = rect().x() + ringCenterPointerX + static_cast<int>(r * cosValue) + static_cast<int>(pointerRadius * sinValue);
    int pointerY = rect().y() + ringCenterPointerY - static_cast<int>(r * sinValue) + static_cast<int>(pointerRadius * cosValue);
    
    // I don't why this need adjust 1 pixel, it's weird.
    if (angle > 270) {
        pointerY += 1;
    }

    return QPointF(pointerX, pointerY);
}

