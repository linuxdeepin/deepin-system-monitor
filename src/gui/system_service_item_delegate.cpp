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
    if (!index.isValid())
        return;

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(1);

    int radius = 8;
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QColor bgColor, highlightColor, highlightTextColor;
    if (index.row() % 2 == 0) {
        bgColor = palette.color(DPalette::AlternateBase);
    } else {
        bgColor = palette.color(DPalette::Base);
    }

    QPalette::ColorRole colorRole = DPalette::Foreground;
    if (opt.state & DStyle::State_Enabled) {
        if (opt.state & DStyle::State_Selected) {
            colorRole = DPalette::HighlightedText;
            bgColor = Qt::blue;
        } else {
            colorRole = DPalette::Foreground;
        }
    }

    switch (opt.viewItemPosition) {
        case QStyleOptionViewItem::Beginning: {
            QRect rect = opt.rect;
            rect.setX(rect.x() + 8);  // TODO: content left padding
            QPainterPath path;
            path.moveTo(rect.x() + radius / 2, rect.y());
            path.arcTo(rect.x(), rect.y(), radius, radius, 90, 90);
            path.lineTo(rect.x(), rect.y() + rect.height() - radius / 2);
            path.arcTo(rect.x(), rect.y() + rect.height() - radius, radius, radius, 180, 90);
            path.lineTo(rect.x() + rect.width(), rect.y() + rect.height());
            path.lineTo(rect.x() + rect.width(), rect.y());
            path.closeSubpath();
            // background
            painter->fillPath(path, bgColor);
            // selected item

            // text
            style->drawItemText(painter, rect, static_cast<int>(opt.displayAlignment), palette,
                                opt.state & DStyle::State_Enabled, opt.text, colorRole);
        } break;
        case QStyleOptionViewItem::Middle:
            painter->fillRect(opt.rect, bgColor);
            style->drawItemText(painter, opt.rect, static_cast<int>(opt.displayAlignment), palette,
                                opt.state & DStyle::State_Enabled, opt.text, colorRole);
            break;
        case QStyleOptionViewItem::End: {
            QRect rect = opt.rect;
            rect.setWidth(rect.width() - 8);
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
            style->drawItemText(painter, rect, static_cast<int>(opt.displayAlignment), palette,
                                opt.state & DStyle::State_Enabled, opt.text, colorRole);
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
    option->displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    option->textElideMode = Qt::ElideRight;
    option->features = QStyleOptionViewItem::HasDisplay;
    if (index.row() % 2 == 0)
        option->features |= QStyleOptionViewItem::Alternate;

    switch (index.column()) {
        case SystemServiceTableModel::kSystemServiceNameColumn:
            option->text = index.data().toString();
            break;
        case SystemServiceTableModel::kSystemServiceLoadStateColumn:
            option->text = index.data().toString();
            break;
        case SystemServiceTableModel::kSystemServiceActiveStateColumn:
            option->text = index.data().toString();
            break;
        case SystemServiceTableModel::kSystemServiceSubStateColumn:
            option->text = index.data().toString();
            break;
        case SystemServiceTableModel::kSystemServiceStateColumn:
            option->text = index.data().toString();
            break;
        case SystemServiceTableModel::kSystemServiceDescriptionColumn:
            option->text = index.data().toString();
            break;
        case SystemServiceTableModel::kSystemServicePIDColumn:
            option->text = index.data().toString();
            option->displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
            break;
        default:
            break;
    }
}
