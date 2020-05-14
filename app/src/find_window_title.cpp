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

#include "find_window_title.h"
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <QDebug>
#include <QObject>
#include <QtX11Extras/QX11Info>
#include "utils.h"

FindWindowTitle::FindWindowTitle() {}

FindWindowTitle::~FindWindowTitle() {}

QList<int> FindWindowTitle::getWindowPids()
{
    return m_windowTitles.keys();
}

QString FindWindowTitle::getWindowTitle(int pid)
{
    if (m_windowTitles.contains(pid)) {
        return getWindowName(m_windowTitles.value(pid));
    } else {
        return {};
    }
}

void FindWindowTitle::updateWindowInfos()
{
    QList<xcb_window_t> windows;

    xcb_get_property_reply_t *listReply =
        getProperty(rootWindow, "_NET_CLIENT_LIST_STACKING", XCB_ATOM_WINDOW);

    if (listReply) {
        xcb_window_t *windowList = static_cast<xcb_window_t *>(xcb_get_property_value(listReply));
        uint windowListLength = listReply->length;

        for (uint i = 0; i < windowListLength; i++) {
            xcb_window_t window = windowList[i];

            foreach (QString type, getWindowTypes(window)) {
                if (type.contains("_NET_WM_WINDOW_TYPE_NORMAL") ||
                    type.contains("_NET_WM_WINDOW_TYPE_DIALOG")) {
                    windows.append(window);
                }
            }
        }

        free(listReply);

        m_windowTitles.clear();
        foreach (auto window, windows) {
            int pid = Utils::getWindowPid(this, window);

            if (!m_windowTitles.contains(pid)) {
                m_windowTitles[pid] = window;
            }
        }
    }
}

xcb_window_t FindWindowTitle::getWindow(int pid)
{
    if (m_windowTitles.contains(pid)) {
        return m_windowTitles.value(pid);
    } else {
        return UINT_MAX;
    }
}
