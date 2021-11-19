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
#include "base/base_header_view.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QResizeEvent>
#include <QKeyEvent>


/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/


class UT_BaseHeaderView : public ::testing::Test
{
public:
    UT_BaseHeaderView() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget obj;
        m_tester = new BaseHeaderView(Qt::Horizontal, &obj);
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    BaseHeaderView *m_tester;
};

TEST_F(UT_BaseHeaderView, initTest)
{

}

TEST_F(UT_BaseHeaderView, test_sizeHint_01)
{
    EXPECT_EQ(m_tester->sizeHint().height(), 37);
}

TEST_F(UT_BaseHeaderView, test_paintEvent_01)
{
    EXPECT_TRUE(!m_tester->grab().isNull());
}

TEST_F(UT_BaseHeaderView, test_eventFilter_01)
{
    static QKeyEvent ev(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);

    EXPECT_TRUE(m_tester->eventFilter(m_tester, &ev));
}

TEST_F(UT_BaseHeaderView, test_eventFilter_02)
{
    static QKeyEvent ev(QEvent::KeyPress, Qt::Key_M, Qt::NoModifier);

    EXPECT_FALSE(m_tester->eventFilter(m_tester, &ev));
}

TEST_F(UT_BaseHeaderView, test_focusInEvent_01)
{
    static QFocusEvent ev(QEvent::FocusIn, Qt::OtherFocusReason);
    m_tester->focusInEvent(&ev);

    EXPECT_EQ(m_tester->m_focusReason, Qt::OtherFocusReason);
}

