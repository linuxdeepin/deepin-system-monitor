#ifndef MemoryMONITOR_H
#define MemoryMONITOR_H

#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class MemoryMonitor : public QWidget
{
    Q_OBJECT
    
public:
    MemoryMonitor(QWidget *parent = 0);
    
public slots:
    void render();
    void updateStatus(long uMemory, long tMemory, long uSwap, long tSwap);
    
protected:
    QPointF getEndPointerCoordinate(double percent, int r);
    void paintEvent(QPaintEvent *event);
    
    QImage iconImage;
    QString memoryColor = "#00C5C0";
    QString ringBackgroundColor = "#252525";
    QString swapColor = "#FEDF19";
    QTimer *timer;
    QVBoxLayout *layout;
    double animationFrames = 20;
    int animationIndex = 0;
    int iconRenderOffsetX = -5;
    int iconRenderOffsetY = 10;
    int insideRingRadius = 53;
    int lineHeight = 16;
    int memoryPercentRenderSize = 13;
    int memoryRenderPaddingX = 13;
    int memoryRenderPaddingY = 50;
    int memoryRenderSize = 9;
    int outsideRingRadius = 60;
    int pointerRadius = 3;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 9;
    int ringCenterPointerX = 200;
    int ringCenterPointerY = 60;
    int ringWidth = 6;
    int swapRenderPaddingX = 13;
    int swapRenderPaddingY = 90;
    int swapRenderSize = 9;
    int textPadding = 12;
    int titleRenderOffsetX = 20;
    int titleRenderSize = 20;
    long prevUsedMemory;
    long prevUsedSwap;
    long totalMemory;
    long totalSwap;
    long usedMemory;
    long usedSwap;
};

#endif    
