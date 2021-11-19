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
#include "base/base_commandlink_button.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QKeyEvent>
#include <QMouseEvent>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/


class UT_BaseCommandLinkButton : public ::testing::Test
{
public:
    UT_BaseCommandLinkButton() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget wid;
        m_tester = new BaseCommandLinkButton("1", &wid);
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    BaseCommandLinkButton *m_tester;
};

TEST_F(UT_BaseCommandLinkButton, initTest)
{

}

TEST_F(UT_BaseCommandLinkButton, test_eventFilter_01)
{
    static QKeyEvent ev(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);


    EXPECT_TRUE(m_tester->eventFilter(m_tester, &ev));
}

TEST_F(UT_BaseCommandLinkButton, test_eventFilter_02)
{
    static QMouseEvent ev(QEvent::MouseButtonPress, QPointF(0, 0), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);


    EXPECT_FALSE(m_tester->eventFilter(m_tester, &ev));
}
