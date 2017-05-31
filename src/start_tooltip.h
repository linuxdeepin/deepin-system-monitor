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

#ifndef STARTTOOLTIP_H
#define STARTTOOLTIP_H

#include <QWidget>
#include "window_manager.h"

class StartTooltip : public QWidget
{
    Q_OBJECT
    
public:
    StartTooltip(QWidget *parent = 0);
    void setWindowManager(WindowManager *wm);
    
protected:
    void paintEvent(QPaintEvent *event);
    bool eventFilter(QObject *, QEvent *event);
    
private:
    QString text;
    
    QImage iconImg;
    WindowManager* windowManager;
};

#endif
