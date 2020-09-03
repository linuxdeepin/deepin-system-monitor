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
#include "xwin_kill_preview_widget.h"

#include "xwin_kill_preview_background_widget.h"
#include "wm/wm_info.h"
#include "main_window.h"
#include "process/system_monitor.h"

#include <QDebug>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QScreen>
#include <QWindow>
#include <QRect>
#include <QApplication>
#include <QCursor>
#include <QRegion>

using namespace std;
using namespace util::wm;

XWinKillPreviewWidget::XWinKillPreviewWidget(QWidget *parent) : QWidget(parent)
{
    m_wminfo = new WMInfo();

    initUI();
    initConnections();

    resize(1, 1);
    move(-65536, -65536);
    show();
}

XWinKillPreviewWidget::~XWinKillPreviewWidget()
{
    auto *mw = MainWindow::instance();
    mw->setWindowState((mw->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    setMouseTracking(false);
    releaseMouse();
    releaseKeyboard();
    delete m_wminfo;
}

void XWinKillPreviewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    auto pos = QCursor::pos();
    auto list = m_wminfo->selectWindow(pos);

    // fix cursor not update issue while moved to areas covered by intersected area of dock & normal windows
    if (m_wminfo->isCursorHoveringDocks(pos)) {
        return;
    }

    for (auto &select : list) {
        if (SystemMonitor::getCurrentPID() == select->pid)
            continue;

        if (select->rect.contains(pos)) {
            hide();
            for (auto &background : m_backgroundList) {
                background->hide();
            }
            setMouseTracking(false);
            releaseMouse();
            releaseKeyboard();
            emit windowClicked(select->pid);
            close();
            break;
        }
    }
}

void XWinKillPreviewWidget::mouseMoveEvent(QMouseEvent *)
{
    auto pos = QCursor::pos();

    auto list = m_wminfo->selectWindow(pos);
    bool found {false};

    // fix cursor not update issue while moved to areas covered by intersected area of dock & normal windows
    if (m_wminfo->isCursorHoveringDocks(pos)) {
        for (auto &bg : m_backgroundList)
            bg->clearSelection();
        emit cursorUpdated(m_defaultCursor);
        return;
    }

    for (auto &select : list) {
        if (SystemMonitor::getCurrentPID() == select->pid)
            continue;

        if (select->rect.contains(pos)) {
            found = true;

            // find all windows hovered above, if any clip out the intersected region
            auto hoveredBy = m_wminfo->getHoveredByWindowList(select->wid, select->rect);
            QRegion region {select->rect};
            for (auto &hover : hoveredBy)
                region = region.subtracted(hover->rect);

            for (auto &bg : m_backgroundList) {
                if (bg->geometry().intersects(select->rect)) {
                    auto area = region.intersected(bg->geometry());
                    bg->updateSelection(area);
                    emit cursorUpdated(m_killCursor);
                } else {
                    bg->clearSelection();
                }
            }
            break;
        }
    }
    if (!found) {
        for (auto &bg : m_backgroundList)
            bg->clearSelection();
        emit cursorUpdated(m_defaultCursor);
    }
}

void XWinKillPreviewWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        // restore main window
        auto *mw = MainWindow::instance();
        mw->setWindowState((mw->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);

        close();
    }
}

void XWinKillPreviewWidget::initUI()
{
    Qt::WindowFlags flags {};
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::Window;
    flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, true);
    grabKeyboard();
    setMouseTracking(true);
    grabMouse();

    m_killCursor = QCursor(iconPathFromQrc("kill_cursor.svg"));
    m_defaultCursor = QCursor(Qt::ForbiddenCursor);

    // show background window in all screens
    for (auto screen : QApplication::screens()) {
        auto geom = screen->geometry();
        auto pixmap = screen->grabWindow(m_wminfo->getRootWindow(), geom.x(), geom.y(), geom.width(), geom.height());

        auto *background = new XWinKillPreviewBackgroundWidget(pixmap, this);
        connect(this, &XWinKillPreviewWidget::cursorUpdated, background, &XWinKillPreviewBackgroundWidget::setCursor);
        background->resize(geom.size());
        background->move(geom.x(), geom.y());

        m_backgroundList << background;
        m_screens << screen;

        background->show();
    }
}

void XWinKillPreviewWidget::initConnections()
{
}
