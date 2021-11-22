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
#include "dialog/error_dialog.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QFocusEvent>
#include <QPushButton>

/***************************************STUB begin*********************************************/
void stub_show_exec(void* object)
{
    return;
}

/***************************************STUB end**********************************************/


class UT_ErrorDialog : public ::testing::Test
{
public:
    UT_ErrorDialog() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget wid;
        m_tester = new ErrorDialog("1", "1", &wid);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    ErrorDialog *m_tester;
};

TEST_F(UT_ErrorDialog, initTest)
{

}

TEST_F(UT_ErrorDialog, test_show_01)
{
    typedef int (*fptr)(DDialog*);
    fptr DDialog_exec = (fptr)(&DDialog::exec);   //obtaining an address
    Stub stub;
    stub.set(DDialog_exec, stub_show_exec);
    static QWidget wid;
    m_tester->show(&wid, "1", "1");
}

TEST_F(UT_ErrorDialog, test_initUI_01)
{
    m_tester->initUI();
}
