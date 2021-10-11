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

#include "dbus/environment_file.h"

#include <QList>
#include <QString>
#include <QtDBus>
#include <QDebug>

#include "ut_environment_file.h"

Ut_EnvironmentFile::Ut_EnvironmentFile()
{
}

TEST(UT_EnvironmentFile_OperationEquel, UT_EnvironmentFile_OperationEquel_001)
{
    EnvironmentFile aFile;
    aFile.envFile = "a";
    aFile.flag = 0;

    EnvironmentFile bFile;
    bFile.envFile = "a";
    bFile.flag = 0;

    EXPECT_EQ(aFile == bFile, true);
}

TEST(UT_EnvironmentFile_OperationEquel, UT_EnvironmentFile_OperationEquel_002)
{
    EnvironmentFile aFile;
    aFile.envFile = "a";
    aFile.flag = 0;

    EnvironmentFile bFile;
    bFile.envFile = "b";
    bFile.flag = 0;

    EXPECT_EQ(aFile == bFile, false);
}

TEST(UT_EnvironmentFile_OperationEquel, UT_EnvironmentFile_OperationEquel_003)
{
    EnvironmentFile aFile;
    aFile.envFile = "a";
    aFile.flag = 0;

    EnvironmentFile bFile;
    bFile.envFile = "a";
    bFile.flag = 1;

    EXPECT_EQ(aFile == bFile, false);
}

TEST(UT_EnvironmentFile_OperationEquel, UT_EnvironmentFile_OperationEquel_004)
{
    EnvironmentFile aFile;
    aFile.envFile = "a";
    aFile.flag = 0;

    EnvironmentFile bFile;
    bFile.envFile = "b";
    bFile.flag = 1;

    EXPECT_EQ(aFile == bFile, false);
}
