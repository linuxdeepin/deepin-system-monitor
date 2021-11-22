/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan  <yukuan@uniontech.com>
*
* Maintainer: yukuan  <yukuan@uniontech.com>
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

//Self
#include "service_name_sub_input_dialog.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QShowEvent>
#include <QMessageBox>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_ServiceNameSubInputDialog : public ::testing::Test
{
public:
    UT_ServiceNameSubInputDialog() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget wid;
        m_tester = new ServiceNameSubInputDialog(&wid);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    ServiceNameSubInputDialog *m_tester;
};

TEST_F(UT_ServiceNameSubInputDialog, initTest)
{

}

TEST_F(UT_ServiceNameSubInputDialog, test_getServiceSubName_01)
{
    m_tester->m_name = "1";
    m_tester->getServiceSubName();

    EXPECT_EQ(m_tester->m_name, "1");
}

TEST_F(UT_ServiceNameSubInputDialog, test_setServiceSubName_01)
{
    m_tester->setServiceSubName("1");

    EXPECT_EQ(m_tester->m_name, "1");
}

TEST_F(UT_ServiceNameSubInputDialog, test_result_01)
{
    int res = m_tester->result();

    EXPECT_EQ(res, m_tester->m_result);
}


TEST_F(UT_ServiceNameSubInputDialog, test_setResult_01)
{
    m_tester->setResult(1);

    EXPECT_EQ(m_tester->m_result, 1);
}

TEST_F(UT_ServiceNameSubInputDialog, test_onButtonClicked_01)
{
    m_tester->onButtonClicked(0, "1");

    EXPECT_EQ(m_tester->m_result, QMessageBox::Ok);
}


TEST_F(UT_ServiceNameSubInputDialog, test_onButtonClicked_02)
{
    m_tester->onButtonClicked(1, "1");

    EXPECT_EQ(m_tester->m_result, QMessageBox::Cancel);
}

TEST_F(UT_ServiceNameSubInputDialog, test_showEvent_01)
{
    static QShowEvent ev;
    m_tester->showEvent(&ev);
}
