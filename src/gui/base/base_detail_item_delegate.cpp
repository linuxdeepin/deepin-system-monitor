#include "base_detail_item_delegate.h"
#include <DPalette>
#include <DStyle>

#include <QHelpEvent>
#include <QAbstractItemView>
#include <QToolTip>
#include <QStyleOptionViewItem>

DWIDGET_USE_NAMESPACE
BaseDetailItemDelegate::BaseDetailItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

bool BaseDetailItemDelegate::helpEvent(QHelpEvent *e, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (!e || !view)
        return false;

    // only process tooltip events for now
    if (e->type() == QEvent::ToolTip) {
        QRect rect = view->visualRect(index);
        QRect textRect = rect;

        // initialize style option
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        // content margin
        auto margin = DStyle::pixelMetric(view->style(), DStyle::PM_ContentsMargins);
        // content spacing
        auto spacing = DStyle::pixelMetric(view->style(), DStyle::PM_ContentsSpacing);
        // icon size
        auto iconSize = view->style()->pixelMetric(DStyle::PM_ListViewIconSize);

        // calc textRect's size (exclude any decoration region)
        if (index.column() == 0) {
            if (opt.features & QStyleOptionViewItem::HasDecoration) {
                textRect.setX(textRect.x() + margin * 2 + spacing + iconSize);
            } else {
                textRect.setX(textRect.x() + 2 * margin);
            }
        } else {
            textRect.setX(textRect.x() + margin);
        }

        if (rect.x() + rect.width() >= view->width()) {
            textRect.setWidth(textRect.width() - margin * 2);
        } else {
            textRect.setWidth(textRect.width() - margin);
        }

        // max width of current text calculated with current option's font metric
        QFontMetrics fm(opt.font);
        int w = fm.size(Qt::TextSingleLine, opt.text).width();

        // show tooptip text when text is truncated (text width is larger than visible region)
        if (textRect.width() < w) {
            QVariant tooltip = index.data(Qt::DisplayRole);
            if (tooltip.canConvert<QString>()) {
                QToolTip::showText(e->globalPos(),
                                   QString("<div>%1</div>").arg(tooltip.toString().toHtmlEscaped()),
                                   view);
                return true;
            }
        }
        if (!QStyledItemDelegate::helpEvent(e, view, option, index))
            QToolTip::hideText();
        return true;
    }

    return QStyledItemDelegate::helpEvent(e, view, option, index);
}
