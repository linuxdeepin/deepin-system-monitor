#include <QObject>
#include <QDebug>
#include <QtX11Extras/QX11Info>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "find_window_title.h"

FindWindowTitle::FindWindowTitle()
{
    int screenNum;
    conn = xcb_connect(0, &screenNum);
    xcb_screen_t* screen = xcb_aux_get_screen(conn, screenNum);
    rootWindow = screen->root;
}

FindWindowTitle::~FindWindowTitle()
{
    delete conn;
}

QString FindWindowTitle::getAtomName(xcb_atom_t atom)
{
    QString result;

    xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(conn, atom);
    xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(conn, cookie, NULL);

    if (reply) {
        result = QString::fromLatin1(xcb_get_atom_name_name(reply), xcb_get_atom_name_name_length(reply));
        free(reply);
    }

    return result;

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
    }

    windowTitles = new QMap<int, QString>();
    foreach (auto window, windows) {
        int pid = getWindowPid(window);
        if (!windowTitles->contains(pid)) {
            (*windowTitles)[pid] = getWindowName(window);
        }
    }
}

xcb_atom_t FindWindowTitle::getAtom(QString name)
{
    QByteArray rawName = name.toLatin1();
    xcb_atom_t result = XCB_ATOM_NONE;
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, rawName.size(), rawName.data());
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, NULL);
    if(reply) {
        result = reply->atom;

        free(reply);
    }

    return result;
}

QStringList FindWindowTitle::getWindowTypes(xcb_window_t window)
{
    QStringList types;
    xcb_get_property_reply_t *reply = getProperty(window, "_NET_WM_WINDOW_TYPE", XCB_ATOM_ATOM);

    if(reply) {
        xcb_atom_t *typesA = static_cast<xcb_atom_t*>(xcb_get_property_value(reply));
        int typeNum = reply->length;

        for(int i = 0; i < typeNum; i++) {
            types.append(getAtomName(typesA[i]));
        }

        free(reply);
    }

    return types;
}

xcb_get_property_reply_t* FindWindowTitle::getProperty(xcb_window_t window, QString propertyName, xcb_atom_t type)
{
    xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, window, getAtom(propertyName), type, 0, UINT32_MAX);
    return xcb_get_property_reply(conn, cookie, NULL);
}

QString FindWindowTitle::getWindowName(xcb_window_t window)
{
    if (window == rootWindow) {
        return tr("Desktop");
    } else {
        xcb_get_property_reply_t *reply = getProperty(window, "_NET_WM_NAME", getAtom("UTF8_STRING"));

        if(reply) {
            QString result = QString::fromUtf8(static_cast<char*>(xcb_get_property_value(reply)), xcb_get_property_value_length(reply));

            free(reply);

            return result;
        } else {
            return QString();
        }
    }
}

int FindWindowTitle::getWindowPid(xcb_window_t window)
{
    xcb_get_property_reply_t *reply = getProperty(window, "_NET_WM_PID", XCB_ATOM_CARDINAL);
    int pid = 0;

    if (reply) {
        pid = *((int *) xcb_get_property_value(reply));

        free(reply);
    }

    return pid;
}

QString FindWindowTitle::findWindowTitle(int pid)
{
    if (windowTitles->contains(pid)) {
        return windowTitles->value(pid);
    } else {
        return QString("");
    }
}
