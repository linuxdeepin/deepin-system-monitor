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
#include "monitor_expand_view.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_MonitorExpandView : public ::testing::Test
{
public:
    UT_MonitorExpandView() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget parent;
        m_tester = new MonitorExpandView(&parent);
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    MonitorExpandView *m_tester;
};

TEST_F(UT_MonitorExpandView, initTest)
{

}

TEST_F(UT_MonitorExpandView, test_setDetailButtonVisible_01)
{
    m_tester->setDetailButtonVisible(true);
}
