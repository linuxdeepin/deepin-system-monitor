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

//#include "dbus/unit_file_info.h"
//#include <QString>
//#include <QDebug>
//#include <QExplicitlySharedDataPointer>
//#include <QList>
//#include <QtDBus>
#include "dbus/unit_info.h"

#include <QDebug>

#include "ut_unit_info.h"

Ut_UnitInfo::Ut_UnitInfo()
{
}

TEST(UT_UnitInfo_getName, UT_UnitInfo_getName_001)
{
    const QString name("ABC");
    UnitInfo* info = new UnitInfo(name, "", "", "", "", "", "", 0, "", "");

    QString retName = info->getName();

    EXPECT_EQ(name, retName);

    delete info;
}

TEST(UT_UnitInfo_setName, UT_UnitInfo_setName_001)
{
    const QString name("ABC");
    UnitInfo* info = new UnitInfo();
    info->setName(name);

    QString retName = info->getName();

    EXPECT_EQ(name, retName);

    delete info;
}

TEST(UT_UnitInfo_getDescription, UT_UnitInfo_getDescription_001)
{
    const QString desp("ABC");
    UnitInfo* info = new UnitInfo("", desp, "", "", "", "", "", 0, "", "");

    QString retDesp = info->getDescription();

    EXPECT_EQ(desp, retDesp);

    delete info;
}

TEST(UT_UnitInfo_setDescription, UT_UnitInfo_setDescription_001)
{
    const QString desp("ABC");
    UnitInfo* info = new UnitInfo();
    info->setDescription(desp);

    QString retDesp = info->getDescription();

    EXPECT_EQ(desp, retDesp);

    delete info;
}

TEST(UT_UnitInfo_getLoadState, UT_UnitInfo_getLoadState_001)
{
    const QString loadState("loadStateTest");
    UnitInfo* info = new UnitInfo("name", "description", loadState, "activeState",
                                  "subState", "followedBy", "unitObjectPath", 123, "jobType", "jobObjectPath");

    QString retLoadState = info->getLoadState();

    EXPECT_EQ(loadState, retLoadState);

    delete info;
}

TEST(UT_UnitInfo_setLoadState, UT_UnitInfo_setLoadState_001)
{
    const QString loadState("loadStateTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", "unitObjectPath", 123, "jobType", "jobObjectPath");

    info->setLoadState(loadState);
    QString retLoadState = info->getLoadState();

    EXPECT_EQ(loadState, retLoadState);

    delete info;
}

TEST(UT_UnitInfo_getActiveState, UT_UnitInfo_getActiveState_001)
{
    const QString activeState("activeStateTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", activeState,
                                  "subState", "followedBy", "unitObjectPath", 123, "jobType", "jobObjectPath");

    QString retActiveState = info->getActiveState();

    EXPECT_EQ(activeState, retActiveState);

    delete info;
}

TEST(UT_UnitInfo_setActiveState, UT_UnitInfo_setActiveState_001)
{
    const QString activeState("activeStateTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", "unitObjectPath", 123, "jobType", "jobObjectPath");

    info->setActiveState(activeState);
    QString ret = info->getActiveState();

    EXPECT_EQ(activeState, ret);

    delete info;
}

TEST(UT_UnitInfo_getSubState, UT_UnitInfo_getSubState_001)
{
    const QString testStr("subStateTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  testStr, "followedBy", "unitObjectPath", 123, "jobType", "jobObjectPath");

    QString ret = info->getSubState();

    EXPECT_EQ(testStr, ret);

    delete info;
}

TEST(UT_UnitInfo_setSubState, UT_UnitInfo_setSubState_001)
{
    const QString testStr("activeStateTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", "unitObjectPath", 123, "jobType", "jobObjectPath");

    info->setSubState(testStr);
    QString ret = info->getSubState();

    EXPECT_EQ(testStr, ret);

    delete info;
}

TEST(UT_UnitInfo_getFollowedBy, UT_UnitInfo_getFollowedBy_001)
{
    const QString testStr("followedByTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", testStr, "unitObjectPath", 123, "jobType", "jobObjectPath");

    QString ret = info->getFollowedBy();

    EXPECT_EQ(testStr, ret);

    delete info;
}

TEST(UT_UnitInfo_setFollowedBy, UT_UnitInfo_setFollowedBy_001)
{
    const QString testStr("followedByTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", "unitObjectPath", 123, "jobType", "jobObjectPath");

    info->setFollowedBy(testStr);
    QString ret = info->getFollowedBy();

    EXPECT_EQ(testStr, ret);

    delete info;
}

TEST(UT_UnitInfo_getUnitObjectPath, UT_UnitInfo_getUnitObjectPath_001)
{
    const QString testStr("unitObjectPathTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", testStr, 123, "jobType", "jobObjectPath");

    QString ret = info->getUnitObjectPath();

    EXPECT_EQ(testStr, ret);

    delete info;
}

TEST(UT_UnitInfo_setUnitObjectPath, UT_UnitInfo_setUnitObjectPath_001)
{
    const QString testStr("unitObjectPathTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", "unitObjectPath", 123, "jobType", "jobObjectPath");

    info->setUnitObjectPath(testStr);
    QString ret = info->getUnitObjectPath();

    EXPECT_EQ(testStr, ret);

    delete info;
}

TEST(UT_UnitInfo_getJobId, UT_UnitInfo_getJobId_001)
{
    const quint32 testInt = 456;
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", "unitObjectPath", testInt, "jobType", "jobObjectPath");

    quint32 ret = info->getJobId();

    EXPECT_EQ(testInt, ret);

    delete info;
}

TEST(UT_UnitInfo_setJobId, UT_UnitInfo_setJobId_001)
{
    const quint32 testInt = 456;
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", "unitObjectPath", 123, "jobType", "jobObjectPath");

    info->setJobId(testInt);
    quint32 ret = info->getJobId();

    EXPECT_EQ(testInt, ret);

    delete info;
}

TEST(UT_UnitInfo_getJobType, UT_UnitInfo_getJobType_001)
{
    const QString testStr("jobTypeTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", "unitObjectPath", 123, testStr, "jobObjectPath");

    QString ret = info->getJobType();

    EXPECT_EQ(testStr, ret);

    delete info;
}

TEST(UT_UnitInfo_setJobType, UT_UnitInfo_setJobType_001)
{
    const QString testStr("jobTypeTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", "unitObjectPath", 123, "jobType", "jobObjectPath");

    info->setJobType(testStr);
    QString ret = info->getJobType();

    EXPECT_EQ(testStr, ret);

    delete info;
}

TEST(UT_UnitInfo_getJobObjectPath, UT_UnitInfo_getJobObjectPath_001)
{
    const QString testStr("jobObjectPathTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", "unitObjectPath", 123, "jobType", testStr);

    QString ret = info->getJobObjectPath();

    EXPECT_EQ(testStr, ret);

    delete info;
}

TEST(UT_UnitInfo_setJobObjectPath, UT_UnitInfo_setJobObjectPath_001)
{
    const QString testStr("jobObjectPathTest");
    UnitInfo* info = new UnitInfo("name", "description", "loadState", "activeState",
                                  "subState", "followedBy", "unitObjectPath", 123, "jobType", "jobObjectPath");

    info->setJobObjectPath(testStr);
    QString ret = info->getJobObjectPath();

    EXPECT_EQ(testStr, ret);

    delete info;
}
