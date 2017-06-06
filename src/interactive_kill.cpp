/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#include "interactive_kill.h"
#include "utils.h"

#include <QDebug>
#include <QScreen>
#include <QApplication>

InteractiveKill::InteractiveKill(QWidget *parent) : QWidget(parent)
{
    cursorImage = QImage(Utils::getQrcPath("kill_cursor.png"));
    cursorX = -1;
    cursorY = -1;

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setMouseTracking(true);     // make MouseMove can response
    installEventFilter(this);   // add event filter

    windowManager = new WindowManager();
    QList<xcb_window_t> windows = windowManager->getWindows();

    for (int i = 0; i < windows.length(); i++) {
        if (windowManager->getWindowClass(windows[i]) != "deepin-screen-recorder") {
            windowRects.append(windowManager->adjustRectInScreenArea(windowManager->getWindowRect(windows[i])));
            windowPids.append(windowManager->getWindowPid(windows[i]));
        }
    }

    startTooltip = new StartTooltip();
    startTooltip->setWindowManager(windowManager);
    startTooltip->show();
    
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        screenPixmap = screen->grabWindow(0);
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
    QApplication::setOverrideCursor(Qt::BlankCursor);

    cursorX = mouseEvent->x();
    cursorY = mouseEvent->y();
    
    for (int i = 0; i < windowRects.length(); i++) {
        WindowRect rect = windowRects[i];

        if (mouseEvent->x() >= rect.x && mouseEvent->x() <= rect.x + rect.width &&
            mouseEvent->y() >= rect.y && mouseEvent->y() <= rect.y + rect.height) {
            killWindowRect = rect;

            break;
        }
    }

    repaint();
}

void InteractiveKill::mousePressEvent(QMouseEvent *mouseEvent)
{
    if (startTooltip != NULL) {
        delete startTooltip;
        startTooltip = NULL;
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

void InteractiveKill::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(QPoint(0, 0), screenPixmap);

    if (cursorX >=0 && cursorY >= 0) {
        QPainterPath path;
        path.addRect(QRectF(killWindowRect.x, killWindowRect.y, killWindowRect.width, killWindowRect.height));
        painter.setOpacity(0.2);
        painter.fillPath(path, QColor("#ff0000"));
        
        painter.setOpacity(1);
        painter.drawImage(QPoint(cursorX, cursorY), cursorImage);
    }
}
