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

#include "system_service_page_widget.h"

#include "main_window.h"
#include "system_service_table_view.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DStyle>

#include <QHBoxLayout>
#include <QPainterPath>

// constructor
SystemServicePageWidget::SystemServicePageWidget(DWidget *parent)
    : DFrame(parent)
{
    // global app helper instance
    auto *dAppHelper = DApplicationHelper::instance();
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

    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    auto bgColor = palette.color(DPalette::Background);

    // paint frame background
    painter.fillPath(path, bgColor);
}
