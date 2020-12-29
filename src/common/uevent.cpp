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
#include "uevent.h"

namespace common {
namespace core {

UEvent::UEvent(UEvent::Type _type, UEvent::Priority prio)
    : type(_type)
    , priority(prio)
{
}

UIOEvent::UIOEvent(int d)
    : UEvent()
    , fd(d)
{
}

UTimerEvent::UTimerEvent(int id)
    : UEvent(UEvent::kEventTypeTimer)
    , timerId(id)
{
}

USignalEvent::USignalEvent(int num)
    : UEvent(UEvent::kEventTypeSignal)
    , signum(num)
{
}

UIOWriteEvent::UIOWriteEvent(int fd)
    : UIOEvent(fd)
{
    type = UEvent::kEventTypeIOWrite;
}

UIOReadEvent::UIOReadEvent(int fd)
    : UIOEvent(fd)
{
    type = UEvent::kEventTypeIORead;
}

} // namespace core
} // namespace common
