#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>
#include <DStyleHelper>
#include <QDebug>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>

#include "main_window.h"
#include "model/system_service_table_model.h"
#include "system_service_item_delegate.h"

DWIDGET_USE_NAMESPACE

SystemServiceItemDelegate::SystemServiceItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void SystemServiceItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    if (!index.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(1);

    QWidget *wnd = DApplication::activeWindow();
    DPalette::ColorGroup cg;
    if (!wnd) {
        cg = DPalette::Inactive;
    } else {
        cg = DPalette::Active;
    }

    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());

    int radius = 8;
    radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);
    if (!radius % 2)
        radius += 1;

    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QColor bgColor, highlightColor, highlightTextColor;
    if (opt.features & QStyleOptionViewItem::Alternate) {
        bgColor = palette.color(cg, DPalette::AlternateBase);
    } else {
        bgColor = palette.color(cg, DPalette::Base);
    }

    QPalette::ColorRole colorRole = DPalette::Foreground;
    if (opt.state & DStyle::State_Enabled) {
        if (opt.state & DStyle::State_Selected) {
            colorRole = DPalette::HighlightedText;
            bgColor = palette.color(cg, DPalette::Highlight);
        } else {
            colorRole = DPalette::Foreground;
        }
    }

    QRect rect = opt.rect;

    QFontMetrics fm(opt.font);

    switch (opt.viewItemPosition) {
        case QStyleOptionViewItem::Beginning: {
            rect.setX(rect.x() + margin);  // left margin
            QPainterPath path;
            path.moveTo(rect.x() + radius / 2, rect.y());
            path.arcTo(rect.x(), rect.y(), radius, radius, 90, 90);
            path.lineTo(rect.x(), rect.y() + rect.height() - radius / 2);
            path.arcTo(rect.x(), rect.y() + rect.height() - radius, radius, radius, 180, 90);
            path.lineTo(rect.x() + rect.width(), rect.y() + rect.height());
            path.lineTo(rect.x() + rect.width(), rect.y());
            path.closeSubpath();
            painter->fillPath(path, bgColor);

            QRect textRect = rect;
            textRect.setX(textRect.x() + margin);
            textRect.setWidth(textRect.width() - margin * 2);
            QString text = fm.elidedText(opt.text, opt.textElideMode, textRect.width());
            style->drawItemText(painter, textRect, static_cast<int>(opt.displayAlignment), palette,
                                opt.state & DStyle::State_Enabled, text, colorRole);
        } break;
        case QStyleOptionViewItem::Middle: {
            painter->fillRect(rect, bgColor);

            QRect textRect = rect;
            textRect.setX(textRect.x() + margin);
            textRect.setWidth(textRect.width() - margin * 2);
            QString text = fm.elidedText(opt.text, opt.textElideMode, textRect.width());
            if (cg == DPalette::Active)
                style->drawItemText(painter, textRect, static_cast<int>(opt.displayAlignment),
                                    palette, opt.state & DStyle::State_Enabled, text, colorRole);
            //            else {
            //                style->drawItemText(painter, textRect,
            //                static_cast<int>(opt.displayAlignment),
            //                                    palette, false, text, colorRole);
        }
    }
    break;
    case QStyleOptionViewItem::End: {
        rect.setWidth(rect.width() - margin);  // right margin
        QPainterPath path;
        path.moveTo(rect.x(), rect.y());
        path.lineTo(rect.x(), rect.y() + rect.height());
        path.lineTo(rect.x() + rect.width() - radius / 2, rect.y() + rect.height());
        path.arcTo(rect.x() + rect.width() - radius, rect.y() + rect.height() - radius, radius,
                   radius, 270, 90);
        path.lineTo(rect.x() + rect.width(), rect.y() + radius / 2);
        path.arcTo(rect.x() + rect.width() - radius, rect.y(), radius, radius, 0, 90);
        path.closeSubpath();
        painter->fillPath(path, bgColor);

        QRect textRect = rect;
        textRect.setX(textRect.x() + margin);
        textRect.setWidth(textRect.width() - margin * 2);
        QString text = fm.elidedText(opt.text, opt.textElideMode, textRect.width());
        style->drawItemText(painter, textRect, static_cast<int>(opt.displayAlignment), palette,
                            opt.state & DStyle::State_Enabled, text, colorRole);
    } break;
    default:
        break;
}

painter->restore();
}

QWidget *SystemServiceItemDelegate::createEditor(QWidget *, const QStyleOptionViewItem &,
                                                 const QModelIndex &) const
{
    return nullptr;
}

QSize SystemServiceItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(std::max(36, size.height()));
    return size;
}

void SystemServiceItemDelegate::initStyleOption(QStyleOptionViewItem *option,
                                                const QModelIndex &index) const
{
    option->showDecorationSelected = true;
    bool ok = false;
    if (index.data(Qt::TextAlignmentRole).isValid()) {
        uint value = index.data(Qt::TextAlignmentRole).toUInt(&ok);
        option->displayAlignment = static_cast<Qt::Alignment>(value);
    }
    if (!ok)
        option->displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    option->textElideMode = Qt::ElideRight;
    option->features = QStyleOptionViewItem::HasDisplay;
    if (index.row() % 2 == 0)
        option->features |= QStyleOptionViewItem::Alternate;
    if (index.data(Qt::DisplayRole).isValid())
        option->text = index.data().toString();
}
