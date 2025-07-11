// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "base_header_view.h"
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

#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QtMath>
#include <QModelIndex>
#include <QPainterPath>

using namespace DDLog;

// Constructor
BaseHeaderView::BaseHeaderView(Qt::Orientation orientation, QWidget *parent)
    : DHeaderView(orientation, parent)
{
    qCDebug(app) << "BaseHeaderView constructor";
    installEventFilter(this);
}

// Get header view's hint size
QSize BaseHeaderView::sizeHint() const
{
    // qCDebug(app) << "BaseHeaderView sizeHint";
    return QSize(width(), 37);
}

// Paint event handler
void BaseHeaderView::paintEvent(QPaintEvent *event)
{
    // qCDebug(app) << "BaseHeaderView paintEvent";
    // draw focus
    DHeaderView::paintEvent(event);
    if (hasFocus() && m_focusReason == Qt::TabFocusReason)
    {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);

        QRect rect = viewport()->rect();
        QStyleOptionHeader option;
        initStyleOption(&option);

        auto *style = dynamic_cast<DStyle *>(DApplication::style());
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(option);
        // need take scroll offset into consideration
        QRect focusRect{rect.x() - offset(), rect.y(), length() - sectionPosition(0), rect.height()};
        o.rect = style->visualRect(layoutDirection(), rect, focusRect);
        style->drawPrimitive(DStyle::PE_FrameFocusRect, &o, &painter);
    }
    QPainter painter(viewport());
    QPainterPath innerPath,outPath;
    innerPath.addRoundedRect(viewport()->rect(),8,8);
    outPath.addRect(viewport()->rect());
    outPath=outPath.subtracted(innerPath);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    painter.fillPath(outPath,DApplicationHelper::instance()->applicationPalette().color(DPalette::Active,DPalette::AlternateBase));
#else
    painter.fillPath(outPath,DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::Active,DPalette::AlternateBase));
#endif
}

void BaseHeaderView::focusInEvent(QFocusEvent *event)
{
    // qCDebug(app) << "BaseHeaderView focus in";
    DHeaderView::focusInEvent(event);
    m_focusReason =  event->reason();
}

// Event filter handler
bool BaseHeaderView::eventFilter(QObject *obj, QEvent *ev)
{
    // qCDebug(app) << "BaseHeaderView eventFilter";
    if (ev->type() == QEvent::KeyPress) {
        qCDebug(app) << "BaseHeaderView eventFilter: KeyPress";
        QKeyEvent *kev = dynamic_cast<QKeyEvent *>(ev);
        if (kev->key() == Qt::Key_Space ||
                kev->key() == Qt::Key_Up ||
                kev->key() == Qt::Key_Down) {
            qCDebug(app) << "BaseHeaderView eventFilter: Intercepting key" << kev->key();
            return true;
        }
    }
    return DHeaderView::eventFilter(obj, ev);
}
