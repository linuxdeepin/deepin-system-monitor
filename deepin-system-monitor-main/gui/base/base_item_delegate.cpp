// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#include "base_item_delegate.h"
#include "process/process_icon_cache.h"
#include "ddlog.h"

#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif
#include <DPalette>
#include <DStyle>
#include <DStyleHelper>

#include <QAbstractItemView>
#include <QDebug>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QToolTip>

using namespace DDLog;
DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
// content margin
const int margin = 10;
// content spacing
const int spacing = 10;

// constructor
BaseItemDelegate::BaseItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    qCDebug(app) << "BaseItemDelegate constructor";
}

// paint method for this delegate
void BaseItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    // qCDebug(app) << "BaseItemDelegate paint: painting item at row" << index.row() << "column" << index.column();
    // check index validity
    if (!index.isValid()) {
        // qCDebug(app) << "BaseItemDelegate paint: Invalid index, calling superclass paint.";
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(1);

    // initialize style option for drawing
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

#ifdef ENABLE_INACTIVE_DISPLAY
    QWidget *wnd = DApplication::activeWindow();
#endif
    DPalette::ColorGroup cg;
    if (!(opt.state & DStyle::State_Enabled)) {
        // qCDebug(app) << "BaseItemDelegate paint: State is disabled.";
        cg = DPalette::Disabled;
    } else {
        // qCDebug(app) << "BaseItemDelegate paint: State is enabled.";
#ifdef ENABLE_INACTIVE_DISPLAY
        if (!wnd) {
            // qCDebug(app) << "BaseItemDelegate paint: No active window, using Inactive color group.";
            cg = DPalette::Inactive;
        } else {
            if (wnd->isModal()) {
                // qCDebug(app) << "BaseItemDelegate paint: Active window is modal, using Inactive color group.";
                cg = DPalette::Inactive;
            } else {
                // qCDebug(app) << "BaseItemDelegate paint: Using Active color group.";
                cg = DPalette::Active;
            }
        }
#else
        cg = DPalette::Active;
//        cg = DPalette::Current;
#endif
    }

    // DStyle instance
    auto *style = dynamic_cast<DStyle *>(DApplication::style());
    // icon size
    auto iconSize = style->pixelMetric(DStyle::PM_ListViewIconSize, &option);
    // qCDebug(app) << "BaseItemDelegate paint: Icon size is" << iconSize;

    // global palette
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto palette = DApplicationHelper::instance()->applicationPalette();
#else
    auto palette = DGuiApplicationHelper::instance()->applicationPalette();
#endif

    QPen forground;
    if (index.data(Qt::UserRole + 2).isValid()) {
        // user provided text color (custom color used in treeview)
        // qCDebug(app) << "BaseItemDelegate paint: Using user-provided text color.";
        forground.setColor(palette.color(cg, static_cast<DPalette::ColorType>(index.data(Qt::UserRole + 2).toInt())));
    } else {
        // default text color
        // qCDebug(app) << "BaseItemDelegate paint: Using default text color.";
        forground.setColor(palette.color(cg, DPalette::Text));
    }
    // adjust forground color only when in enabled state
    if (opt.state & DStyle::State_Enabled) {
        if (opt.state & DStyle::State_Selected) {
            // qCDebug(app) << "BaseItemDelegate paint: Item is selected.";
            // selected row's color
            forground.setColor(palette.color(cg, DPalette::TextLively));
            if (opt.state & DStyle::State_Sunken) {
                // qCDebug(app) << "BaseItemDelegate paint: Item is sunken.";
                // color used when current row pressed by mouse
                auto hlColor = opt.palette.highlight().color();
                hlColor.setAlphaF(.1);
                auto newColor = style->adjustColor(forground.color(), 0, 0, 0, 0, 0, 0, -40);
                forground = style->blendColor(newColor, hlColor);
            } else if (opt.state & DStyle::State_MouseOver) {
                // qCDebug(app) << "BaseItemDelegate paint: Mouse is over selected item.";
                // color used when current row hovered by mouse
                forground = style->adjustColor(forground.color(), 0, 0, 20);
            }
        } else {
            // color used when it's normal row & hovered by mouse
            if (opt.state & DStyle::State_MouseOver) {
                // qCDebug(app) << "BaseItemDelegate paint: Mouse is over normal item.";
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                auto type = DApplicationHelper::instance()->themeType();
                forground = style->adjustColor(forground.color(), 0, 0, type == DApplicationHelper::DarkType ? 20 : -50);
#else
                auto type = DGuiApplicationHelper::instance()->themeType();
                forground = style->adjustColor(forground.color(), 0, 0, type == DGuiApplicationHelper::ColorType::DarkType ? 20 : -50);
#endif
            }
        }
    }

    QRect rect = opt.rect;
    QFontMetrics fm(opt.font);
    QRect textRect = rect;

    // adjust left/right most column's left/right margin
    switch (opt.viewItemPosition) {
    case QStyleOptionViewItem::Beginning: {
        // qCDebug(app) << "BaseItemDelegate paint: Item is at the beginning.";
        rect.setX(rect.x() + margin);  // left margin
    } break;
    case QStyleOptionViewItem::Middle: {
        // qCDebug(app) << "BaseItemDelegate paint: Item is in the middle.";
        // whole rect
    } break;
    case QStyleOptionViewItem::End: {
        // qCDebug(app) << "BaseItemDelegate paint: Item is at the end.";
        rect.setWidth(rect.width() - margin);  // right margin
    } break;
    case QStyleOptionViewItem::OnlyOne: { // when view has only one column
        // qCDebug(app) << "BaseItemDelegate paint: Item is the only one.";
        rect.setX(rect.x() + margin);
        rect.setWidth(rect.width() - margin);
    } break;
    default: {
        // qCDebug(app) << "BaseItemDelegate paint: Unknown item position, calling superclass paint.";
        painter->restore();
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    }

    QString text;
    QRect iconRect;
    if (opt.viewItemPosition == QStyleOptionViewItem::Beginning ||
            opt.viewItemPosition == QStyleOptionViewItem::OnlyOne) {
        // left most column or when only has one column
        if (opt.features & QStyleOptionViewItem::HasDecoration) {
            // qCDebug(app) << "BaseItemDelegate paint: Item has decoration.";
            // icon decoration needed
            textRect = rect;
            // | margin - icon - spacing - text - margin |
            textRect.setX(textRect.x() + margin + spacing + iconSize);
            textRect.setWidth(textRect.width() - margin);
            text = fm.elidedText(opt.text, opt.textElideMode, textRect.width());

            iconRect = rect;
            iconRect.setX(rect.x() + margin);
            iconRect.setWidth(iconSize);
        } else {
            // qCDebug(app) << "BaseItemDelegate paint: Item has no decoration.";
            // no icon decoration needed
            // | margin - text - margin |
            textRect = rect;
            textRect.setX(textRect.x() + margin);
            textRect.setWidth(textRect.width() - margin);
            text = fm.elidedText(opt.text, opt.textElideMode, textRect.width());
        }
    } else {
        // | margin - text - margin |
        // qCDebug(app) << "BaseItemDelegate paint: Adjusting text rect for non-first column.";
        textRect = rect;
        textRect.setX(textRect.x() + margin);
        textRect.setWidth(textRect.width() - margin);
        text = fm.elidedText(opt.text, opt.textElideMode, textRect.width());
    }

    // draw icon when decoration needed
    if (opt.features & QStyleOptionViewItem::HasDecoration &&
            (opt.viewItemPosition == QStyleOptionViewItem::Beginning ||
             opt.viewItemPosition == QStyleOptionViewItem::OnlyOne)) {
        // qCDebug(app) << "BaseItemDelegate paint: Drawing icon.";
        // vmargins between icon and edge
        const QIcon &icon = opt.icon;
        auto diff = (iconRect.height() - iconSize) / 2;
        iconRect.adjust(0, diff, 0, -diff);

        // Qt的QIcon::pixmap()接口返回QPixmap对象的内存不会回收，此处使用QPixmapCache类来管理进程图标内存资源
        // 原有QPixmapCache::find接口返回的pixmap指针为空，改为返回pixmap引用对象，保证能根据进程名从缓存中获取到图标资源
        // https://pms.uniontech.com/bug-view-239575.html
        const QString &procName = index.data(Qt::UserRole + 4).toString();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QPixmap iconPixmap;
        core::process::ProcessIconCache::instance()->iconPixmapCache.find(procName, iconPixmap);
        if (!iconPixmap.isNull()) {
            // qCDebug(app) << "BaseItemDelegate paint: Drawing icon from cache for process" << procName;
            painter->drawPixmap(iconRect, iconPixmap);
        } else {
            // qCDebug(app) << "BaseItemDelegate paint: Icon not in cache, generating and drawing for process" << procName;
            const QPixmap &iconPix = icon.pixmap(iconRect.size());
            core::process::ProcessIconCache::instance()->iconPixmapCache.insert(procName, iconPix);
            painter->drawPixmap(iconRect, iconPix);
        }
#else
        QPixmap iconPixmap;
        if (core::process::ProcessIconCache::instance()->iconPixmapCache.find(procName, &iconPixmap)) {
            // qCDebug(app) << "BaseItemDelegate paint: Drawing icon from cache for process" << procName;
            painter->drawPixmap(iconRect, iconPixmap);
        } else {
            // qCDebug(app) << "BaseItemDelegate paint: Icon not in cache, generating and drawing for process" << procName;
            const QPixmap &iconPix = icon.pixmap(iconRect.size());
            core::process::ProcessIconCache::instance()->iconPixmapCache.insert(procName, iconPix);
            painter->drawPixmap(iconRect, iconPix);
        }
#endif
    }
    // draw content text
    // qCDebug(app) << "BaseItemDelegate paint: Drawing text:" << text;
    painter->setPen(forground);
    painter->drawText(textRect, static_cast<int>(opt.displayAlignment), text);

    // restore painter state
    painter->restore();
    // qCDebug(app) << "BaseItemDelegate paint: Painting finished.";
}

// editor for editing item data, not used here
QWidget *BaseItemDelegate::createEditor(QWidget *, const QStyleOptionViewItem &,
                                        const QModelIndex &) const
{
    // qCDebug(app) << "BaseItemDelegate createEditor: called for index" << index << ", but no editor is created.";
    return nullptr;
}

// size hint for this delegate
QSize BaseItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    qCDebug(app) << "BaseItemDelegate sizeHint";
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(std::max(36, size.height()));
    qCDebug(app) << "BaseItemDelegate sizeHint: returning size" << size;
    return size;
}

// handler for help events
bool BaseItemDelegate::helpEvent(QHelpEvent *e, QAbstractItemView *view,
                                 const QStyleOptionViewItem &option, const QModelIndex &index)
{
    qCDebug(app) << "BaseItemDelegate helpEvent";
    if (!e || !view) {
        qCDebug(app) << "helpEvent: invalid event, view or index";
        return false;
    }

    // only process tooltip events for now
    if (e->type() == QEvent::ToolTip) {
        qCDebug(app) << "helpEvent: ToolTip event";
        QRect rect = view->visualRect(index);
        QRect textRect = rect;

        // initialize style option
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        // icon size
        auto iconSize = view->style()->pixelMetric(DStyle::PM_ListViewIconSize);

        // calc textRect's size (exclude any decoration region)
        if (index.column() == 0) {
            qCDebug(app) << "helpEvent: Calculating textRect for first column.";
            if (opt.features & QStyleOptionViewItem::HasDecoration) {
                qCDebug(app) << "helpEvent: Item has decoration, adjusting textRect.";
                textRect.setX(textRect.x() + margin * 2 + spacing + iconSize);
            } else {
                qCDebug(app) << "helpEvent: Item has no decoration, adjusting textRect.";
                textRect.setX(textRect.x() + 2 * margin);
            }
        } else {
            qCDebug(app) << "helpEvent: Calculating textRect for non-first column.";
            textRect.setX(textRect.x() + margin);
        }

        if (rect.x() + rect.width() >= view->width()) {
            qCDebug(app) << "helpEvent: Item is at view edge, adjusting width.";
            textRect.setWidth(textRect.width() - margin * 2);
        } else {
            textRect.setWidth(textRect.width() - margin);
        }

        // max width of current text calculated with current option's font metric
        QFontMetrics fm(opt.font);
        int w = fm.size(Qt::TextSingleLine, opt.text).width();

        // show tooptip text when text is truncated (text width is larger than visible region)
        if (textRect.width() < w) {
            qCDebug(app) << "helpEvent: text is truncated, show tooltip";
            QVariant tooltip = index.data(Qt::DisplayRole);
            if (tooltip.canConvert<QString>()) {
                qCDebug(app) << "helpEvent: Showing tooltip text.";
                QToolTip::showText(e->globalPos(),
                                   QString("<div>%1</div>").arg(tooltip.toString().toHtmlEscaped()),
                                   view);
                return true;
            }
        }
        if (!QStyledItemDelegate::helpEvent(e, view, option, index)) {
            qCDebug(app) << "helpEvent: Superclass helpEvent returned false, hiding tooltip.";
            QToolTip::hideText();
        }
        return true;
    }

    return QStyledItemDelegate::helpEvent(e, view, option, index);
}

// initialize style option
void BaseItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    qCDebug(app) << "BaseItemDelegate initStyleOption for index" << index;
    option->showDecorationSelected = true;
    bool ok = false;
    // text alignment option
    if (index.data(Qt::TextAlignmentRole).isValid()) {
        uint value = index.data(Qt::TextAlignmentRole).toUInt(&ok);
        option->displayAlignment = static_cast<Qt::Alignment>(value);
        qCDebug(app) << "BaseItemDelegate initStyleOption: TextAlignmentRole is valid, alignment set to" << option->displayAlignment;
    }
    // use default alignment
    if (!ok) {
        option->displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
        qCDebug(app) << "BaseItemDelegate initStyleOption: Using default alignment.";
    }
    // text elide option
    option->textElideMode = Qt::ElideRight;
    // has display role
    option->features = QStyleOptionViewItem::HasDisplay;
    if (index.data(Qt::DisplayRole).isValid()) {
        option->text = index.data().toString();
        qCDebug(app) << "BaseItemDelegate initStyleOption: DisplayRole is valid, text set to" << option->text;
    }

    // check if has decoration role
    if (index.data(Qt::DecorationRole).isValid()) {
        option->features |= QStyleOptionViewItem::HasDecoration;
        option->icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        qCDebug(app) << "BaseItemDelegate initStyleOption: DecorationRole is valid.";
    }
}
