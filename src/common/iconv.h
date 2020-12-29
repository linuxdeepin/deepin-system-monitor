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
#ifndef ICONV_H
#define ICONV_H

#include <QCoreApplication>
#include <QMap>
#include <QString>

namespace common {
namespace icon {

enum {
    // context: Title.Bar.Switch
    kIconv_Processes,
    kIconv_Services,

    // context: Title.Bar.Search
    kIconv_Search,

    // context: Process.Priority

    //

    //

    //

    //

    //

    // context: time period
    kIconv_TimePeriod_60s,
    kIconv_TimePeriod_5m,
    kIconv_TimePeriod_15m,

    //

    //

    //
};

QMap<int, QString> loadToolbarText()
{
    QMap<int, QString> map;
    map[kIconv_Processes] = QCoreApplication::translate("Title.Bar.Switch", "Processes");
    map[kIconv_Services] = QCoreApplication::translate("Title.Bar.Switch", "Services");
    map[kIconv_Search] = QCoreApplication::translate("Title.Bar.Search", "Search");
    return map;
}

QMap<int, QString> loadProcessTableText()
{
    return {};
}

QMap<int, QString> loadTimePeriodText()
{
    QMap<int, QString> map;
    map[kIconv_TimePeriod_60s] = QCoreApplication::translate("Core", "60 seconds");
    map[kIconv_TimePeriod_5m] = QCoreApplication::translate("Core", "5 minutes");
    map[kIconv_TimePeriod_15m] = QCoreApplication::translate("Core", "15 minutes");
    return map;
}

} // namespace icon
} // namespace common

#endif // ICONV_H
