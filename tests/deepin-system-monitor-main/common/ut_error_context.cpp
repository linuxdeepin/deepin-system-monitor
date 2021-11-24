/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//self
#include "common/error_context.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_ErrorContext : public ::testing::Test
{
public:
    UT_ErrorContext() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new ErrorContext();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    ErrorContext *m_tester;
};

TEST_F(UT_ErrorContext, initTest)
{

}

TEST_F(UT_ErrorContext, test_ErrorContext_01)
{
    ErrorContext();
}

TEST_F(UT_ErrorContext, test_getCode_01)
{
    m_tester->getCode();
}

TEST_F(UT_ErrorContext, test_setCode_01)
{
    m_tester->setCode(1);
}

TEST_F(UT_ErrorContext, test_getSubCode_01)
{
    m_tester->getSubCode();
}

TEST_F(UT_ErrorContext, test_setSubCode_01)
{
    m_tester->setSubCode(1);
}

TEST_F(UT_ErrorContext, test_getErrorName_01)
{
    m_tester->getErrorName();
}

TEST_F(UT_ErrorContext, test_setErrorName_01)
{
    m_tester->setErrorName("1");
}

TEST_F(UT_ErrorContext, test_getErrorMessage_01)
{
    m_tester->getErrorMessage();
}

TEST_F(UT_ErrorContext, test_setErrorMessage_01)
{
    m_tester->setErrorMessage("1");
}

TEST_F(UT_ErrorContext, test_operator_01)
{
    const ErrorContext other;
    m_tester->operator==(other);
}

