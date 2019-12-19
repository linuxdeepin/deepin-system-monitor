#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <QColor>
#include <QPainter>
#include <QPixmap>
#include <mutex>
#include <thread>

enum DisplayMode { kDisplayModeCompact = 0, kDisplayModeExpand };

inline QString formatColor(const QColor &color, int base = 10)
{
    if (base == 16) {
        return QString("A:%1 R:%2 G:%3 B:%4")
            .arg(color.alpha(), 0, 16)
            .arg(color.red(), 0, 16)
            .arg(color.green(), 0, 16)
            .arg(color.blue(), 0, 16);
    } else if (base == 10) {
        return QString("A:%1 R:%2 G:%3 B:%4")
            .arg(color.alpha())
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue());
    } else {
        return {};
    }
}

QImage compizDropShadow(QPainter &painter, const QPixmap &pixmap, const QRect &rect, int radius,
                        int distance, const QColor &color);

#endif  // UI_COMMON_H
