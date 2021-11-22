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
#include "base/base_table_view.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"
#include "addr_any.h"

//Qt
#include <QSignalSpy>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QTableView>
#include <QSortFilterProxyModel>
#include <DApplication>
#include <DStyle>

/***************************************STUB begin*********************************************/
bool stub_BaseTableView_currentChanged_isValid()
{
    return true;
}

DStyle* stub_drawRow_style()
{
    static DStyle style;
    return  &style;
}
/***************************************STUB end**********************************************/


class UT_BaseTableView : public ::testing::Test
{
public:
    UT_BaseTableView() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget obj;
        m_tester = new BaseTableView(&obj);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    BaseTableView *m_tester;
};

TEST_F(UT_BaseTableView, initTest)
{

}

TEST_F(UT_BaseTableView, test_setModel_01)
{
    QSortFilterProxyModel model;
    m_tester->setModel(&model);
}

TEST_F(UT_BaseTableView, test_paintEvent_01)
{
    EXPECT_TRUE(!m_tester->grab().isNull());
}

TEST_F(UT_BaseTableView, test_drawRow_01)
{
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    static QStyleOptionViewItem view;
    static QWidget widget;
    view.widget = &widget;
    static QModelIndex index;
    AddrAny any;
    std::map<std::string,void*> result;
    any.get_local_func_addr_symtab("^style()$", result);

    DApplication::style();
    Stub stub;
    stub.set(ADDR(DApplication, style), stub_drawRow_style);
    m_tester->drawRow(&painter, view, index);
}

TEST_F(UT_BaseTableView, test_focusInEvent_01)
{
    static QFocusEvent ev(QEvent::FocusIn, Qt::OtherFocusReason);
    m_tester->focusInEvent(&ev);

    EXPECT_EQ(m_tester->m_focusReason, Qt::OtherFocusReason);
}


TEST_F(UT_BaseTableView, test_currentChanged_01)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QModelIndex current;
    static QModelIndex prev;
    m_tester->currentChanged(current, prev);
}

TEST_F(UT_BaseTableView, test_viewportEvent_01)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QHoverEvent ev(QEvent::HoverLeave, QPointF(0.0, 0.0), QPointF(10.0, 10.0), Qt::NoModifier);
    m_tester->viewportEvent(&ev);
}

TEST_F(UT_BaseTableView, test_viewportEvent_02)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QHoverEvent ev(QEvent::HoverEnter, QPointF(0.0, 0.0), QPointF(10.0, 10.0), Qt::NoModifier);
    m_tester->viewportEvent(&ev);
}

TEST_F(UT_BaseTableView, test_viewportEvent_03)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QHoverEvent ev(QEvent::HoverMove, QPointF(0.0, 0.0), QPointF(10.0, 10.0), Qt::NoModifier);
    m_tester->viewportEvent(&ev);
}

TEST_F(UT_BaseTableView, test_viewportEvent_04)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QHoverEvent ev(QEvent::HoverMove, QPointF(0.0, 0.0), QPointF(10.0, 10.0), Qt::NoModifier);
    m_tester->viewportEvent(&ev);
}

TEST_F(UT_BaseTableView, test_viewportEvent_05)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QHoverEvent ev(QEvent::HoverMove, QPointF(0.0, 0.0), QPointF(10.0, 10.0), Qt::NoModifier);
    m_tester->viewportEvent(&ev);
}

TEST_F(UT_BaseTableView, test_viewportEvent_06)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QMouseEvent ev(QEvent::MouseMove, QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->viewportEvent(&ev);
}

TEST_F(UT_BaseTableView, test_viewportEvent_07)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QMouseEvent ev(QEvent::MouseButtonRelease, QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->viewportEvent(&ev);
}

TEST_F(UT_BaseTableView, test_viewportEvent_08)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QMouseEvent ev(QEvent::MouseButtonDblClick, QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->viewportEvent(&ev);
}

TEST_F(UT_BaseTableView, test_viewportEvent_09)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QMouseEvent ev(QEvent::MouseButtonPress, QPointF(0.0, 0.0), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->viewportEvent(&ev);
}

TEST_F(UT_BaseTableView, test_viewportEvent_10)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QTouchEvent ev(QEvent::TouchEnd);
    m_tester->viewportEvent(&ev);
}

TEST_F(UT_BaseTableView, test_scrollTo_01)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseTableView_currentChanged_isValid);
    static QModelIndex index;
    m_tester->scrollTo(index);
}

















