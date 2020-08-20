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

#include "xwin_kill_preview_tooltip_widget.h"
#include "xwin_kill_preview_mask_widget.h"
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

#include <memory>
#include <list>

using namespace std;
using namespace util::wm;

XWinKillPreviewWidget::XWinKillPreviewWidget(QWidget *parent) : QWidget(parent)
{
    initUI();
    initConnections();

    m_wminfo = new WMInfo();
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
    for (auto &select : list) {
        if (SystemMonitor::getCurrentPID() == select->pid)
            continue;

        if (select->rect.contains(pos)) {
            m_mask->hide();
            for (auto &tooltip : m_tooltips) {
                tooltip->hide();
            }
            QApplication::setOverrideCursor(Qt::ArrowCursor);
            setMouseTracking(false);
            releaseMouse();
            releaseKeyboard();
            emit windowClicked(select->pid);
            break;
        }
    }
    QApplication::setOverrideCursor(Qt::ArrowCursor);
    close();
}

void XWinKillPreviewWidget::mouseMoveEvent(QMouseEvent *)
{
    auto pos = QCursor::pos();

    auto list = m_wminfo->selectWindow(pos);
    bool found {false};
    for (auto &select : list) {
        if (SystemMonitor::getCurrentPID() == select->pid)
            continue;

        if (select->rect.contains(pos)) {
            found = true;
            QApplication::setOverrideCursor(m_acceptCursor);
            m_mask->resize(select->rect.size());
            m_mask->move(select->rect.topLeft());
            m_mask->show();
            break;
        }
    }
    if (!found) {
        m_mask->hide();
        QApplication::setOverrideCursor(m_rejectCursor);
    }
}

void XWinKillPreviewWidget::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);

    if (event->key() == Qt::Key_Escape) {
        setMouseTracking(false);
        releaseMouse();
        releaseKeyboard();

        // restore main window
        auto *mw = MainWindow::instance();
        mw->setWindowState((mw->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);

        QApplication::setOverrideCursor(Qt::ArrowCursor);
        close();
    }
}

void XWinKillPreviewWidget::initUI()
{
    Qt::WindowFlags flags {};
    flags |= Qt::Window;
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::WindowStaysOnTopHint;
    flags |= Qt::CustomizeWindowHint;
    flags |= Qt::BypassWindowManagerHint;
    setWindowFlags(flags);
    setAttribute(Qt::WA_DeleteOnClose, true);
    grabKeyboard();
    setMouseTracking(true);
    grabMouse();

    // show tooltip window in all screens
    for (auto screen : QApplication::screens()) {
        auto *tooltip = new XWinKillPreviewTooltipWidget(this);
        tooltip->move(screen->availableGeometry().center() - tooltip->rect().center());
        tooltip->show();
        m_tooltips << tooltip;
    }

    m_mask = new XWinKillPreviewMaskWidget(this);
    m_cursorIcon = QIcon(iconPathFromQrc("kill_cursor.svg"));
    m_acceptCursor = QCursor(m_cursorIcon.pixmap({48, 48}));
    m_rejectCursor = QCursor(Qt::ForbiddenCursor);
}

void XWinKillPreviewWidget::initConnections()
{
}
