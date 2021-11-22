/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:     xuezifan<xuezifan@uniontech.com>
*
* Maintainer: xuezifan<xuezifan@uniontech.com>
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

//self
#include "dbus/environment_file.h"

//qt
#include <QList>
#include <QString>
#include <QtDBus>
#include <QDebug>

//gtest
#include "stub.h"
#include <gtest/gtest.h>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_EnvironmentFile : public ::testing::Test
{
public:
    UT_EnvironmentFile() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new EnvironmentFile();
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    EnvironmentFile *m_tester;
};

TEST_F(UT_EnvironmentFile, initTest)
{

}


TEST_F(UT_EnvironmentFile, test_Operator_01)
{
    EnvironmentFile aFile;
    aFile.envFile = "a";
    aFile.flag = 0;
    m_tester->envFile = "a";

    EnvironmentFile bFile;
    bFile.envFile = "a";
    bFile.flag = 0;

    EXPECT_EQ(aFile == bFile, true);
}

TEST_F(UT_EnvironmentFile, test_Operator_02)
{
    EnvironmentFile aFile;
    aFile.envFile = "a";
    aFile.flag = 0;

    EnvironmentFile bFile;
    bFile.envFile = "b";
    bFile.flag = 0;

    EXPECT_EQ(aFile == bFile, false);
}

TEST_F(UT_EnvironmentFile, test_Operator_03)
{
    EnvironmentFile aFile;
    aFile.envFile = "a";
    aFile.flag = 0;

    EnvironmentFile bFile;
    bFile.envFile = "a";
    bFile.flag = 1;

    EXPECT_EQ(aFile == bFile, false);
}

TEST_F(UT_EnvironmentFile, test_Operator_04)
{
    EnvironmentFile aFile;
    aFile.envFile = "a";
    aFile.flag = 0;

    EnvironmentFile bFile;
    bFile.envFile = "b";
    bFile.flag = 1;

    EXPECT_EQ(aFile == bFile, false);
}
