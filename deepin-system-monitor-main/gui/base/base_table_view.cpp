// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "base_table_view.h"

#include "base_header_view.h"
#include "base_item_delegate.h"
#include "ddlog.h"

#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif
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
#include <QFocusEvent>

using namespace DDLog;
#define HEADER_MIN_SECTION_SIZE 120

// default constructor
BaseTableView::BaseTableView(DWidget *parent)
    : DTreeView(parent)
{
    qCDebug(app) << "BaseTableView constructor";
    // enable touch event handling
    setAttribute(Qt::WA_AcceptTouchEvents);

    // set delegate instance
    m_itemDelegate = new BaseItemDelegate(this);
    setItemDelegate(m_itemDelegate);
    // set header view instance
    m_headerView = new BaseHeaderView(Qt::Horizontal, this);
    setHeader(m_headerView);
    // section movable
    m_headerView->setSectionsMovable(true);
    // section clickable
    m_headerView->setSectionsClickable(true);
    // section resizable
    m_headerView->setSectionResizeMode(DHeaderView::Interactive);
    // can stretch last section?
    m_headerView->setStretchLastSection(true);
    // show sort indicator?
    m_headerView->setSortIndicatorShown(true);
    // section default text alignment
    m_headerView->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // header context menu policy
    m_headerView->setContextMenuPolicy(Qt::CustomContextMenu);
    // header view focus policy
    m_headerView->setFocusPolicy(Qt::StrongFocus);
    // header section context min width
    m_headerView->setMinimumSectionSize(HEADER_MIN_SECTION_SIZE);
    // not allowing expanding/collpasing top-level items
    setRootIsDecorated(false);
    // items are not expandable
    setItemsExpandable(false);
    setFrameStyle(QFrame::NoFrame);
    viewport()->setAutoFillBackground(false);
    // if set to true, qt will draw item background for you, thats not what you want here!
    setAlternatingRowColors(false);
    // disable default focus style
    setAllColumnsShowFocus(false);
    setFocusPolicy(Qt::StrongFocus);

    // adjust focus order (header -> treeview)
    setTabOrder(m_headerView, this);

    // treeview touch scroll support
    auto *scroller = QScroller::scroller(viewport());
    auto prop = scroller->scrollerProperties();
    // turn off overshoot to fix performance issue
    prop.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0);
    prop.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 1);
    // lock scroll direction to fix performance issue
    prop.setScrollMetric(QScrollerProperties::AxisLockThreshold, 1);
    scroller->setScrollerProperties(prop);
    // enable touch gesture
    QScroller::grabGesture(viewport(), QScroller::TouchGesture);
    //set horizontalScrollbar always visible
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

// set view model
void BaseTableView::setModel(QAbstractItemModel *model)
{
    qCDebug(app) << "setModel";
    DTreeView::setModel(model);
    // listen on modelReset signal, reset any hovered or pressed index
    if (model) {
        qCDebug(app) << "Connecting model reset signal";
        connect(model, &QAbstractItemModel::modelReset, this, [ = ]() {
            qCDebug(app) << "Model reset, clearing hover and pressed states";
            m_hover = {};
            m_pressed = {};
        });
//        connect(model, &QAbstractItemModel::rowsRemoved, this, [ = ]() {
//        qCWarning(app)<<(model->rowCount()==0);
//        });
    }
}

// paint event handler
void BaseTableView::paintEvent(QPaintEvent *event)
{
    // qCDebug(app) << "BaseTableView paintEvent";
    // viewport's painter object
    QPainter painter(viewport());
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setOpacity(1);
    // enable clipping
    painter.setClipping(true);

    QWidget *wnd = DApplication::activeWindow();
    DPalette::ColorGroup cg;
    if (!wnd) {
        qCDebug(app) << "No active window";
        cg = DPalette::Inactive;
    } else {
        qCDebug(app) << "Active window found";
        cg = DPalette::Active;
    }

    // DStyle object
    auto style = dynamic_cast<DStyle *>(DApplication::style());
    // global app helper instance
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif
    // global palette
    auto palette = dAppHelper->applicationPalette();

    // background brush
    QBrush bgBrush(palette.color(cg, DPalette::Base));

    // initialize style option
    QStyleOptionFrame option;
    initStyleOption(&option);

    // frame radius
    int radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);

    QRect rect = viewport()->rect();
    // create a clip region double height of viewport's rect
    QRectF clipRect(rect.x(), rect.y() - rect.height(), rect.width(), rect.height() * 2);
    // create a rect equal to upper half part of clip region
    QRectF subRect(rect.x(), rect.y() - rect.height(), rect.width(), rect.height());
    QPainterPath clipPath, subPath;
    clipPath.addRoundedRect(clipRect, radius, radius);
    subPath.addRect(subRect);
    // clip out upper half part of the clipRect
    clipPath = clipPath.subtracted(subPath);

    // draw viewport's background
    painter.fillPath(clipPath, bgBrush);

    // restore painter
    painter.restore();
    DTreeView::paintEvent(event);
}

// draw custom styled row
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
    // DStyle instance
    auto *style = dynamic_cast<DStyle *>(DApplication::style());

    // frame radius
    auto radius = style->pixelMetric(DStyle::PM_FrameRadius, &options);
    // content margin
    auto margin = 10;

    auto palette = options.palette;
    QBrush background;
    // base row color
    auto baseColor = palette.color(DPalette::Active, DPalette::Base);
    // alternate row color
    if (!(index.row() & 1)) {
        background = palette.color(DPalette::Active, DPalette::AlternateBase);
    } else {
        background = baseColor;
    }
    QStyleOptionViewItem opt(options);
    // adjust background color only if view's in enabled state
    if (options.state & DStyle::State_Enabled) {
        if (selectionModel()->isSelected(index)) {
            // current row's color
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

    // adjust row's rect
    QPainterPath path;
    QRect rowRect {options.rect.x() - header()->offset(), // x offset of horizontal scrollbar position
                   options.rect.y(),
                   header()->length() - header()->sectionPosition(0), // max width calculated from header's width
                   options.rect.height()};
    // set left/right most column's margin
    rowRect.setX(rowRect.x() + margin);
    rowRect.setWidth(rowRect.width() - margin);

    // draw row background
    path.addRoundedRect(rowRect, radius, radius);
    painter->fillPath(path, background);

    // delegate to base draw method
    QTreeView::drawRow(painter, opt, index);

    // draw focus
    if (hasFocus() && m_focusReason == Qt::TabFocusReason && currentIndex().row() == index.row()) {
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(options);
        o.state |= QStyle::State_KeyboardFocusChange | QStyle::State_HasFocus;
        o.rect = style->visualRect(layoutDirection(), viewport()->rect(), rowRect);
        style->drawPrimitive(DStyle::PE_FrameFocusRect, &o, painter);
    }

    // restore painter state
    painter->restore();
}

void BaseTableView::focusInEvent(QFocusEvent *event)
{
    // qCDebug(app) << "focusInEvent reason: " << event->reason();
    DTreeView::focusInEvent(event);
    m_focusReason =  event->reason();
}

// current selected item changed handler
void BaseTableView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    // qCDebug(app) << "currentChanged from" << previous << "to" << current;
    DTreeView::currentChanged(current, previous);

    // update previous item's paint region
    if (previous.isValid()) {
        qCDebug(app) << "Updating previous item";
        QRect previousRect = visualRect(previous);
        previousRect.setX(0);
        previousRect.setWidth(viewport()->width());
        previousRect.adjust(-1, -1, 1, 1);
        viewport()->update(previousRect);
    }
    // update current item's paint region
    if (current.isValid()) {
        qCDebug(app) << "Updating current item";
        QRect currentRect = visualRect(current);
        currentRect.setX(0);
        currentRect.setWidth(viewport()->width());
        currentRect.adjust(-1, -1, 1, 1);
        viewport()->update(currentRect);
    }
}

// viewport event handler
bool BaseTableView::viewportEvent(QEvent *event)
{
    // qCDebug(app) << "viewportEvent type: " << event->type();
    switch (event->type()) {
    case QEvent::HoverLeave: {
        // qCDebug(app) << "HoverLeave event";
        // hover leave event, update last hovered item's visual region
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
        // qCDebug(app) << "HoverEnter or HoverMove event";
        // hover move event, update both last & current hovered item's visual region
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
        // qCDebug(app) << "MouseMove or MouseButtonRelease or MouseButtonDblClick or MouseButtonPress event";
        auto *mev = dynamic_cast<QMouseEvent *>(event);
        auto newIndex = indexAt(mev->pos());
        QRegion region;
        QRect rect;
        // update pressed row's region if any
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
        // only left mouse button events (click & double click) need refresh
        m_pressed = (mev->button() == Qt::LeftButton && (mev->type() == QEvent::MouseButtonPress || mev->type() == QEvent::MouseButtonDblClick)) ? newIndex : QModelIndex();
        viewport()->update(region);
        break;
    }
    case QEvent::TouchEnd: {
        // when using touch screen, no need any hovered effect...so reset hovered item in case Qt translate touch events into mouse events
        m_hover = {};
        break;
    }
    default:
        break;
    }
    // delegate other events to base event handler
    return DTreeView::viewportEvent(event);
}

// scroll viewport to ensure specified indexed item be visible
void BaseTableView::scrollTo(const QModelIndex &index, QAbstractItemView::ScrollHint hint)
{
    qCDebug(app) << "scrollTo index: " << index << " hint: " << hint;
    // check index validity
    if (!(index.isValid() && index.row() >= 0 && index.column() >= 0 && index.model() == model())) {
        qCDebug(app) << "scrollTo: invalid index";
        return;
    }

    // reset last hovered item in case user scroll viewport with up/down key press
    m_hover = {};

    auto area = viewport()->rect();
    // calculate current indexed item's rect
    QRect rect(columnViewportPosition(index.column()),
               indexRowSizeHint(index) * index.row() - verticalScrollBar()->value(),
               columnWidth(index.column()),
               indexRowSizeHint(index));

    if (rect.isEmpty()) {
        qCDebug(app) << "scrollTo: rect is empty, nothing to do";
        // nothing to do
    } else if (hint == EnsureVisible && area.contains(rect)) {
        qCDebug(app) << "scrollTo: item is already visible";
        viewport()->update(QRect {0,
                                  indexRowSizeHint(index) * index.row() - verticalScrollBar()->value(),
                                  viewport()->width(),
                                  indexRowSizeHint(index)});
        // nothing to do
    } else {
        qCDebug(app) << "scrollTo: scrolling to make item visible";
        // current item above viewport rect
        bool above = (hint == EnsureVisible
                      && (rect.top() < area.top()
                          || area.height() < rect.height()));
        // current item below viewport rect
        bool below = (hint == EnsureVisible
                      && rect.bottom() > area.bottom()
                      && rect.height() < area.height());

        int verticalValue = verticalScrollBar()->value();
        if (hint == PositionAtTop || above) {
            qCDebug(app) << "scrollTo: positioning at top";
            verticalValue += rect.top();
        } else if (hint == PositionAtBottom || below) {
            qCDebug(app) << "scrollTo: positioning at bottom";
            verticalValue += rect.bottom() - area.height();
        } else if (hint == PositionAtCenter) {
            qCDebug(app) << "scrollTo: positioning at center";
            verticalValue += rect.top() - ((area.height() - rect.height()) / 2);
        }
        // adjust scroll bar to a new position to ensure item visible
        verticalScrollBar()->setValue(verticalValue);
    }
}
