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

#include "find_window_title.h"
#include <QDebug>
#include <QObject>
#include <QtX11Extras/QX11Info>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

FindWindowTitle::FindWindowTitle()
{
    windowTitles = new QMap<int, xcb_window_t>();
}

FindWindowTitle::~FindWindowTitle()
{
    windowTitles->clear();
    delete windowTitles;
}

QList<int> FindWindowTitle::getWindowPids()
{
    return windowTitles->keys();
}

QString FindWindowTitle::getWindowTitle(int pid)
{
    if (windowTitles->contains(pid)) {
        return getWindowName(windowTitles->value(pid));
    } else {
        return QString("");
    }
}

void FindWindowTitle::updateWindowInfos()
{
    QList<xcb_window_t> windows;

    xcb_get_property_reply_t *listReply = getProperty(rootWindow, "_NET_CLIENT_LIST_STACKING", XCB_ATOM_WINDOW);

    if (listReply) {
        xcb_window_t *windowList = static_cast<xcb_window_t*>(xcb_get_property_value(listReply));
        int windowListLength = listReply->length;

        for (int i = 0; i < windowListLength; i++) {
            xcb_window_t window = windowList[i];

            foreach(QString type, getWindowTypes(window)) {
                if (type == "_NET_WM_WINDOW_TYPE_NORMAL" || type == "_NET_WM_WINDOW_TYPE_DIALOG") {
                    windows.append(window);
                }
            }
        }

        free(listReply);

        windowTitles->clear();
        foreach (auto window, windows) {
            int pid = getWindowPid(window);
            if (!windowTitles->contains(pid)) {
                (*windowTitles)[pid] = window;
            }
        }
    }
}

xcb_window_t FindWindowTitle::getWindow(int pid)
{
    if (windowTitles->contains(pid)) {
        return windowTitles->value(pid);
    } else {
        return -1;
    }
}

