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
#include "dialog/custombuttonbox.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QFocusEvent>
#include <QPushButton>

/***************************************STUB begin*********************************************/

QPushButton* stub_focusInEvent_button()
{
    static QPushButton btn;
    return &btn;
}

/***************************************STUB end**********************************************/


class UT_CustomButtonBox : public ::testing::Test
{
public:
    UT_CustomButtonBox() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget wid;
        m_tester = new CustomButtonBox(&wid);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CustomButtonBox *m_tester;
};

TEST_F(UT_CustomButtonBox, initTest)
{

}


TEST_F(UT_CustomButtonBox, test_focusInEvent_01)
{
    Stub stub;
    stub.set(ADDR(DButtonBox, button), stub_focusInEvent_button);
    static QFocusEvent ev(QEvent::FocusIn);

    m_tester->focusInEvent(&ev);
}
