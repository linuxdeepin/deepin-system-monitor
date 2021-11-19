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
#include "base/base_detail_view_widget.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QResizeEvent>


/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/


class UT_BaseDetailViewWidget : public ::testing::Test
{
public:
    UT_BaseDetailViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget obj;
        m_tester = new BaseDetailViewWidget(&obj);
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    BaseDetailViewWidget *m_tester;
};

TEST_F(UT_BaseDetailViewWidget, initTest)
{

}

TEST_F(UT_BaseDetailViewWidget, test_setTitle_01)
{
    m_tester->setTitle("1");

    EXPECT_EQ(m_tester->m_titleText, "1");
}

TEST_F(UT_BaseDetailViewWidget, test_title_01)
{
    m_tester->m_titleText = "1";

    EXPECT_EQ(m_tester->title(), "1");
}

TEST_F(UT_BaseDetailViewWidget, test_setDetail_01)
{
    m_tester->setDetail("1");

    EXPECT_EQ(m_tester->m_detailText, "1");
}

TEST_F(UT_BaseDetailViewWidget, test_detail_01)
{
    m_tester->m_detailText = "1";

    EXPECT_EQ(m_tester->detail(), "1");
}

TEST_F(UT_BaseDetailViewWidget, test_titleHeight_01)
{
    m_tester->titleHeight();
}

TEST_F(UT_BaseDetailViewWidget, test_detailFontChanged_01)
{
    QFont font;
    font.setBold(true);
    m_tester->detailFontChanged(font);

    EXPECT_EQ(m_tester->m_titleFont.bold(), font.bold());
}

TEST_F(UT_BaseDetailViewWidget, test_updateWidgetGrometry_01)
{
    m_tester->updateWidgetGrometry();

    EXPECT_NE(m_tester->m_arrowButton->width(), 0);
}

TEST_F(UT_BaseDetailViewWidget, test_onThemeTypeChanged_01)
{
    m_tester->onThemeTypeChanged(DGuiApplicationHelper::LightType);
}

TEST_F(UT_BaseDetailViewWidget, test_onThemeTypeChanged_02)
{
    m_tester->onThemeTypeChanged(DGuiApplicationHelper::DarkType);
}

TEST_F(UT_BaseDetailViewWidget, test_resizeEvent_01)
{
    static QResizeEvent ev(QSize(0, 0), QSize(20, 20));
    m_tester->resizeEvent(&ev);
}

TEST_F(UT_BaseDetailViewWidget, test_paintEvent_01)
{
    EXPECT_TRUE(!m_tester->grab().isNull());
}

TEST_F(UT_BaseDetailViewWidget, test_mouseMoveEvent_01)
{
    static QMouseEvent ev(QEvent::MouseMove, QPoint(0, 0), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->mouseMoveEvent(&ev);
}






