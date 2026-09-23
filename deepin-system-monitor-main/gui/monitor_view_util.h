// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MONITOR_VIEW_UTIL_H
#define MONITOR_VIEW_UTIL_H

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>

#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionFrame>
#include <QWidget>

namespace MonitorViewUtil {

static void paintRoundedBackground(QWidget *widget, QPainter &painter)
{
    painter.setRenderHint(QPainter::Antialiasing);

    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();

    QWidget *wnd = DApplication::activeWindow();
    DPalette::ColorGroup cg = wnd ? DPalette::Active : DPalette::Inactive;

    auto *style = dynamic_cast<DStyle *>(DApplication::style());
    QStyleOptionFrame option;
    option.initFrom(widget);
    int radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);

    QRectF rect = widget->rect();
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);

    QBrush bgBrush(palette.color(cg, DPalette::Window));
    painter.fillPath(path, bgBrush);
}

} // namespace MonitorViewUtil

#endif // MONITOR_VIEW_UTIL_H
