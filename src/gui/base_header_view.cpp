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

//#include <cmath>

static const int kSpacingMargin = 4;
// recommend size
// static const QSize kDropDownSize {10, 6};
static const QSize kDropDownSize {11, 10};

BaseHeaderView::BaseHeaderView(Qt::Orientation orientation, QWidget *parent)
    : DHeaderView(orientation, parent)
{
    installEventFilter(this);

    viewport()->setAutoFillBackground(false);
}

QSize BaseHeaderView::sizeHint() const
{
    return QSize(width(), 36 + m_spacing);
}

int BaseHeaderView::sectionSizeHint(int logicalIndex) const
{
//    QStyleOptionHeader option;
//    initStyleOption(&option);
//    auto *style = dynamic_cast<DStyle *>(DApplication::style());
//    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

//    QFontMetrics fm(this->font());
//    QString buf = model()->headerData(logicalIndex, Qt::Horizontal, Qt::DisplayRole).toString();
//    if (sortIndicatorSection() == logicalIndex) {
//        return fm.size(Qt::TextSingleLine, buf).width() + margin * 3 + kDropDownSize.width();
//    } else {
//        return fm.size(Qt::TextSingleLine, buf).width() + margin * 2;
//    }
    return DHeaderView::sectionSizeHint(logicalIndex);
}

void BaseHeaderView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(1);

    DPalette::ColorGroup cg;
#ifdef ENABLE_INACTIVE_DISPLAY
    QWidget *wnd = DApplication::activeWindow();
    if (!wnd) {
        cg = DPalette::Inactive;
    } else {
        cg = DPalette::Active;
    }
#else
    cg = DPalette::Active;
//    cg = DPalette::Current;
#endif

    auto palette = DApplicationHelper::instance()->applicationPalette();

    auto *style = dynamic_cast<DStyle *>(DApplication::style());

    QStyleOption opt;
    opt.initFrom(this);
    QBrush bgBrush(palette.color(cg, DPalette::Base));
    QBrush hlBrush = Qt::transparent;
    QRect hlRect;

    // background
    if ((opt.state & DStyle::State_Enabled) && m_hover != -1) {
        auto baseColor = bgBrush.color();
        if (m_pressed >= 0) {
            auto actColor = palette.color(cg, DPalette::Highlight);
            actColor.setAlphaF(0.1);
            auto newColor = style->adjustColor(baseColor, 0, 0, -20, 0, 0, 20, 0);
            hlBrush = style->blendColor(newColor, actColor);
        } else {
            hlBrush = style->adjustColor(baseColor, 0, 0, -10);
        }
        auto spos = sectionPosition(m_hover);
        auto sw = sectionSize(m_hover);
        hlRect = {
            m_hover > 0 ? spos + 1 - offset() : spos - offset(),
            0,
            m_hover > 0 ? sw - 1 : sw,
            height()};
    }

    QStyleOptionHeader option;
    initStyleOption(&option);
    auto radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);

    QRect rect = viewport()->rect();
    QRectF clipRect(rect.x(), rect.y(), rect.width(), rect.height() * 2);
    QRectF subRect(rect.x(), rect.y() + rect.height(), rect.width(), rect.height());
    QPainterPath clipPath, subPath;
    clipPath.addRoundedRect(clipRect, radius, radius);
    subPath.addRect(subRect);
    clipPath = clipPath.subtracted(subPath);

    painter.fillPath(clipPath, bgBrush);
    QPainterPath hlPath;
    hlPath.addRect(hlRect);
    hlPath = hlPath.intersected(clipPath);
    painter.fillPath(hlPath, hlBrush);

    DHeaderView::paintEvent(event);
    painter.restore();

    // draw focus
    if (hasFocus()) {
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(option);
        QRect focusRect {rect.x() - offset(), rect.y(), length() - sectionPosition(0), rect.height()};
        o.rect = style->visualRect(layoutDirection(), rect, focusRect);
        style->drawPrimitive(DStyle::PE_FrameFocusRect, &o, &painter);
    }
}

void BaseHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(1);

    DPalette::ColorGroup cg;
    QStyleOption opt;
    opt.initFrom(this);
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

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    //    DPalette palette = dAppHelper->palette(this);

    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());

    QStyleOptionHeader option;
    initStyleOption(&option);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    // title
    QRect contentRect(rect.x(), rect.y(), rect.width(), rect.height() - m_spacing);
    QRect hSpacingRect(rect.x(), contentRect.height(), rect.width(),
                       rect.height() - contentRect.height());

    QBrush contentBrush(palette.color(cg, DPalette::Base));
    // horizontal spacing
    QBrush hSpacingBrush(palette.color(cg, DPalette::FrameBorder));
    //    QBrush hSpacingBrush(
    //        DApplicationHelper::instance()->palette(this).color(DPalette::FrameBorder));
    // vertical spacing
    QBrush vSpacingBrush(palette.color(cg, DPalette::FrameBorder));
    QRectF vSpacingRect(rect.x(), rect.y() + kSpacingMargin, m_spacing,
                        rect.height() - kSpacingMargin * 2);
    QBrush clearBrush(palette.color(cg, DPalette::Window));

    painter->fillRect(hSpacingRect, clearBrush);
    painter->fillRect(hSpacingRect, hSpacingBrush);

    if (visualIndex(logicalIndex) > 0) {
        painter->fillRect(vSpacingRect, clearBrush);
        painter->fillRect(vSpacingRect, vSpacingBrush);
    }

    QPen forground;
    auto type = DApplicationHelper::instance()->themeType();
    forground.setColor(palette.color(cg, DPalette::Text));
    if (opt.state == QStyle::State_Enabled && m_pressed == logicalIndex) {
        forground = opt.palette.highlight().color();
    } else if (opt.state == QStyle::State_Enabled && m_hover == logicalIndex) {
        forground = style->adjustColor(forground.color(), 0, 0, type == DApplicationHelper::DarkType ? 20 : -50);
    }

    QRect textRect;
    if (sortIndicatorSection() == logicalIndex) {
        textRect = {
            contentRect.x() + margin,
            contentRect.y(),
            contentRect.width() - margin * 3 - kDropDownSize.width(),
            contentRect.height()};
    } else {
        textRect = {
            contentRect.x() + margin,
            contentRect.y(),
            contentRect.width() - margin,
            contentRect.height()};
    }
    QString title = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString();
    int align = model()->headerData(logicalIndex, orientation(), Qt::TextAlignmentRole).toInt();

    // forground
    painter->setPen(forground);
    painter->drawText(textRect, int(align), title);

    // sort indicator
    if (isSortIndicatorShown() && logicalIndex == sortIndicatorSection()) {
        QRect sortIndicator(textRect.x() + textRect.width() + margin,
                            textRect.y() + qCeil((textRect.height() - kDropDownSize.height()) / 2.),
                            kDropDownSize.width(), kDropDownSize.height());
        option.rect = sortIndicator;
        if (sortIndicatorOrder() == Qt::DescendingOrder) {
            style->drawPrimitive(DStyle::PE_IndicatorArrowDown, &option, painter, this);
        } else if (sortIndicatorOrder() == Qt::AscendingOrder) {
            style->drawPrimitive(DStyle::PE_IndicatorArrowUp, &option, painter, this);
        }
    }

    painter->restore();
}

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

bool BaseHeaderView::viewportEvent(QEvent *event)
{
    // #ref: QTreeView::event
    switch (event->type()) {
    case QEvent::HoverEnter: {
        auto *he = dynamic_cast<QHoverEvent *>(event);
        m_hover = logicalIndexAt(he->pos());
        if (m_hover != -1)
            updateSection(m_hover);
        break;
    }
    case QEvent::Leave:
    case QEvent::HoverLeave: {
        if (m_hover != -1)
            updateSection(m_hover);
        m_hover = -1;
        break;
    }
    case QEvent::HoverMove: {
        auto *he = dynamic_cast<QHoverEvent *>(event);
        int oldHover = m_hover;
        m_hover = logicalIndexAt(he->pos());
        if (m_hover != oldHover) {
            if (oldHover != -1)
                updateSection(oldHover);
            if (m_hover != -1)
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
