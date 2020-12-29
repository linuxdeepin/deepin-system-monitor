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
#ifndef UEVENT_H
#define UEVENT_H

#include <event2/event.h>

#include <memory>

namespace common {
namespace core {

/**
 * @brief The UEvent class
 */
class UEvent
{
public:
    enum Type {
        kEventTypeNone = 0,
        kEventTypeTimer = 1, // timer event
        kEventTypeIORead = 2, // file descriptor event
        kEventTypeIOWrite = 3, // file descriptor event
        kEventTypeSignal = 4, // file descriptor event
    };

    enum Priority {
        kEventPriorityHighest = 0,
        kEventPriorityHigh = 1,
        kEventPriorityNormal = 2,
        kEventPriorityLow = 3,
        kEventPriorityLowest = 4,

        kEventPriorityMax = kEventPriorityLowest + 1
    };

protected:
    explicit UEvent(UEvent::Type _type = kEventTypeNone,
                    UEvent::Priority prio = kEventPriorityNormal);
    virtual ~UEvent() = default;
    UEvent(const UEvent &) = delete;
    UEvent &operator=(const UEvent &) = delete;
    friend std::default_delete<UEvent>;

public:
    UEvent::Type type;
    Priority priority;
};

class UTimerEvent : public UEvent
{
public:
    explicit UTimerEvent(int id);
    ~UTimerEvent() = default;

public:
    int timerId;
};

class UIOEvent : public UEvent
{
protected:
    explicit UIOEvent(int _fd);
    ~UIOEvent() = default;

public:
    int fd;
};

class UIOReadEvent : public UIOEvent
{
public:
    explicit UIOReadEvent(int fd);
    ~UIOReadEvent() = default;
};

class UIOWriteEvent : public UIOEvent
{
public:
    explicit UIOWriteEvent(int fd);
    ~UIOWriteEvent() = default;
};

class USignalEvent : public UEvent
{
public:
    explicit USignalEvent(int num);
    ~USignalEvent() = default;

    int signum;
};

} // namespace core
} // namespace common

#endif // UEVENT_H
