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
#ifndef WM_INFO_H
#define WM_INFO_H

#include <QList>
#include <QRect>
#include <QPixmap>
#include <QString>
#include <QByteArray>
#include <QSharedPointer>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>

#include <memory>
#include <map>
#include <list>

namespace util {
// x11/xcb stuff
namespace wm {

struct wm_window_t;
struct wm_window_ext_t;
struct wm_request_t;
struct wm_frame_extents_t;
struct wm_tree_t;
struct atom_meta;

using WMWindow      = std::shared_ptr<struct wm_window_t>;
using WMWindowExt   = std::unique_ptr<struct util::wm::wm_window_ext_t>;
using AtomMeta      = std::unique_ptr<struct atom_meta>;
using WMWindowArea  = std::unique_ptr<struct wm_window_area_t>;
using WMWId         = xcb_window_t;
using WMTree        = std::unique_ptr<struct util::wm::wm_tree_t>;

struct wm_window_t {
    WMWId   wid;
    pid_t   pid;      // _NET_WM_PID
    QString title {}; // _NET_WM_NAME && UTF8_STRING || WM_NAME
    QPixmap icon  {};
};

struct atom_meta {
    QByteArray name;
    xcb_atom_t atom;
};

struct wm_window_area_t {
    WMWId wid;
    pid_t pid;
    QRect rect;
};

class WMInfo
{
    enum intern_atom_type {
        kOtherAtom              = -1,
        kNetNameAtom            = 0,
        kUTF8StringAtom,
        kNetDesktopAtom,
        kNetWindowTypeAtom,
        kNetStateAtom,
        kNetPIDAtom,
        kNetFrameExtentsAtom,
        kNetVirtualRootsAtom,
        kStateAtom
    };

public:
    WMInfo();
    ~WMInfo();

    // top level window (including wm frame) in top to bottom order that contains cursor
    std::list<WMWindowArea> selectWindow(const QPoint &pos);

private:
    std::map<pid_t, WMWindow> updateWindowStackCache();
    void buildWindowTreeSchema();

    void initAtomCache(xcb_connection_t *conn);
    inline xcb_atom_t getAtom(xcb_connection_t *conn, xcb_intern_atom_cookie_t &cookie);
    QByteArray getAtomName(xcb_connection_t *conn, xcb_atom_t atom);

    WMWindowExt requestWindowExtInfo(xcb_connection_t *conn, xcb_window_t window);

private:
    WMTree m_tree;

    std::map<intern_atom_type, xcb_atom_t>  m_internAtomCache;
    std::map<xcb_atom_t, AtomMeta>          m_atomCache;
};

} // !wm
} // !util
#endif // WM_INFO_H
