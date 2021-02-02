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
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "base_header_view.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>
#include <DStyleHelper>

#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QtMath>
#include <QModelIndex>
#include <QPainterPath>

// Constructor
BaseHeaderView::BaseHeaderView(Qt::Orientation orientation, QWidget *parent)
    : DHeaderView(orientation, parent)
{
    installEventFilter(this);

    viewport()->setAutoFillBackground(false);
}

// Get header view's hint size
QSize BaseHeaderView::sizeHint() const
{
    return QSize(width(), 36 + m_spacing);
}

// Paint event handler
void BaseHeaderView::paintEvent(QPaintEvent *event)
{
    DHeaderView::paintEvent(event);

    // draw focus
    if (hasFocus() && m_focusReason == Qt::TabFocusReason) {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);

        QRect rect = viewport()->rect();
        QStyleOptionHeader option;
        initStyleOption(&option);

        auto *style = dynamic_cast<DStyle *>(DApplication::style());
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(option);
        // need take scroll offset into consideration
        QRect focusRect {rect.x() - offset(), rect.y(), length() - sectionPosition(0), rect.height()};
        o.rect = style->visualRect(layoutDirection(), rect, focusRect);
        style->drawPrimitive(DStyle::PE_FrameFocusRect, &o, &painter);
    }
}

void BaseHeaderView::focusInEvent(QFocusEvent *event)
{
    DHeaderView::focusInEvent(event);
    m_focusReason =  event->reason();
}

// Event filter handler
bool BaseHeaderView::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *kev = dynamic_cast<QKeyEvent *>(ev);
        if (kev->key() == Qt::Key_Space ||
                kev->key() == Qt::Key_Up ||
                kev->key() == Qt::Key_Down) {
            return true;
        }
    }
    return DHeaderView::eventFilter(obj, ev);
}

// Viewport event filter
bool BaseHeaderView::viewportEvent(QEvent *event)
{
    // #ref: QTreeView::event
    switch (event->type()) {
    case QEvent::HoverEnter: {
        auto *he = dynamic_cast<QHoverEvent *>(event);
        // mark current section under cursor as hovered
        m_hover = logicalIndexAt(he->pos());
        if (m_hover != -1)
            // repaint section in hovered state
            updateSection(m_hover);
        break;
    }
    case QEvent::Leave:
    case QEvent::HoverLeave: {
        if (m_hover != -1)
            // repaint section in normal state after hover leave
            updateSection(m_hover);
        m_hover = -1;
        break;
    }
    case QEvent::HoverMove: {
        auto *he = dynamic_cast<QHoverEvent *>(event);
        int oldHover = m_hover;
        // mark current row under cursor as hovered
        m_hover = logicalIndexAt(he->pos());
        if (m_hover != oldHover) {
            if (oldHover != -1)
                // repaint old section in normal state
                updateSection(oldHover);
            if (m_hover != -1)
                // repaint new hovered section in hovered state
                updateSection(m_hover);
        }
        break;
    }
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress: {
        auto *mev = dynamic_cast<QMouseEvent *>(event);
        m_pressed = logicalIndexAt(mev->pos());
        m_pressed = (mev->button() == Qt::LeftButton && (mev->type() == QEvent::MouseButtonPress || mev->type() == QEvent::MouseButtonDblClick)) ? m_pressed : -1;
        update();
        break;
    }
    default:
        break;
    }
    return DHeaderView::viewportEvent(event);
}
