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
#include "block_dev_summary_view_widget.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QDebug>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_BlockDevSummaryViewWidget : public ::testing::Test
{
public:
    UT_BlockDevSummaryViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new BlockDevSummaryViewWidget();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    BlockDevSummaryViewWidget *m_tester;
};

TEST_F(UT_BlockDevSummaryViewWidget, initTest)
{

}


TEST_F(UT_BlockDevSummaryViewWidget, test_fontChanged_01)
{
    QFont font;
    font.setBold(true);
    m_tester->fontChanged(font);

    EXPECT_EQ(m_tester->m_font.bold(), font.bold());
    EXPECT_EQ(m_tester->height(), 260);
}

TEST_F(UT_BlockDevSummaryViewWidget, test_chageSummaryInfo_01)
{
    QString deviceName = "";
    m_tester->chageSummaryInfo(deviceName);
}

TEST_F(UT_BlockDevSummaryViewWidget, test_paintEvent_01)
{
    EXPECT_FALSE(m_tester->grab().isNull());
}

TEST_F(UT_BlockDevSummaryViewWidget, test_rowCount_01)
{
    EXPECT_EQ(m_tester->model()->rowCount(), 7);
}

TEST_F(UT_BlockDevSummaryViewWidget, test_columnCount_01)
{
    EXPECT_EQ(m_tester->model()->columnCount(), 2);
}

TEST_F(UT_BlockDevSummaryViewWidget, test_data_01)
{
    QModelIndex index =  m_tester->model()->index(0, 0);
    m_tester->model()->data(index, Qt::DisplayRole);
}

TEST_F(UT_BlockDevSummaryViewWidget, test_flags_01)
{
    QModelIndex index =  m_tester->model()->index(0, 0);
    EXPECT_EQ(m_tester->model()->flags(index), Qt::NoItemFlags);
}








