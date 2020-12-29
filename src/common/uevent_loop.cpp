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
#include "uevent_loop.h"

#include "common/common.h"

#include <QDebug>

#include <unistd.h>

using namespace common::compat;

namespace common {
namespace core {

struct callback_cookie_t {
    UEventFilter *filter;
    struct event *evp;
    int ctx;
    int __unused__; // padding
};

void cbEventFilter(int fd, short flag, void *cookie)
{
    auto *ctx = reinterpret_cast<struct callback_cookie_t *>(cookie);
    if (ctx) {
        std::unique_ptr<UEvent> evp;
        if (flag & EV_READ) {
            evp.reset(new UIOReadEvent(fd));
        } else if (flag & EV_WRITE) {
            evp.reset(new UIOWriteEvent(fd));
        } else if (flag & EV_TIMEOUT) {
            // timerId
            auto timerId = ctx->ctx;
            evp.reset(new UTimerEvent(timerId));
        } else if (flag & EV_SIGNAL) {
            evp.reset(new USignalEvent(fd));
        } else {
            return;
        }
        ctx->filter->uevent(evp.get());
    }
}

UEventLoop::UEventLoop(QObject *parent)
    : QObject(parent)
    , m_loop(event_base_new())
{
    int fds[2];
    int rc = pipe(fds);
    if (!rc && m_loop) {
        m_exitfd = fds[1];

        // event loop
        rc = event_base_priority_init(m_loop, UEvent::kEventPriorityMax);
        if (!rc) {
            // install exit event filter
            Callback<void(evutil_socket_t, int, void *)>::func = std::bind(&UEventLoop::exitRequested,
                                                                           this,
                                                                           std::placeholders::_1,
                                                                           std::placeholders::_2,
                                                                           std::placeholders::_3);
            auto exit_cb = static_cast<event_callback_fn>(Callback<void(evutil_socket_t, short, void *)>::callback);
            auto *exit_ev = event_new(m_loop, fds[0], EV_READ, exit_cb, this);
            rc = event_priority_set(exit_ev, UEvent::kEventPriorityHighest);
            if (!rc)
                event_add(exit_ev, nullptr);
        }
    }
}

UEventLoop::~UEventLoop()
{
    evutil_closesocket(m_exitfd);

    for (auto &cookie : m_cookieDB) {
        event_free(cookie->evp);
        delete cookie;
    }

    event_base_free(m_loop);
    libevent_global_shutdown();
}

void UEventLoop::run()
{
    if (!m_running.load()) {
        m_running.store(true);
        m_cookieDB.clear();
        event_base_loop(m_loop, EVLOOP_NO_EXIT_ON_EMPTY);
    }
    m_running.store(false);
}

// quit event loop
void UEventLoop::exit()
{
    if (m_running.load()) {
        // writing less than PIPE_BUF bytes into pipe is atomic guaranteed, so no lock is needed here
        char buf[] = {1};
        write(m_exitfd, buf, 1);
    }
}

bool UEventLoop::installReadEventFilter(int readfd,
                                        bool persist,
                                        UEvent::Priority priority,
                                        const timeval *timeout,
                                        UEventFilter *filter)
{
    short flag = EV_READ;
    if (persist) {
        flag |= EV_PERSIST;
    }

    bool ok = installEventFilter(readfd, flag, priority, timeout, filter);

    return ok;
}

bool UEventLoop::installWriteEventFilter(int writefd,
                                         bool persist,
                                         UEvent::Priority priority,
                                         const timeval *timeout,
                                         UEventFilter *filter)
{
    short flag = EV_WRITE;
    if (persist) {
        flag |= EV_PERSIST;
    }

    bool ok = installEventFilter(writefd, flag, priority, timeout, filter);

    return ok;
}

bool UEventLoop::installSignalEventFilter(int signal,
                                          UEvent::Priority priority,
                                          const timeval *timeout,
                                          UEventFilter *filter)
{
    short flag = EV_SIGNAL | EV_PERSIST;
    bool ok = installEventFilter(signal, flag, priority, timeout, filter);

    return ok;
}

bool UEventLoop::installTimerEventFilter(int timerId,
                                         bool persist,
                                         UEvent::Priority priority,
                                         const timeval *timeout,
                                         UEventFilter *filter)
{
    short flag = EV_TIMEOUT;
    if (persist) {
        flag |= EV_PERSIST;
    }

    bool ok = installEventFilter(timerId, flag, priority, timeout, filter);

    return ok;
}

// exit event handler
void UEventLoop::exitRequested(evutil_socket_t fd, short, void *)
{
    char buf[16];
    while (read(fd, buf, 15)) {}
    event_base_loopbreak(m_loop);
}

bool UEventLoop::installEventFilter(int fd,
                                    short flag,
                                    UEvent::Priority priority,
                                    const timeval *timeout,
                                    UEventFilter *filter)
{
    bool ok {true};

    struct event *ev {};
    struct callback_cookie_t *cookie {};

    int xfd;

    cookie = new struct callback_cookie_t();
    cookie->filter = filter;
    cookie->ctx = fd;

    if (flag & (EV_READ | EV_WRITE | EV_SIGNAL)) {
        xfd = fd;
    } else if (flag & EV_TIMEOUT) {
        xfd = -1;
    } else {
        return !ok;
    }
    ev = event_new(m_loop, xfd, flag, cbEventFilter, reinterpret_cast<void *>(cookie));
    if (!ev)
        return !ok;

    // set event filter priority
    event_priority_set(ev, priority);

    int rc = event_add(ev, timeout);
    if (!rc) {
        cookie->evp = ev;
        m_cookieDB << cookie;

        // activate timer event for once directly after added
        if (xfd == -1)
            event_active(ev, flag, 0);
    }
    return rc == 0;
}

} // namespace core
} // namespace common
