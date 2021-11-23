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
#include "mem_stat_view_widget.h"
#include "system/mem.h"
#include "chart_view_widget.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QResizeEvent>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_MemStatViewWidget : public ::testing::Test
{
public:
    UT_MemStatViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget parent;
        m_tester = new MemStatViewWidget(&parent);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    MemStatViewWidget *m_tester;
};

TEST_F(UT_MemStatViewWidget, initTest)
{

}

TEST_F(UT_MemStatViewWidget, test_paintEvent_01)
{
    EXPECT_TRUE(!m_tester->grab().isNull());
}

TEST_F(UT_MemStatViewWidget, test_paintEvent_02)
{
    m_tester->m_memInfo->readMemInfo();
    EXPECT_TRUE(!m_tester->grab().isNull());
}

TEST_F(UT_MemStatViewWidget, test_resizeEvent_01)
{
    QResizeEvent event(QSize(10, 10), QSize(20, 20));
    m_tester->resizeEvent(&event);
}

TEST_F(UT_MemStatViewWidget, test_fontChanged_01)
{
    QFont font;
    font.setItalic(true);
    m_tester->fontChanged(font);

    EXPECT_TRUE(m_tester->m_font.italic());
}

TEST_F(UT_MemStatViewWidget, test_onModelUpdate_01)
{
    m_tester->onModelUpdate();
}

TEST_F(UT_MemStatViewWidget, test_updateWidgetGeometry_01)
{
    m_tester->updateWidgetGeometry();
//    EXPECT_FALSE(m_tester->m_swapChartWidget->isHidden());
}

