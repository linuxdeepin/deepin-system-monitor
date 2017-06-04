#include <QObject>
#include <QDebug>
#include <QtX11Extras/QX11Info>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "find_window_title.h"

FindWindowTitle::FindWindowTitle()
{
}

void FindWindowTitle::updateWindowInfos()
{
    QList<xcb_window_t> windows;
    
    xcb_get_property_reply_t *listReply = getProperty(getRootWindow(), "_NET_CLIENT_LIST_STACKING", XCB_ATOM_WINDOW);

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



QString FindWindowTitle::findWindowTitle(int pid)
{
    if (windowTitles->contains(pid)) {
        return windowTitles->value(pid);
    } else {
        return QString("");
    }
}
