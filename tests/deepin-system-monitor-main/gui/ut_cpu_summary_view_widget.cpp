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
#include "cpu_summary_view_widget.h"
#include "model/cpu_info_model.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_CPUDetailSummaryTable : public ::testing::Test
{
public:
    UT_CPUDetailSummaryTable() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static CPUInfoModel model;
        m_tester = new CPUDetailSummaryTable(&model, nullptr);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CPUDetailSummaryTable *m_tester;
};

TEST_F(UT_CPUDetailSummaryTable, initTest)
{

}

TEST_F(UT_CPUDetailSummaryTable, test_fontChanged_01)
{
    QFont font;
    font.setBold(true);
    m_tester->fontChanged(font);

    EXPECT_EQ(m_tester->m_font.bold(), font.bold());
}

TEST_F(UT_CPUDetailSummaryTable, test_onModelUpdate_01)
{
    m_tester->onModelUpdate();
}

TEST_F(UT_CPUDetailSummaryTable, test_paintEvent_01)
{
    EXPECT_FALSE(m_tester->grab().isNull());
}
