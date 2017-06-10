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

#ifndef INTERACTIVEKILL_H
#define INTERACTIVEKILL_H

#include "start_tooltip.h"
#include "window_manager.h"
#include <QKeyEvent>
#include <QPaintEvent>
#include <QWidget>

class InteractiveKill : public QWidget
{
    Q_OBJECT
 
public:
    InteractiveKill(QWidget *parent = 0);
    ~InteractiveKill();
    
    void keyPressEvent(QKeyEvent *keyEvent);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    void paintEvent(QPaintEvent *);
    
signals:
    void killWindow(int pid);
    
private:
    QImage cursorImage;
    QList<WindowRect> windowRects;
    QList<int> windowPids;
    QPixmap screenPixmap;
    StartTooltip *startTooltip;
    WindowManager *windowManager;
    WindowRect killWindowRect;
    int cursorX;
    int cursorY;
    int killWindowIndex;
};

#endif
