/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      Wang Yong <wangyong@deepin.com>
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

#include "interactive_kill.h"

#include "start_tooltip.h"
#include "gui/ui_common.h"
#include "utils.h"

#include <dscreenwindowsutil.h>

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>
#include <QIcon>
#include <QKeyEvent>
#include <QPaintEvent>

DWM_USE_NAMESPACE

InteractiveKill::InteractiveKill(QWidget *parent)
    : QWidget(parent)
{
    m_cursorImage = QIcon(iconPathFromQrc("kill_cursor.svg")).pixmap({48, 48});
    cursorX = -1;
    cursorY = -1;
    killWindowIndex = -1;

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setMouseTracking(true);    // make MouseMove can response
    installEventFilter(this);  // add event filter

    QPoint pos = this->cursor().pos();
    DScreenWindowsUtil *screenWin = DScreenWindowsUtil::instance(pos);
    screenRect = screenWin->backgroundRect();
    this->move(screenRect.x(), screenRect.y());
    this->setFixedSize(screenRect.width(), screenRect.height());

    windowManager = new DWindowManager();
    windowManager->setRootWindowRect(screenRect);
    windows = windowManager->getWindows();

    for (int i = 0; i < windows.length(); i++) {
        if (windowManager->getWindowClass(windows[i]) == "Desktop") {
            m_desktopRect = windowManager->getWindowRect(windows[i]);
        }
        if (windowManager->getWindowClass(windows[i]) != "deepin-screen-recorder" &&
                windowManager->getWindowClass(windows[i]) != "Desktop") {
            windowRects.append(
                windowManager->adjustRectInScreenArea(windowManager->getWindowRect(windows[i])));
            windowPids.append(Utils::getWindowPid(windowManager, windows[i]));
        }
    }

    startTooltip = new StartTooltip();
    startTooltip->setWindowManager(windowManager);
    startTooltip->show();

    QList<QScreen *> screenList = qApp->screens();
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    int screenNum = qApp->desktop()->screenNumber(pos);

    if (screenNum != 0 && screenNum < screenList.length()) {
        screenPixmap = screenList[screenNum]->grabWindow(screenWin->rootWindowId(), screenRect.x(),
                                                         screenRect.y(), screenRect.width(),
                                                         screenRect.height());
    } else {
        screenPixmap =
            primaryScreen->grabWindow(screenWin->rootWindowId(), screenRect.x(), screenRect.y(),
                                      screenRect.width(), screenRect.height());
    }

    showFullScreen();
}

InteractiveKill::~InteractiveKill()
{
    delete startTooltip;
    delete windowManager;
}

void InteractiveKill::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() == Qt::Key_Escape) {
        close();
    }
}

void InteractiveKill::mouseMoveEvent(QMouseEvent *mouseEvent)
{

    cursorX = mouseEvent->x();
    cursorY = mouseEvent->y();

    for (int i = 0; i < windowRects.length(); i++) {
        WindowRect rect = windowRects[i];

        if (cursorX + screenRect.x() >= rect.x &&
                cursorX + screenRect.x() <= rect.x + rect.width &&
                cursorY + screenRect.y() >= rect.y &&
                cursorY + screenRect.y() <= rect.y + rect.height) {
            rect.x = rect.x - screenRect.x();
            rect.y = rect.y - screenRect.y();

            killWindowRect = rect;
            killWindowIndex = i;

            break;
        }
    }

    if (mouseHoverWindow({cursorX, cursorY})) {
        setCursor(QCursor(m_cursorImage));
    } else {
        setCursor(QCursor(Qt::ForbiddenCursor));
    }

    update();
}

void InteractiveKill::mousePressEvent(QMouseEvent *mouseEvent)
{
    if (!mouseHoverWindow(mouseEvent->pos())) {
        QWidget::mousePressEvent(mouseEvent);
    } else {
        if (startTooltip != nullptr) {
            delete startTooltip;
            startTooltip = nullptr;
        }

        for (int i = 0; i < windowRects.length(); i++) {
            WindowRect rect = windowRects[i];

            if (mouseEvent->x() >= rect.x && mouseEvent->x() <= rect.x + rect.width &&
                    mouseEvent->y() >= rect.y && mouseEvent->y() <= rect.y + rect.height) {
                killWindow(windowPids[i]);

                break;
            }
        }
        close();
    }
}

void InteractiveKill::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(QPoint(0, 0), screenPixmap);

    if (cursorX >= 0 && cursorY >= 0 && mouseHoverWindow({cursorX, cursorY})) {
        // Just render kill window with window index.
        QRegion windowRegion = QRegion(
                                   QRect(killWindowRect.x, killWindowRect.y, killWindowRect.width, killWindowRect.height));

        for (int i = 0; i < killWindowIndex; i++) {
            WindowRect rect = windowRects[i];

            if (rect.x > killWindowRect.x ||
                    rect.x + rect.width < killWindowRect.x + killWindowRect.width ||
                    rect.y > killWindowRect.y ||
                    rect.y + rect.height < killWindowRect.y + killWindowRect.height) {
                windowRegion =
                    windowRegion.subtracted(QRegion(rect.x, rect.y, rect.width, rect.height));
            }
        }

        painter.setBrush(Qt::red);
        painter.setOpacity(0.2);

        painter.setClipping(true);
        painter.setClipRegion(windowRegion);
        painter.drawRect(
            QRect(killWindowRect.x, killWindowRect.y, killWindowRect.width, killWindowRect.height));
    }
}
