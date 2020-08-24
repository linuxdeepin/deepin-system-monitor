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
#include "xwin_kill_preview_background_widget.h"

#include "xwin_kill_preview_tooltip_widget.h"

#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QPaintEvent>
#include <QDebug>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QHideEvent>
#include <QRegion>

XWinKillPreviewBackgroundWidget::XWinKillPreviewBackgroundWidget(QPixmap &background, QWidget *parent)
    : QWidget(parent)
    , m_background(background)
{
    initUI();
    initConnection();
}

void XWinKillPreviewBackgroundWidget::updateSelection(const QRegion &region)
{
    // coordinate translate
    m_selRegion = region.translated(-geometry().x(), -geometry().y());
    update();
}

void XWinKillPreviewBackgroundWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QPainterPath path;

    painter.setOpacity(1);
    painter.drawPixmap(rect(), m_background);

    painter.setOpacity(0.25);
    path.addRegion(m_selRegion);
    painter.fillPath(path, Qt::red);
}

void XWinKillPreviewBackgroundWidget::resizeEvent(QResizeEvent *)
{
    m_tooltip->move(geometry().center() - m_tooltip->rect().center());
}

void XWinKillPreviewBackgroundWidget::hideEvent(QHideEvent *)
{
    if (m_tooltip)
        m_tooltip->hide();
}

void XWinKillPreviewBackgroundWidget::initUI()
{
    Qt::WindowFlags flags {};
    flags |= Qt::Window;
    flags |= Qt::WindowStaysOnTopHint;
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::BypassWindowManagerHint;
    setWindowFlags(flags);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);

    m_tooltip = new XWinKillPreviewTooltipWidget(this);
    m_tooltip->installEventFilter(this);
    m_tooltip->move(geometry().center() - m_tooltip->rect().center());
    m_tooltip->show();

    installEventFilter(this);
}

void XWinKillPreviewBackgroundWidget::initConnection()
{

}
