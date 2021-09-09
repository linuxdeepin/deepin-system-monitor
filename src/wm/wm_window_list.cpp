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
#include "wm_window_list.h"
#include "wm_atom.h"
#include "common/thread_manager.h"
#include "system/system_monitor.h"
#include "system/system_monitor_thread.h"
#include "process/process_db.h"
#include "common/common.h"

#include <QtDBus>

#include <xcb/xcb.h>

using namespace core::process;
using namespace common::core;
using namespace common::init;
using namespace core::system;

namespace core {
namespace wm {

struct XReplyDeleter {
    void operator()(void *p)
    {
        free(p);
    }
};
using XGetPropertyReply = std::unique_ptr<xcb_get_property_reply_t, XReplyDeleter>;
using XGetAtomNameReply = std::unique_ptr<xcb_get_atom_name_reply_t, XReplyDeleter>;

const int maxImageW = 1024;
const int maxImageH = 1024;
const int offsetImagePointerWH = 2;

WMWindowList::WMWindowList(QObject *parent)
    : QObject(parent)
{
}

void WMWindowList::addDesktopEntryApp(Process *proc)
{
    if (!proc)
        return;

    auto pid = proc->pid();

    if (!isTrayApp(pid)
            && !isGuiApp(pid)
            && !m_desktopEntryCache.contains(pid)) {

        // check if cmd is a shell or scripting language
        auto isCmdInList = [ = ](const QString & cmd) {
            auto subCmd = cmd.mid(cmd.lastIndexOf('/') + 1);
            for (auto s : shellList) {
                if (subCmd.startsWith(s.toLocal8Bit())) {
                    return true;
                }
            }
            for (auto s : scriptList) {
                if (cmd.startsWith(s.toLocal8Bit())) {
                    return true;
                }
            }
            return false;
        };

        bool bb = false;
        if (proc->cmdline().size() > 0) {
            auto cmd = proc->cmdline()[0];
            if (cmd.startsWith("/")) {
                // cmd starts with full path
                bb = isCmdInList(cmd);
            } else {
                // cmd starts with raw name
                for (auto p : pathList) {
                    bb = isCmdInList(p.append('/').append(cmd));
                    if (bb) {
                        break;
                    }
                }
            }
        }

        if (!bb)
            m_desktopEntryCache << pid;
    }
}

int WMWindowList::getAppCount()
{
    return static_cast<int>(m_trayAppcache.size() + m_guiAppcache.size()) + m_desktopEntryCache.size();
}

bool WMWindowList::isTrayApp(pid_t pid) const
{
    return m_trayAppcache.find(pid) != m_trayAppcache.end();
}

bool WMWindowList::isGuiApp(pid_t pid) const
{
    return m_guiAppcache.find(pid) != m_guiAppcache.end();
}

bool WMWindowList::isDesktopEntryApp(pid_t pid) const
{
    return m_desktopEntryCache.contains(pid);
}

void WMWindowList::removeDesktopEntryApp(pid_t pid)
{
    m_desktopEntryCache.removeAll(pid);
}

QImage WMWindowList::getWindowIcon(pid_t pid) const
{
    auto search = m_guiAppcache.find(pid);
    WMWId winId = UINT32_MAX;
    if (search != m_guiAppcache.end()) {
        winId = search->second->winId;
    }

    auto *conn = m_conn.xcb_connection();
    auto cookie = xcb_get_property(conn, false, winId, m_conn.atom(WMAtom::_NET_WM_ICON), XCB_ATOM_ANY, 0, UINT32_MAX);
    XGetPropertyReply reply(xcb_get_property_reply(conn, cookie, nullptr));

    if (reply) {
        int len = xcb_get_property_value_length(reply.get());
        if (len < 2)
            return {};

        uint *data = reinterpret_cast<uint *>(xcb_get_property_value(reply.get()));
        if (data)
        {
            //get the maximum image from data
            int max_w = 0;
            int max_h = 0;

            uint *max_icon = nullptr;
            uint *data_end = reinterpret_cast<uint *>(xcb_get_property_value_end(reply.get()).data);

            while ((data + offsetImagePointerWH) < data_end) {

                int w = static_cast<int>(data[0]);
                int h = static_cast<int>(data[1]);
                int size = w * h;

                data += offsetImagePointerWH;

                if (size <= 0 || w > maxImageW || h > maxImageH) {
                    break;
                }

                if (w > max_w || h > max_h) {
                    max_icon = data;
                    max_w = w;
                    max_h = h;
                }

                data += size;
            }

            if (max_icon != nullptr) {
                if (max_w > maxImageW || max_h > maxImageH) {
                    return QImage();
                }

                QImage img(max_w, max_h, QImage::Format_ARGB32);
                int byteCount = img.byteCount() / 4;

                for (int i = 0; i < byteCount; ++i) {
                    //Save covert uchar* to uint*
                    (reinterpret_cast<uint*>(img.bits()))[i] = max_icon[i];
                }
                return img;
            }
        }



    }
    return {};
}

QString WMWindowList::getWindowTitle(pid_t pid) const
{
    // process may have multiple window opened, each with a different title
    auto range = m_guiAppcache.equal_range(pid);
    QList<QString> titles;
    for (auto i = range.first; i != range.second; ++i)
        titles << i->second->title;

    if (titles.size() > 1) {
        QString title;
        // format: [abc]|[def]|[ghi]...
        for (int j = titles.size() - 1; j >= 0; j--) {
            title.append(QString("[%1]").arg(titles[j]));
            if (j > 0)
                title.append(" | ");
        }
        return title;
    } else if (titles.size() == 1) {
        return titles[0];
    } else {
        return {};
    }
}

QList<WMWId> WMWindowList::getTrayWindows() const
{
    QDBusInterface busInterface("com.deepin.dde.TrayManager", "/com/deepin/dde/TrayManager",
                                "org.freedesktop.DBus.Properties", QDBusConnection::sessionBus());
    QDBusMessage reply = busInterface.call("Get", "com.deepin.dde.TrayManager", "TrayIcons");
    QVariant v = reply.arguments().first();
    const QDBusArgument &argument = v.value<QDBusVariant>().variant().value<QDBusArgument>();

    argument.beginArray();
    QList<WMWId> winIds;
    while (!argument.atEnd()) {
        WMWId winId;

        argument >> winId;
        winIds << winId;
    }
    argument.endArray();

    return winIds;
}

void WMWindowList::updateWindowListCache()
{
    m_desktopEntryCache.clear();

    auto *conn = m_conn.xcb_connection();
    // get window info
    auto cookie = xcb_get_property(conn, 0, m_conn.rootWindow(), m_conn.atom(WMAtom::_NET_CLIENT_LIST_STACKING), XCB_ATOM_WINDOW, 0, UINT_MAX);
    xcb_flush(conn);
    XGetPropertyReply reply(xcb_get_property_reply(conn, cookie, nullptr));
    if (!reply)
        return;

    m_guiAppcache.clear();
    const xcb_get_property_reply_t *R = reply.get();
    xcb_window_t *clientList = reinterpret_cast<xcb_window_t *>(xcb_get_property_value(R));
    int len = xcb_get_property_value_length(R);

    for (int i = 0, index = 0; index < len; i++, index += sizeof(xcb_window_t)) {
        auto wid = clientList[i];
        auto winfo = getWindowInfo(wid);
        const QStringList &windowtype = getWindowType(wid);
        if (winfo && (windowtype.contains("_NET_WM_WINDOW_TYPE_NORMAL") || windowtype.contains("_NET_WM_WINDOW_TYPE_DIALOG"))) {
            m_guiAppcache[winfo->pid] = std::move(winfo);
        }
    }

    m_trayAppcache.clear();
    const QList<WMWId> &trayWndList = getTrayWindows();
    for (auto i = 0; i < trayWndList.size(); i++) {
        auto wid = trayWndList[i];
        auto winfo = getWindowInfo(wid);
        if (winfo && winfo.get()->pid > 0)
            m_trayAppcache.insert({winfo->pid, std::move(winfo)});
    }
}

pid_t WMWindowList::getWindowPid(WMWId winId) const
{
    auto *conn = m_conn.xcb_connection();
    auto pidCookie = xcb_get_property(conn, 0, winId, m_conn.atom(WMAtom::_NET_WM_PID), XCB_ATOM_CARDINAL, 0, 4);

    XGetPropertyReply pidReply(xcb_get_property_reply(conn, pidCookie, nullptr));
    if (pidReply && pidReply->type == XCB_ATOM_CARDINAL) {
        auto *pid = reinterpret_cast<pid_t *>(xcb_get_property_value(pidReply.get()));
        return *pid;
    } else
        return -1;
}

QByteArray WMWindowList::getAtomName(xcb_connection_t *conn, xcb_atom_t atom) const
{
    AtomMeta meta(new struct atom_meta());
    meta->atom = atom;

    auto cookie = xcb_get_atom_name(conn, atom);
    XGetAtomNameReply reply(xcb_get_atom_name_reply(conn, cookie, nullptr));
    if (reply) {
        auto len = xcb_get_atom_name_name_length(reply.get());
        auto name = xcb_get_atom_name_name(reply.get());
        meta->name = QByteArray{name, len};
    }

    return meta->name;
}

QStringList WMWindowList::getWindowType(WMWId winId) const
{
    QStringList windowType;
    auto *conn = m_conn.xcb_connection();
    auto windowTypeCookie = xcb_get_property(conn, 0, winId, m_conn.atom(WMAtom::_NET_WM_WINDOW_TYPE), XCB_ATOM_ATOM, 0, BUFSIZ);

    XGetPropertyReply windowTypeReply(xcb_get_property_reply(conn, windowTypeCookie, nullptr));
    if (windowTypeReply && windowTypeReply->type != XCB_NONE && windowTypeReply->value_len > 0) {
        auto *atoms = reinterpret_cast<xcb_atom_t *>(xcb_get_property_value(windowTypeReply.get()));
        Q_ASSERT(atoms != nullptr);
        for (uint32_t i = 0; i < windowTypeReply->value_len; ++i) {
            windowType << getAtomName(conn, atoms[i]);
        } // !for
    }
    return windowType;
}

WMWindow WMWindowList::getWindowInfo(WMWId winId)
{
    WMWindow window(new struct wm_window_t());
    window->winId = winId;

    auto *conn = m_conn.xcb_connection();
    // pid
    auto pidCookie = xcb_get_property(conn, 0, winId, m_conn.atom(WMAtom::_NET_WM_PID), XCB_ATOM_CARDINAL, 0, 4);
    // title
    auto netNameCookie = xcb_get_property(conn, 0, winId, m_conn.atom(WMAtom::_NET_WM_NAME), m_conn.atom(WMAtom::UTF8_STRING), 0, BUFSIZ);

    xcb_flush(conn);

    XGetPropertyReply pidReply(xcb_get_property_reply(conn, pidCookie, nullptr));
    if (pidReply && pidReply->type == XCB_ATOM_CARDINAL) {
        auto *pid = reinterpret_cast<pid_t *>(xcb_get_property_value(pidReply.get()));
        window->pid = *pid;
    } else
        window->pid = -1;

    const char *name {};
    int len {};
    xcb_atom_t encoding = XCB_NONE;
    XGetPropertyReply netNameReply(xcb_get_property_reply(conn, netNameCookie, nullptr));
    if (netNameReply && netNameReply->type != XCB_NONE) {
        name = reinterpret_cast<const char *>(xcb_get_property_value(netNameReply.get()));
        len = xcb_get_property_value_length(netNameReply.get());
        encoding = netNameReply->type;
    } else {
        auto nameCookie = xcb_icccm_get_wm_name(conn, winId);
        XGetPropertyReply nameReply(xcb_get_property_reply(conn, nameCookie, nullptr));
        if (nameReply && nameReply->type != XCB_NONE) {
            name = reinterpret_cast<const char *>(xcb_get_property_value(nameReply.get()));
            len = xcb_get_property_value_length(nameReply.get());
            encoding = nameReply->type;
        }
    }

    if (len != 0) {
        if (encoding == XCB_ATOM_STRING) {
            window->title = QString::fromLocal8Bit(name, len);
        } else if (encoding == m_conn.atom(WMAtom::UTF8_STRING)) {
            window->title = QString::fromUtf8(name, len);
        }
    }

    return window;
}

} // namespace wm
} // namespace core
