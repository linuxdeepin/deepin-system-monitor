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
#include "mem_summary_view_widget.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_MemSummaryViewWidget : public ::testing::Test
{
public:
    UT_MemSummaryViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget parent;
        m_tester = new MemSummaryViewWidget(&parent);
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    MemSummaryViewWidget *m_tester;
};

TEST_F(UT_MemSummaryViewWidget, initTest)
{

}

TEST_F(UT_MemSummaryViewWidget, test_fontChanged_01)
{
    QFont font;
    font.setItalic(true);
    m_tester->fontChanged(font);

    EXPECT_TRUE(m_tester->m_font.italic());
}

TEST_F(UT_MemSummaryViewWidget, test_onModelUpdate_01)
{
    m_tester->onModelUpdate();
}

TEST_F(UT_MemSummaryViewWidget, test_paintEvent_01)
{
    EXPECT_TRUE(!m_tester->grab().isNull());
}

