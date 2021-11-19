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
#include "base/base_item_delegate.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QTableView>

/***************************************STUB begin*********************************************/

bool stub_BaseItemDelegate_paint_isVaild()
{
    return true;
}
/***************************************STUB end**********************************************/


class UT_BaseItemDelegate : public ::testing::Test
{
public:
    UT_BaseItemDelegate() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget obj;
        m_tester = new BaseItemDelegate(&obj);
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    BaseItemDelegate *m_tester;
};

TEST_F(UT_BaseItemDelegate, initTest)
{

}

TEST_F(UT_BaseItemDelegate, test_paint_01)
{
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    static QStyleOptionViewItem option;
    static QModelIndex index;
    m_tester->paint(&painter, option, index);
}


TEST_F(UT_BaseItemDelegate, test_paint_02)
{
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    static QStyleOptionViewItem option;
    static QModelIndex index;

    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_createEditor_01)
{
    static QWidget wid;
    static QStyleOptionViewItem option;
    static QModelIndex index;
    EXPECT_EQ(m_tester->createEditor(&wid, option, index), nullptr);
}

TEST_F(UT_BaseItemDelegate, test_sizeHint_01)
{
    static QStyleOptionViewItem option;
    static QModelIndex index;
    m_tester->sizeHint(option, index);
}

TEST_F(UT_BaseItemDelegate, test_helpEvent_01)
{
    static QHelpEvent he(QEvent::ToolTip, QPoint(0, 0), QPoint(0, 0));
    static QWidget parent;
    static QTableView view(&parent);
    static QStyleOptionViewItem option;
    static QModelIndex index;
    m_tester->helpEvent(&he, &view, option, index);
}

TEST_F(UT_BaseItemDelegate, test_initStyleOption_01)
{
    static QStyleOptionViewItem option;
    static QModelIndex index;
    m_tester->initStyleOption(&option, index);
}
