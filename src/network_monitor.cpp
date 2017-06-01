#include "network_monitor.h"
#include <QPainter>
#include <QDebug>

#include "utils.h"
#include "smooth_curve_generator.h"

using namespace Utils;

NetworkMonitor::NetworkMonitor(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(180);
    
    iconImage = QImage(Utils::getQrcPath("icon_network.png"));
    
    downloadSpeeds = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        downloadSpeeds->append(0);
    }

    uploadSpeeds = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        uploadSpeeds->append(0);
    }
}

void NetworkMonitor::paintEvent(QPaintEvent *) 
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    setFontSize(painter, 20);
    painter.setPen(QPen(QColor("#aaaaaa")));
    
    // Draw icon.
    painter.drawImage(QPoint(iconRenderOffsetX, iconRenderOffsetY), iconImage);
    
    // Draw title.
    QFont font = painter.font() ;
    font.setPointSize(titleRenderSize);
    font.setWeight(QFont::Light);
    painter.setFont(font);
    painter.setPen(QPen(QColor("#ffffff")));
    painter.drawText(QRect(rect().x() + titleRenderOffsetX, rect().y(), rect().width() - titleRenderOffsetX, rect().height()), Qt::AlignLeft | Qt::AlignTop, "网络");
    
    // Draw network summary.
    painter.setPen(QPen(QColor(downloadColor)));
    painter.setBrush(QBrush(QColor(downloadColor)));
    painter.drawEllipse(QPointF(rect().x() + pointerRenderPaddingX, rect().y() + downloadRenderPaddingY + pointerRenderPaddingY), pointerRadius, pointerRadius);
    
    setFontSize(painter, downloadRenderSize);
    painter.setPen(QPen(QColor("#666666")));
    painter.drawText(QRect(rect().x() + downloadRenderPaddingX,
                           rect().y() + downloadRenderPaddingY,
                           rect().width(),
                           rect().height()),
                     Qt::AlignLeft | Qt::AlignTop,
                     QString("下载速度 %1 累计下载 %2").arg(formatBandwidth(totalRecvKbs)).arg(formatByteCount(totalRecvBytes))
        );

    painter.setPen(QPen(QColor(uploadColor)));
    painter.setBrush(QBrush(QColor(uploadColor)));
    painter.drawEllipse(QPointF(rect().x() + pointerRenderPaddingX, rect().y() + uploadRenderPaddingY + pointerRenderPaddingY), pointerRadius, pointerRadius);
    
    setFontSize(painter, uploadRenderSize);
    painter.setPen(QPen(QColor("#666666")));
    painter.drawText(QRect(rect().x() + uploadRenderPaddingX,
                           rect().y() + uploadRenderPaddingY,
                           rect().width(),
                           rect().height()),
                     Qt::AlignLeft | Qt::AlignTop,
                     QString("上传速度 %1 累计上传 %2").arg(formatBandwidth(totalSentKbs)).arg(formatByteCount(totalSentBytes))
        );
    
    painter.translate(downloadWaveformsRenderOffsetX, downloadWaveformsRenderOffsetY);
    painter.scale(1, -1);

    painter.setPen(QPen(QColor(downloadColor), 1.6));
    painter.setBrush(QBrush());
    painter.drawPath(downloadPath);

    painter.translate(uploadWaveformsRenderOffsetX, uploadWaveformsRenderOffsetY);
    painter.scale(1, -1);

    painter.setPen(QPen(QColor(uploadColor), 1.6));
    painter.setBrush(QBrush());
    painter.drawPath(uploadPath);
}

void NetworkMonitor::updateStatus(uint32_t tRecvBytes, uint32_t tSentBytes, float tRecvKbs, float tSentKbs)
{
    totalRecvBytes = tRecvBytes;
    totalSentBytes = tSentBytes;
    totalRecvKbs = tRecvKbs;
    totalSentKbs = tSentKbs;
    
    // Init download path.
    downloadSpeeds->append(totalRecvKbs);

    if (downloadSpeeds->size() > pointsNumber) {
        downloadSpeeds->pop_front();
    }

    QList<QPointF> downloadPoints;

    double downloadMaxHeight = 0;
    for (int i = 0; i < downloadSpeeds->size(); i++) {
        if (downloadSpeeds->at(i) > downloadMaxHeight) {
            downloadMaxHeight = downloadSpeeds->at(i);
        }
    }

    for (int i = 0; i < downloadSpeeds->size(); i++) {
        if (downloadMaxHeight < downloadRenderMaxHeight) {
            downloadPoints.append(QPointF(i * 5, downloadSpeeds->at(i)));
        } else {
            downloadPoints.append(QPointF(i * 5, downloadSpeeds->at(i) * 45 / downloadMaxHeight));
        }
    }

    downloadPath = SmoothCurveGenerator::generateSmoothCurve(downloadPoints);
    
    // Init upload path.
    uploadSpeeds->append(totalSentKbs);

    if (uploadSpeeds->size() > pointsNumber) {
        uploadSpeeds->pop_front();
    }

    QList<QPointF> uploadPoints;

    double uploadMaxHeight = 0;
    for (int i = 0; i < uploadSpeeds->size(); i++) {
        if (uploadSpeeds->at(i) > uploadMaxHeight) {
            uploadMaxHeight = uploadSpeeds->at(i);
        }
    }

    for (int i = 0; i < uploadSpeeds->size(); i++) {
        if (uploadMaxHeight < uploadRenderMaxHeight) {
            uploadPoints.append(QPointF(i * 5, uploadSpeeds->at(i)));
        } else {
            uploadPoints.append(QPointF(i * 5, uploadSpeeds->at(i) * 45 / uploadMaxHeight));
        }
    }

    uploadPath = SmoothCurveGenerator::generateSmoothCurve(uploadPoints);
    
    repaint();
}
