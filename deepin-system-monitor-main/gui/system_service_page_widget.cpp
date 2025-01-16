// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "system_service_page_widget.h"

#include "main_window.h"
#include "system_service_table_view.h"

#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif
#include <DStyle>

#include <QHBoxLayout>
#include <QPainterPath>

// constructor
SystemServicePageWidget::SystemServicePageWidget(DWidget *parent)
    : DFrame(parent)
{
    // global app helper instance
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif
    // global palette
    auto palette = dAppHelper->applicationPalette();

    // content margin
    int margin = 10;

    // main frame layout
    auto *layout = new QHBoxLayout(this);
    // service table view instance
    m_svcTableView = new SystemServiceTableView(this);
    layout->addWidget(m_svcTableView);
    layout->setContentsMargins(margin, margin, margin, margin);
    setLayout(layout);
}
// destructor
SystemServicePageWidget::~SystemServicePageWidget() {}

// paint event handler
void SystemServicePageWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QPainterPath path;
    path.addRect(QRectF(rect()));
    painter.setOpacity(1);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif
    auto palette = dAppHelper->applicationPalette();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto bgColor = palette.color(DPalette::Background);
#else
    auto bgColor = palette.color(DPalette::Window);
#endif

    // paint frame background
    painter.fillPath(path, bgColor);
}
