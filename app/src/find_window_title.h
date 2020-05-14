/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
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

#ifndef FINDWINDOWTITLE_H
#define FINDWINDOWTITLE_H

#include <dwindowmanager.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <QMap>

DWM_USE_NAMESPACE

class FindWindowTitle : public DWindowManager
{
    Q_OBJECT

public:
    FindWindowTitle();
    ~FindWindowTitle();

    QList<int> getWindowPids();
    QString getWindowTitle(int pid);
    void updateWindowInfos();
    xcb_window_t getWindow(int pid);

private:
    QMap<int, xcb_window_t> m_windowTitles;
};

#endif
