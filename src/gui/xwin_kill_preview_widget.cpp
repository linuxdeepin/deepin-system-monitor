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

#include "application.h"
#include "xwin_kill_preview_background_widget.h"
#include "wm/wm_info.h"
#include "main_window.h"
#include "ui_common.h"

#include <QDebug>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QScreen>
#include <QWindow>
#include <QRect>
#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QRegion>

#include <unistd.h>

using namespace std;
using namespace core::wm;

// constructor
XWinKillPreviewWidget::XWinKillPreviewWidget(QWidget *parent) : QWidget(parent)
{
    // new window manager instance
    m_wminfo = new WMInfo();

    // init ui components & connections
    initUI();
    initConnections();

    // resize preview widget to 1x1 in case any obvious black flash widgets occassionly shown on screen
    resize(1, 1);
    // move preview widget way further away so it's impossible to show on screen
    move(-65536, -65536);
    show();
}

// destructor
XWinKillPreviewWidget::~XWinKillPreviewWidget()
{
    // restore window state
    auto *mw = gApp->mainWindow();
    mw->setWindowState((mw->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    // release any owned mouse & keyboard
    setMouseTracking(false);
    releaseMouse();
    releaseKeyboard();
    delete m_wminfo;
}

// mouse press event
void XWinKillPreviewWidget::mousePressEvent(QMouseEvent *event)
{
    // only accept left mouse button click event
    if (event->button() != Qt::LeftButton) {
        return;
    }

    // get the list of windows under cursor in stacked order when mouse pressed
    auto pos = QCursor::pos();
    auto list = m_wminfo->selectWindow(pos);

    // fix cursor not update issue while moved to areas covered by intersected area of dock & normal windows
    if (m_wminfo->isCursorHoveringDocks(pos)) {
        return;
    }

    for (auto &select : list) {
        // if the window is created by ourself, then ignore it
        if (getpid() == select->pid)
            continue;

        // if such window exists, we emit window clicked signal to notify kill application performed action
        if (select->rect.contains(pos)) {
            // hide preview & background widgets first
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

// mouse move event handler
void XWinKillPreviewWidget::mouseMoveEvent(QMouseEvent *)
{
    double x = QGuiApplication::primaryScreen()->devicePixelRatio(); // 获得当前的缩放比例
    auto pos = QCursor::pos();
    // get the list of windows under cursor from cache in stacked order
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
        // if the window is created by ourself, then ignore it
        if (getpid() == select->pid)
            continue;
        auto selRect = QRect(static_cast<int>(select->rect.x() / x), static_cast<int>(select->rect.y() / x), static_cast<int>(select->rect.width() / x), static_cast<int>(select->rect.height() / x));
        if (selRect.contains(pos)) {
            found = true;

            // find all windows hovered above, if any clip out the intersected region
            auto hoveredBy = m_wminfo->getHoveredByWindowList(select->wid, selRect);
            QRegion region {selRect};
            for (auto &hover : hoveredBy) {
                auto hoverrect = QRect(static_cast<int>(hover->rect.x() / x), static_cast<int>(hover->rect.y() / x), static_cast<int>(hover->rect.width() / x), static_cast<int>(hover->rect.height() / x));
                region = region.subtracted(hoverrect);
            }

            // if current selected window is crossing screens, we need update each sub part on each screen
            for (auto &bg : m_backgroundList) {
                if (bg->geometry().intersects(selRect)) {
                    auto area = region.intersected(bg->geometry());
                    bg->updateSelection(area);
                    emit cursorUpdated(m_killCursor);
                } else {
                    // if current screen doesn't intersect with the selected window, we need clear any selection left before
                    bg->clearSelection();
                }
            }
            break;
        }
    }
    // if no such window found, we need clear any selection left before, plus restore cursor to its default style (forbit style)
    if (!found) {
        for (auto &bg : m_backgroundList)
            bg->clearSelection();
        emit cursorUpdated(m_defaultCursor);
    }
}

// key press event handler
void XWinKillPreviewWidget::keyPressEvent(QKeyEvent *event)
{
    // ESC pressed
    if (event->key() == Qt::Key_Escape) {
        // restore main window state
        auto *mw = gApp->mainWindow();
        mw->setWindowState((mw->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);

        close();
    }
}

// initialize ui components
void XWinKillPreviewWidget::initUI()
{
    Qt::WindowFlags flags {};
    // frameless style
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::Window;
    // always on top
    flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    // transparent background
    setAttribute(Qt::WA_TranslucentBackground);
    // self free after close
    setAttribute(Qt::WA_DeleteOnClose, true);

    // rquest exclusive owning keyboard & mouse
    grabKeyboard();
    setMouseTracking(true);
    grabMouse();

    // styled kill cursor
    m_killCursor = QCursor(iconPathFromQrc("kill_cursor.svg"));
    // default forbid style cursor
    m_defaultCursor = QCursor(Qt::ForbiddenCursor);

    // show background window in all screens
    for (auto screen : QApplication::screens()) {
        // screen geometry
        auto geom = screen->geometry();
        // snapshot current screen
        auto pixmap = screen->grabWindow(m_wminfo->getRootWindow(), geom.x(), geom.y(), geom.width(), geom.height());

        // create preview background widget for each screen
        auto *background = new XWinKillPreviewBackgroundWidget(pixmap, this);
        // update cursor on cursor updated signal
        connect(this, &XWinKillPreviewWidget::cursorUpdated, background, &XWinKillPreviewBackgroundWidget::setCursor);
        // resize background widget to current screen size
        background->resize(geom.size());
        // move background widget to current screen position
        background->move(geom.x(), geom.y());

        // keep a list of each background widget & screen
        m_backgroundList << background;
        m_screens << screen;

        // show background preview widget on each screen
        background->show();
    }
}

// initialize connections (nothing to do here)
void XWinKillPreviewWidget::initConnections()
{
}
