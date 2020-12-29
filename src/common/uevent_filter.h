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
#ifndef UEVENT_FILTER_H
#define UEVENT_FILTER_H

#include "uevent.h"

#include <QMap>
#include <QVariant>

namespace common {
namespace core {

/**
 * @brief UEvent filter interface class
 */
class UEventFilter
{
public:
    UEventFilter() = default;
    virtual ~UEventFilter() = default;

    virtual bool uevent(UEvent *event) = 0;
}; // UEventFilter

} // namespace core
} // namespace common

#endif // UEVENT_FILTER_H
