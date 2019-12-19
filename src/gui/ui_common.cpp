#include <QGraphicsDropShadowEffect>
#include <QGraphicsEffect>

#include "ui_common.h"

QT_BEGIN_NAMESPACE
extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius,
                                          bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

QImage compizDropShadow(QPainter &painter, const QPixmap &pixmap, const QRect &rect, int radius,
                        int distance, const QColor &color)
{
    // if nothing to show outside the item, just draw source
    if ((radius + distance) <= 0 || pixmap.isNull()) {
        return pixmap.toImage();
    }

    // save world transform
    QTransform saved_trans = painter.worldTransform();
    painter.setWorldTransform(QTransform());

    // Calculate size for the background image
    QSize size(pixmap.width() + 2 * distance, pixmap.height() + 2 * distance);

    QImage tmp(size, QImage::Format_ARGB32_Premultiplied);
    QPixmap scaled = pixmap.scaled(size);
    tmp.fill(0);
    QPainter tmpPainter(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    tmpPainter.drawPixmap(QPointF(-distance, -distance), scaled);
    tmpPainter.end();

    // blur the alpha channel
    QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
    blurred.fill(0);
    QPainter blurPainter(&blurred);
    qt_blurImage(&blurPainter, tmp, radius, false, true);
    blurPainter.end();

    tmp = blurred;

    // colorized shadow
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color);
    tmpPainter.end();

    // draw the blurred shadow...
    //    painter.drawImage(rect, tmp);
    return tmp;

    // draw the actual pixmap...
    //    painter.drawPixmap(rect, pixmap, QRectF());

    // restore world transform
    //    painter.setWorldTransform(saved_trans);
}
