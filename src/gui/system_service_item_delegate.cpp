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

    painter->save();
    painter->setOpacity(1);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QWidget *wnd = DApplication::activeWindow();
    DPalette::ColorGroup cg;
    if (!(opt.state & DStyle::State_Enabled)) {
        cg = DPalette::Disabled;
    } else {
        if (!wnd) {
            cg = DPalette::Inactive;
        } else {
            if (wnd->isModal()) {
                cg = DPalette::Inactive;
            } else {
                cg = DPalette::Active;
            }
        }
    }

    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());

    int radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QBrush background;
    QPen forground;
    if (opt.features & QStyleOptionViewItem::Alternate) {
        background = palette.color(cg, DPalette::AlternateBase);
    } else {
        background = palette.color(cg, DPalette::Base);
    }

    forground.setColor(palette.color(cg, DPalette::Text));
    if (opt.state & DStyle::State_Enabled) {
        if (opt.state & DStyle::State_Selected) {
            background = palette.color(cg, DPalette::Highlight);
            forground.setColor(palette.color(cg, DPalette::HighlightedText));
        }
    }
    painter->setPen(forground);

    QRect rect = opt.rect;
    QFontMetrics fm(opt.font);
    QPainterPath path, clipPath;
    QRect textRect = rect;

    switch (opt.viewItemPosition) {
        case QStyleOptionViewItem::Beginning: {
            rect.setX(rect.x() + margin);  // left margin

            QPainterPath rectPath, roundedPath;
            roundedPath.addRoundedRect(rect.x(), rect.y(), rect.width() * 2, rect.height(), radius,
                                       radius);
            rectPath.addRect(rect.x() + rect.width(), rect.y(), rect.width(), rect.height());
            clipPath = roundedPath.subtracted(rectPath);
            painter->setClipPath(clipPath);
            path.addRect(rect);
        } break;
        case QStyleOptionViewItem::Middle: {
            path.addRect(rect);
        } break;
        case QStyleOptionViewItem::End: {
            rect.setWidth(rect.width() - margin);  // right margin

            QPainterPath rectPath, roundedPath;
            roundedPath.addRoundedRect(rect.x() - rect.width(), rect.y(), rect.width() * 2,
                                       rect.height(), radius, radius);
            rectPath.addRect(rect.x() - rect.width(), rect.y(), rect.width(), rect.height());
            clipPath = roundedPath.subtracted(rectPath);
            painter->setClipPath(clipPath);
            path.addRect(rect);
        } break;
        default: {
            painter->restore();
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }
    }

    textRect = rect;
    textRect.setX(textRect.x() + margin);
    textRect.setWidth(textRect.width() - margin * 2);
    QString text = fm.elidedText(opt.text, opt.textElideMode, textRect.width());

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->fillPath(path, background);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawText(textRect, static_cast<int>(opt.displayAlignment), text);

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
