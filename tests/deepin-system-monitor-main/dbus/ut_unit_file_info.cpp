/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      wangchao <wangchao@uniontech.com>
* Maintainer:  wangchao <wangchao@uniontech.com>
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

#include "dbus/unit_file_info.h"

#include <QString>
#include <QDebug>
#include <QExplicitlySharedDataPointer>
#include <QList>
#include <QtDBus>

#include "ut_unit_file_info.h"

Ut_UnitFileInfo::Ut_UnitFileInfo()
{
}

TEST(UT_UnitFileInfo_setName, UT_UnitFileInfo_setName_001)
{
    UnitFileInfo* info = new UnitFileInfo();

    const QString name("ABC");

    info->setName(name);
    QString retName = info->getName();

    EXPECT_EQ(name, retName);

    delete info;
}

TEST(UT_UnitFileInfo_getName, UT_UnitFileInfo_getName_001)
{
    const QString name("DEF");
    UnitFileInfo* info = new UnitFileInfo(name, "");
    QString retName = info->getName();

    EXPECT_EQ(name, retName);

    delete info;
}

TEST(UT_UnitFileInfo_getStatus, UT_UnitFileInfo_getStatus_001)
{
    const QString name("DEF");
    const QString status("XYZ");
    UnitFileInfo* info = new UnitFileInfo(name, status);
    QString retStatus = info->getStatus();

    EXPECT_EQ(status, retStatus);

    delete info;
}

TEST(UT_UnitFileInfo_setStatus, UT_UnitFileInfo_setStatus_001)
{
    UnitFileInfo* info = new UnitFileInfo();

    const QString status("ABC");
    info->setStatus(status);

    QString retStatus = info->getStatus();

    EXPECT_EQ(status, retStatus);

    delete info;
}
