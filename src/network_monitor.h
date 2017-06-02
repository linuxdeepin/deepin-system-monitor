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
    QImage iconImage;
    QList<double> *downloadSpeeds;
    QList<double> *uploadSpeeds;
    QPainterPath downloadPath;
    QPainterPath uploadPath;
    QString downloadColor = "#E14300";
    QString uploadColor = "#004EEF";
    float totalRecvKbs;
    float totalSentKbs;
    int downloadRenderMaxHeight = 30;
    int downloadRenderPaddingX = 13;
    int downloadRenderPaddingY = 50;
    int downloadRenderSize = 9;
    int downloadWaveformsRenderOffsetX = 4;
    int downloadWaveformsRenderOffsetY = 136;
    int iconRenderOffsetX = -5;
    int iconRenderOffsetY = 10;
    int pointerRadius = 3;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 9;
    int pointsNumber = 51;
    int textPadding = 100;
    int titleRenderOffsetX = 20;
    int titleRenderSize = 20;
    int uploadRenderMaxHeight = 30;
    int uploadRenderPaddingX = 13;
    int uploadRenderPaddingY = 70;
    int uploadRenderSize = 9;
    int uploadWaveformsRenderOffsetY = -5;
    int waveformRenderPadding = 20;
    int gridRenderOffsetY = 99;
    int gridPaddingTop = 10;
    int gridPaddingRight = 21;
    int gridSize = 20;
    uint32_t totalRecvBytes;
    uint32_t totalSentBytes;
};

#endif    
