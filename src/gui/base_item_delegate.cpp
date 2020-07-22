﻿/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "base_item_delegate.h"

#include <DApplication>
#include <DApplicationHelper>
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

DWIDGET_USE_NAMESPACE

BaseItemDelegate::BaseItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void BaseItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (!index.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(1);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

#ifdef ENABLE_INACTIVE_DISPLAY
    QWidget *wnd = DApplication::activeWindow();
#endif
    DPalette::ColorGroup cg;
    if (!(opt.state & DStyle::State_Enabled)) {
        cg = DPalette::Disabled;
    } else {
#ifdef ENABLE_INACTIVE_DISPLAY
        if (!wnd) {
            cg = DPalette::Inactive;
        } else {
            if (wnd->isModal()) {
                cg = DPalette::Inactive;
            } else {
                cg = DPalette::Active;
            }
        }
#else
        cg = DPalette::Active;
//        cg = DPalette::Current;
#endif
    }

    auto *style = dynamic_cast<DStyle *>(DApplication::style());

    auto margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);
    auto spacing = style->pixelMetric(DStyle::PM_ContentsSpacing, &option);
    auto iconSize = style->pixelMetric(DStyle::PM_ListViewIconSize, &option);

    auto palette = DApplicationHelper::instance()->applicationPalette();

    QPen forground;
    if (index.data(Qt::UserRole + 2).isValid()) {
        forground.setColor(palette.color(cg, static_cast<DPalette::ColorType>(index.data(Qt::UserRole + 2).toInt())));
    } else {
        forground.setColor(palette.color(cg, DPalette::Text));
    }
    if (opt.state & DStyle::State_Enabled) {
        if (opt.state & DStyle::State_Selected) {
            forground.setColor(palette.color(cg, DPalette::TextLively));
        }
    }

    QRect rect = opt.rect;
    QFontMetrics fm(opt.font);
    QRect textRect = rect;

    switch (opt.viewItemPosition) {
    case QStyleOptionViewItem::Beginning: {
        rect.setX(rect.x() + margin);  // left margin
    } break;
    case QStyleOptionViewItem::Middle: {
        // whole rect
    } break;
    case QStyleOptionViewItem::End: {
        rect.setWidth(rect.width() - margin);  // right margin
    } break;
    case QStyleOptionViewItem::OnlyOne: {
        rect.setX(rect.x() + margin);
        rect.setWidth(rect.width() - margin);
    } break;
    default: {
        painter->restore();
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    }

    QString text;
    QRect iconRect;
    if (opt.viewItemPosition == QStyleOptionViewItem::Beginning ||
            opt.viewItemPosition == QStyleOptionViewItem::OnlyOne) {
        if (opt.features & QStyleOptionViewItem::HasDecoration) {
            textRect = rect;
            // | margin - icon - spacing - text - margin |
            textRect.setX(textRect.x() + margin + spacing + iconSize);
            textRect.setWidth(textRect.width() - margin);
            text = fm.elidedText(opt.text, opt.textElideMode, textRect.width());

            iconRect = rect;
            iconRect.setX(rect.x() + margin);
            iconRect.setWidth(iconSize);
        } else {
            // | margin - text - margin |
            textRect = rect;
            textRect.setX(textRect.x() + margin);
            textRect.setWidth(textRect.width() - margin);
            text = fm.elidedText(opt.text, opt.textElideMode, textRect.width());
        }
    } else {
        // | margin - text - margin |
        textRect = rect;
        textRect.setX(textRect.x() + margin);
        textRect.setWidth(textRect.width() - margin);
        text = fm.elidedText(opt.text, opt.textElideMode, textRect.width());
    }

    if (opt.features & QStyleOptionViewItem::HasDecoration &&
            (opt.viewItemPosition == QStyleOptionViewItem::Beginning ||
             opt.viewItemPosition == QStyleOptionViewItem::OnlyOne)) {
        auto diff = (iconRect.height() - iconSize) / 2;
        opt.icon.paint(painter, iconRect.adjusted(0, diff, 0, -diff));
    }
    painter->setPen(forground);
    painter->drawText(textRect, static_cast<int>(opt.displayAlignment), text);

    painter->restore();
}

QWidget *BaseItemDelegate::createEditor(QWidget *, const QStyleOptionViewItem &,
                                        const QModelIndex &) const
{
    return nullptr;
}

QSize BaseItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(std::max(36, size.height()));
    return size;
}

bool BaseItemDelegate::helpEvent(QHelpEvent *e, QAbstractItemView *view,
                                 const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (!e || !view)
        return false;

    if (e->type() == QEvent::ToolTip) {
        QRect rect = view->visualRect(index);
        QRect textRect = rect;

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        auto margin = DStyle::pixelMetric(view->style(), DStyle::PM_ContentsMargins);
        auto spacing = DStyle::pixelMetric(view->style(), DStyle::PM_ContentsSpacing);
        auto iconSize = view->style()->pixelMetric(DStyle::PM_ListViewIconSize);

        // textRect
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

        // textWidth
        QFontMetrics fm(opt.font);
        int w = fm.size(Qt::TextSingleLine, opt.text).width();

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

void BaseItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
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
    if (index.data(Qt::DisplayRole).isValid())
        option->text = index.data().toString();

    if (index.data(Qt::DecorationRole).isValid()) {
        option->features |= QStyleOptionViewItem::HasDecoration;
        option->icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    }
}
