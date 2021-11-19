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
#include "xwin_kill_preview_background_widget.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <DStyle>


/***************************************STUB begin*********************************************/
Dtk::Widget::DStyle* stub_paintEvent_dstyle()
{
    static Dtk::Widget::DStyle style;
    return  &style;
}
/***************************************STUB end**********************************************/

class UT_XWinKillPreviewBackgroundWidget : public ::testing::Test
{
public:
    UT_XWinKillPreviewBackgroundWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget wid;
        static QPixmap pixmap;
        m_tester = new XWinKillPreviewBackgroundWidget(pixmap, &wid);
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    XWinKillPreviewBackgroundWidget *m_tester;
};

TEST_F(UT_XWinKillPreviewBackgroundWidget, initTest)
{

}

TEST_F(UT_XWinKillPreviewBackgroundWidget, test_updateSelection_01)
{
    m_tester->updateSelection(QRegion());

    EXPECT_EQ(m_tester->m_selRegion, QRegion());
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, test_clearSelection_01)
{
    m_tester->clearSelection();

    EXPECT_EQ(m_tester->m_selRegion, QRegion());
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, test_paintEvent_01)
{
    Stub stub;
    stub.set(ADDR(QWidget, style), stub_paintEvent_dstyle);
    EXPECT_TRUE(!m_tester->grab().isNull());
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, test_initUI_01)
{
    m_tester->initUI();
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, test_initConnection_01)
{
    m_tester->initConnection();
}
