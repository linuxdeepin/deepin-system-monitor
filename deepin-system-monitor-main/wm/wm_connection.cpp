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
#include "wm_connection.h"

#include <QDebug>

namespace core {
namespace wm {

WMConnection::WMConnection(const QByteArray &display)
{
    m_conn = XConnection(xcb_connect(display.isEmpty() ? nullptr : display.constData(), &m_screenNumber));
    auto *conn = m_conn.get();
    if (conn && xcb_connection_has_error(conn)) {
        qDebug() << "Unable to connect to X server";
        return;
    }
    m_atoms.initialize(conn);

    m_setup = xcb_get_setup(conn);
    auto iter = xcb_setup_roots_iterator(m_setup);
    auto screenCount = xcb_setup_roots_length(m_setup);
    if (screenCount < m_screenNumber) {
        qDebug() << QString("Unable to access to screen %1, max is %2").arg(m_screenNumber).arg(screenCount - 1);
        return;
    }

    // get the screen we want from a list of screens in linked list structure
    for (int i = 0; i < m_screenNumber; i++)
        xcb_screen_next(&iter);
    m_screen = iter.data;

    m_root = m_screen->root;
}

WMConnection::~WMConnection()
{
}

} // namespace wm
} // namespace core
