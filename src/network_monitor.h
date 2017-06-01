#ifndef NETWORKMONITOR_H
#define NETWORKMONITOR_H

#include <QWidget>

class NetworkMonitor : public QWidget
{
    Q_OBJECT
    
public:
    NetworkMonitor(QWidget *parent = 0);
    
public slots:
    void updateStatus(uint32_t totalRecvBytes, uint32_t totalSentBytes, float totalRecvKbs, float totalSentKbs);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    uint32_t totalSentBytes;
    uint32_t totalRecvBytes;
    float totalSentKbs;
    float totalRecvKbs;
    
    int iconRenderOffsetX = -5;
    int iconRenderOffsetY = 10;
    
    int titleRenderOffsetX = 20;
    
    int titleRenderSize = 20;
    
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 9;
    int pointerRadius = 3;
    
    QString downloadColor = "#E14300";
    int downloadRenderPaddingX = 13;
    int downloadRenderPaddingY = 50;
    int downloadRenderSize = 9;
    
    QString uploadColor = "#004EEF";
    int uploadRenderPaddingX = 13;
    int uploadRenderPaddingY = 70;
    int uploadRenderSize = 9;
    
    int pointsNumber = 50;
    
    QPainterPath downloadPath;
    QList<double> *downloadSpeeds;
    int downloadRenderMaxHeight = 30;
    int downloadWaveformsRenderOffsetX = 0;
    int downloadWaveformsRenderOffsetY = 150;

    QPainterPath uploadPath;
    QList<double> *uploadSpeeds;
    int uploadRenderMaxHeight = 20;
    int uploadWaveformsRenderOffsetX = 0;
    int uploadWaveformsRenderOffsetY = -5;

    QImage iconImage;
};

#endif    
