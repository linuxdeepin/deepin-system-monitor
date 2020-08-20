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
#include "xwin_kill_preview_mask_widget.h"

#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>

DWIDGET_USE_NAMESPACE

XWinKillPreviewMaskWidget::XWinKillPreviewMaskWidget(QWidget *parent) : QWidget(parent)
{
    initUI();
}

void XWinKillPreviewMaskWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    auto background = Qt::red;
    QPainterPath path;

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(.25);
    path.addRect(rect());
    painter.fillPath(path, background);
}

void XWinKillPreviewMaskWidget::initUI()
{
    Qt::WindowFlags flags {};
    flags |= Qt::ToolTip;
    flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
}
