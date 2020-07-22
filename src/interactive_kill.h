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

#ifndef INTERACTIVEKILL_H
#define INTERACTIVEKILL_H

#include <dwindowmanager.h>

#include <QWidget>

DWM_USE_NAMESPACE

class QPaintEvent;
class QKeyEvent;
class StartTooltip;

class InteractiveKill : public QWidget
{
    Q_OBJECT

public:
    InteractiveKill(QWidget *parent = nullptr);
    ~InteractiveKill();

    void keyPressEvent(QKeyEvent *keyEvent);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    void paintEvent(QPaintEvent *);

private:
    inline bool mouseHoverWindow(const QPoint &pos)
    {
        bool hover = false;
        for (auto &w : windowRects) {
            QRect crect(w.x, w.y, w.width, w.height);
            if (crect.contains(pos)) {
                hover = true;
                break;
            }
        }
        return hover;
    }


signals:
    void killWindow(int pid);

private:
    QPixmap m_cursorImage;
    QRect screenRect;
    QList<xcb_window_t> windows;
    QList<WindowRect> windowRects;
    QList<int> windowPids;
    QPixmap screenPixmap;
    StartTooltip *startTooltip;
    DWindowManager *windowManager;
    WindowRect killWindowRect;
    int cursorX;
    int cursorY;
    int killWindowIndex;

    WindowRect m_desktopRect {};
};

#endif
