/*
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

#include "base_table_view.h"

#include "base_header_view.h"
#include "base_item_delegate.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>

#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QHeaderView>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QScroller>
#include <QScrollerProperties>
#include <QScrollBar>

BaseTableView::BaseTableView(DWidget *parent)
    : DTreeView(parent)
{
    m_itemDelegate = new BaseItemDelegate(this);
    setItemDelegate(m_itemDelegate);

    m_headerView = new BaseHeaderView(Qt::Horizontal, this);
    setHeader(m_headerView);
    m_headerView->setSectionsMovable(true);
    m_headerView->setSectionsClickable(true);
    m_headerView->setSectionResizeMode(DHeaderView::Interactive);
    m_headerView->setStretchLastSection(true);
    m_headerView->setSortIndicatorShown(true);
    m_headerView->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_headerView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_headerView->setFocusPolicy(Qt::TabFocus);

    setRootIsDecorated(false);
    setItemsExpandable(false);
    setFrameStyle(QFrame::NoFrame);
    viewport()->setAutoFillBackground(false);
    // if set to true, qt will draw item background for you, thats not what you want here!
    setAlternatingRowColors(false);
    setAllColumnsShowFocus(false);
    setFocusPolicy(Qt::TabFocus);

    setTabOrder(m_headerView, this);

    // treeview touch scroll support
    auto *scroller = QScroller::scroller(viewport());
    auto prop = scroller->scrollerProperties();
    // turn off overshoot to fix performance issue
    prop.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0);
    prop.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 1);
    prop.setScrollMetric(QScrollerProperties::AxisLockThreshold, 1);
    scroller->setScrollerProperties(prop);
    QScroller::grabGesture(viewport(), QScroller::TouchGesture);
}

void BaseTableView::setModel(QAbstractItemModel *model)
{
    DTreeView::setModel(model);

    if (model) {
        connect(model, &QAbstractItemModel::modelReset, this, [=]() {
            m_hover = {};
            m_pressed = {};
        });
    }
}

void BaseTableView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setOpacity(1);
    painter.setClipping(true);

    QWidget *wnd = DApplication::activeWindow();
    DPalette::ColorGroup cg;
    if (!wnd) {
        cg = DPalette::Inactive;
    } else {
        cg = DPalette::Active;
    }

    auto style = dynamic_cast<DStyle *>(DApplication::style());
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();

    QBrush bgBrush(palette.color(cg, DPalette::Base));

    QStyleOptionFrame option;
    initStyleOption(&option);
    int radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);

    QRect rect = viewport()->rect();
    QRectF clipRect(rect.x(), rect.y() - rect.height(), rect.width(), rect.height() * 2);
    QRectF subRect(rect.x(), rect.y() - rect.height(), rect.width(), rect.height());
    QPainterPath clipPath, subPath;
    clipPath.addRoundedRect(clipRect, radius, radius);
    subPath.addRect(subRect);
    clipPath = clipPath.subtracted(subPath);

    painter.fillPath(clipPath, bgBrush);

    painter.restore();
    DTreeView::paintEvent(event);
}

void BaseTableView::drawRow(QPainter *painter, const QStyleOptionViewItem &options,
                            const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

#ifdef ENABLE_INACTIVE_DISPLAY
    QWidget *wnd = DApplication::activeWindow();
#endif
    DPalette::ColorGroup cg;
    if (!(options.state & DStyle::State_Enabled)) {
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
#endif
    }

    auto *style = dynamic_cast<DStyle *>(DApplication::style());

    auto radius = style->pixelMetric(DStyle::PM_FrameRadius, &options);
    auto margin = style->pixelMetric(DStyle::PM_ContentsMargins, &options);

    auto palette = options.palette;
    QBrush background;
    auto baseColor = palette.color(DPalette::Active, DPalette::Base);
    if (!(index.row() & 1)) {
        background = palette.color(DPalette::Active, DPalette::AlternateBase);
    } else {
        background = baseColor;
    }
    QStyleOptionViewItem opt(options);
    if (options.state & DStyle::State_Enabled) {
        if (selectionModel()->isSelected(index)) {
            background = palette.color(cg, DPalette::Highlight);
            // #ref: DStyle::generatedBrush
            if (m_pressed.isValid() && m_pressed.row() == index.row()) {
                // pressed
                background = style->adjustColor(background.color(), 0, 0, -10);
                opt.state = options.state | QStyle::State_Sunken;
            } else if (m_hover.isValid() && m_hover.row() == index.row()) {
                // hovered
                background = style->adjustColor(background.color(), 0, 0, 20);
            }
        } else {
            if (m_pressed.isValid() && m_pressed.row() == index.row()) {
                // pressed
                background = style->adjustColor(baseColor, 0, 0, -20, 0, 0, 20, 0);
            } else if (m_hover.isValid() && m_hover.row() == index.row()) {
                // hovered
                background = style->adjustColor(baseColor, 0, 0, -10);
            }
        }
    }

    // draw row background
    QPainterPath path;
    QRect rowRect { options.rect.x() - header()->offset(),
                    options.rect.y(),
                    header()->length() - header()->sectionPosition(0),
                    options.rect.height() };
    rowRect.setX(rowRect.x() + margin);
    rowRect.setWidth(rowRect.width() - margin);

    path.addRoundedRect(rowRect, radius, radius);
    painter->fillPath(path, background);

    QTreeView::drawRow(painter, opt, index);

    // draw focus
    if (hasFocus() && currentIndex().row() == index.row()) {
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(options);
        o.state |= QStyle::State_KeyboardFocusChange | QStyle::State_HasFocus;
        o.rect = style->visualRect(layoutDirection(), viewport()->rect(), rowRect);
        style->drawPrimitive(DStyle::PE_FrameFocusRect, &o, painter);
    }

    painter->restore();
}

void BaseTableView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    DTreeView::currentChanged(current, previous);

    if (previous.isValid()) {
        QRect previousRect = visualRect(previous);
        previousRect.setX(0);
        previousRect.setWidth(viewport()->width());
        viewport()->update(previousRect);
    }
    if (current.isValid()) {
        QRect currentRect = visualRect(current);
        currentRect.setX(0);
        currentRect.setWidth(viewport()->width());
        viewport()->update(currentRect);
    }
}

bool BaseTableView::viewportEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::HoverLeave: {
        if (m_hover.isValid()) {
            auto rect = visualRect(m_hover);
            rect.setX(0);
            rect.setWidth(viewport()->width());
            m_hover = QModelIndex();
            viewport()->update(rect);
        }
        break;
    }
    case QEvent::HoverEnter:
    case QEvent::HoverMove: {
        auto *hev = dynamic_cast<QHoverEvent *>(event);
        auto oldHover = m_hover;
        m_hover = indexAt(hev->pos());
        if (m_hover != oldHover) {
            if (oldHover.isValid()) {
                auto rect = visualRect(oldHover);
                rect.setX(0);
                rect.setWidth(viewport()->width());
                viewport()->update(rect);
            }
        }
        if (m_hover.isValid()) {
            auto rect = visualRect(m_hover);
            rect.setX(0);
            rect.setWidth(viewport()->width());
            viewport()->update(rect);
        }
        break;
    }
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress: {
        auto *mev = dynamic_cast<QMouseEvent *>(event);
        auto newIndex = indexAt(mev->pos());
        QRegion region;
        QRect rect;
        if (m_pressed.isValid()) {
            rect = visualRect(m_pressed);
            rect.setX(0);
            rect.setWidth(viewport()->width());
            region += rect;
        }
        if (newIndex.isValid() && newIndex.row() != m_pressed.row()) {
            rect = visualRect(newIndex);
            rect.setX(0);
            rect.setWidth(viewport()->width());
            region += rect;
        }
        m_pressed = (mev->button() == Qt::LeftButton && (mev->type() == QEvent::MouseButtonPress || mev->type() == QEvent::MouseButtonDblClick)) ? newIndex : QModelIndex();
        viewport()->update(region);
        break;
    }
    default:
        break;
    }
    return DTreeView::viewportEvent(event);
}

void BaseTableView::scrollTo(const QModelIndex &index, QAbstractItemView::ScrollHint hint)
{
    if (!(index.isValid() && index.row() >= 0 && index.column() >= 0 && index.model() == model())) {
        return;
    }

    auto area = viewport()->rect();
    QRect rect(columnViewportPosition(index.column()),
               indexRowSizeHint(index) * index.row() - verticalScrollBar()->value(),
               columnWidth(index.column()),
               indexRowSizeHint(index));

    if (rect.isEmpty()) {
        // nothing to do
    } else if (hint == EnsureVisible && area.contains(rect)) {
        viewport()->update(rect);
        // nothing to do
    } else {
        bool above = (hint == EnsureVisible
                      && (rect.top() < area.top()
                          || area.height() < rect.height()));
        bool below = (hint == EnsureVisible
                      && rect.bottom() > area.bottom()
                      && rect.height() < area.height());

        int verticalValue = verticalScrollBar()->value();
        if (hint == PositionAtTop || above)
            verticalValue += rect.top();
        else if (hint == PositionAtBottom || below)
            verticalValue += rect.bottom() - area.height();
        else if (hint == PositionAtCenter)
            verticalValue += rect.top() - ((area.height() - rect.height()) / 2);
        verticalScrollBar()->setValue(verticalValue);
    }
}
