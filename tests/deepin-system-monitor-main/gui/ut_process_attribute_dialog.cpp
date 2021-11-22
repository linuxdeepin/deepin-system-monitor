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
#include "process_attribute_dialog.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QResizeEvent>
#include <QCloseEvent>


/***************************************STUB begin*********************************************/


/***************************************STUB end**********************************************/

class UT_ProcessAttributeDialog : public ::testing::Test
{
public:
    UT_ProcessAttributeDialog() : m_tester1(nullptr), m_tester2(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget wid;
        QIcon icon;
        m_tester1 = new ProcessAttributeDialog(1, "1", "1", "1", icon, 1, &wid);
        m_tester2 = new ProcessAttributeDialog(1, "1", "1", "", icon, 1, &wid);
    }

    virtual void TearDown()
    {
        if (m_tester1) {
            delete m_tester1;
            m_tester1 = nullptr;
        }
        if (m_tester2) {
            delete m_tester2;
            m_tester2 = nullptr;
        }
    }

protected:
    ProcessAttributeDialog *m_tester1;
    ProcessAttributeDialog *m_tester2;
};

TEST_F(UT_ProcessAttributeDialog, initTest)
{

}

TEST_F(UT_ProcessAttributeDialog, test_initUI_01)
{

    m_tester1->initUI();

    EXPECT_EQ(m_tester1->m_procCmdLabel->isHidden(), false);
}

TEST_F(UT_ProcessAttributeDialog, test_initUI_02)
{
    m_tester2->initUI();
    EXPECT_EQ(m_tester2->m_procCmdLabel->isHidden(), true);
}

TEST_F(UT_ProcessAttributeDialog, test_resizeItemWidget_01)
{
    m_tester2->resizeItemWidget();
}

TEST_F(UT_ProcessAttributeDialog, test_resizeEvent_01)
{
    QResizeEvent ev(QSize(10, 10), QSize(20, 20));
    m_tester1->resizeEvent(&ev);

    EXPECT_NE(m_tester1->m_frame, nullptr);
}

TEST_F(UT_ProcessAttributeDialog, test_closeEvent_01)
{
    QCloseEvent ev;
    m_tester1->closeEvent(&ev);
}

TEST_F(UT_ProcessAttributeDialog, test_eventFitler_01)
{
    static QKeyEvent ev(QKeyEvent::KeyPress, Qt::Key_Cancel, Qt::NoModifier);
    m_tester1->eventFilter(m_tester1, &ev);
}

TEST_F(UT_ProcessAttributeDialog, test_eventFitler_02)
{
    static QEvent ev(QEvent::Show);
    m_tester1->eventFilter(m_tester1, &ev);
}

TEST_F(UT_ProcessAttributeDialog, test_eventFitler_03)
{
    static QEvent ev(QEvent::FontChange);
    m_tester1->eventFilter(m_tester1, &ev);
}

TEST_F(UT_ProcessAttributeDialog, test_eventFitler_04)
{
    static QMouseEvent ev(QMouseEvent::MouseButtonPress, QPointF(0, 0), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester1->eventFilter(m_tester1, &ev);
}










