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
#ifndef UEVENT_LOOP_H
#define UEVENT_LOOP_H

#include "uevent_filter.h"

#include <QMap>
#include <QMutex>

#include <event2/event.h>

#include <memory>
#include <unordered_map>

namespace common {
namespace core {

void cbEventFilter(int fd, short flag, void *cookie);

/**
 * @brief The UEventLoop class
 */
struct callback_cookie_t;
class UEventLoop : QObject
{
    Q_OBJECT

public:
    explicit UEventLoop(QObject *parent = nullptr);
    ~UEventLoop();

    void run();
    void exit();
    inline bool isRunning()
    {
        return m_running.load();
    }

    /**
     * @brief installReadEventFilter
     * @param readfd
     * @param persist
     * @param priority
     * @param timeout The maximum amount of time to wait for the event, or NULL to wait forever
     * @param filter
     * @return
     */
    bool installReadEventFilter(int readfd,
                                bool persist,
                                UEvent::Priority priority,
                                const struct timeval *timeout,
                                UEventFilter *filter);
    /**
     * @brief installWriteEventFilter
     * @param writefd
     * @param persist
     * @param priority
     * @param timeout The maximum amount of time to wait for the event, or NULL to wait forever
     * @param filter
     * @return
     */
    bool installWriteEventFilter(int writefd,
                                 bool persist,
                                 UEvent::Priority priority,
                                 const struct timeval *timeout,
                                 UEventFilter *filter);
    /**
     * @brief installSignalEventFilter
     * @param signal
     * @param priority
     * @param timeout The maximum amount of time to wait for the event, or NULL to wait forever
     * @param filter
     * @return
     */
    bool installSignalEventFilter(int signal,
                                  UEvent::Priority priority,
                                  const struct timeval *timeout,
                                  UEventFilter *filter);
    /**
     * @brief installTimerEventFilter
     * @param timerId
     * @param persist
     * @param priority
     * @param timeout The maximum amount of time to wait for the event, or NULL to wait forever
     * @param filter
     * @return
     */
    bool installTimerEventFilter(int timerId,
                                 bool persist,
                                 UEvent::Priority priority,
                                 const struct timeval *timeout,
                                 UEventFilter *filter);

    /*
     * TODO: add EV_READ callback to handle aysnc adding event filter req.
    bool installReadEventFilterAsync();
    bool installWriteEventFilterAsync();
    bool installSignalEventFilterAsync();
    bool installTimerEventFilterAsync();
    */

protected:
    /**
     * @brief Exit event handle
     * @param fd
     * @param flag
     * @param cookie
     */
    void exitRequested(evutil_socket_t fd, short flag, void *cookie);

    /**
     * @brief installEventFilter
     * @param fd
     * @param flag
     * @param priority
     * @param timeout
     * @param filter
     * @return
     */
    bool installEventFilter(int fd,
                            short flag,
                            UEvent::Priority priority,
                            const struct timeval *timeout,
                            UEventFilter *filter);

private:
    QList<struct callback_cookie_t *> m_cookieDB;
    struct event_base *m_loop;

    int m_exitfd;
    std::atomic<bool> m_running {false};
}; // UEventLoop

} // namespace core
} // namespace common

#endif // UEVENT_LOOP_H
