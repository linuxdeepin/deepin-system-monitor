// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
#include <QStyleOptionViewItem>
#include <QIcon>
#include <QHelpEvent>
#include <DStyle>
DWIDGET_USE_NAMESPACE;
/***************************************STUB begin*********************************************/

bool stub_BaseItemDelegate_paint_isVaild()
{
    return true;
}

int stub_helpEvent_column()
{
    return 0;
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
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
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
    option.state = DStyle::State_Enabled | DStyle::State_Selected | DStyle::State_Sunken;

    static QModelIndex index;

    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_paint_03)
{
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    static QStyleOptionViewItem option;
    option.state = DStyle::State_Enabled | DStyle::State_Selected | DStyle::State_MouseOver;

    static QModelIndex index;

    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_paint_04)
{
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    static QStyleOptionViewItem option;
    option.state = DStyle::State_Enabled | DStyle::State_MouseOver;

    static QModelIndex index;

    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_paint_05)
{
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    static QStyleOptionViewItem option;
    option.state = DStyle::State_Enabled | DStyle::State_MouseOver;
    option.viewItemPosition = QStyleOptionViewItem::Beginning;

    static QModelIndex index;

    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_paint_06)
{
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    static QStyleOptionViewItem option;
    option.state = DStyle::State_Enabled | DStyle::State_MouseOver;
    option.viewItemPosition = QStyleOptionViewItem::Middle;

    static QModelIndex index;

    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_paint_07)
{
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    static QStyleOptionViewItem option;
    option.state = DStyle::State_Enabled | DStyle::State_MouseOver;
    option.viewItemPosition = QStyleOptionViewItem::End;

    static QModelIndex index;

    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_paint_08)
{
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    static QStyleOptionViewItem option;
    option.state = DStyle::State_Enabled | DStyle::State_MouseOver;
    option.viewItemPosition = QStyleOptionViewItem::OnlyOne;

    static QModelIndex index;

    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_paint_09)
{
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    static QStyleOptionViewItem option;
    option.state = DStyle::State_Enabled | DStyle::State_MouseOver;
    option.viewItemPosition = QStyleOptionViewItem::OnlyOne;
    option.features = QStyleOptionViewItem::HasDecoration;

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

TEST_F(UT_BaseItemDelegate, test_helpEvent_02)
{
    static QHelpEvent he(QEvent::ToolTip, QPoint(0, 0), QPoint(0, 0));
    static QWidget parent;
    static QTableView view(&parent);
    static QStyleOptionViewItem option;
    option.features =  QStyleOptionViewItem::HasDecoration;
    Stub stub;
    stub.set(ADDR(QModelIndex, column), stub_helpEvent_column);
    static QModelIndex index;
    m_tester->helpEvent(&he, &view, option, index);
}

TEST_F(UT_BaseItemDelegate, test_initStyleOption_01)
{
    static QStyleOptionViewItem option;
    static QModelIndex index;
    m_tester->initStyleOption(&option, index);
}

// ========== 新增：覆盖 paint 的 Disabled 状态、decoration 绘制、default 分支等 ==========

TEST_F(UT_BaseItemDelegate, test_paint_disabledState)
{
    // state 不含 State_Enabled → 命中 cg = DPalette::Disabled 分支
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    QStyleOptionViewItem option;
    option.state = QStyle::State_None;  // 未启用
    option.viewItemPosition = QStyleOptionViewItem::OnlyOne;
    QModelIndex index;
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_paint_defaultViewItemPosition)
{
    // viewItemPosition 为默认(Invalid) → switch default 分支，调用父类 paint 后 return
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    QStyleOptionViewItem option;
    option.state = DStyle::State_Enabled;
    QModelIndex index;
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_paint_beginningWithDecoration)
{
    // Beginning + HasDecoration → 走 icon 绘制分支
    static QPixmap pix(200, 100);
    static QPainter painter(&pix);
    QStyleOptionViewItem option;
    option.state = DStyle::State_Enabled;
    option.viewItemPosition = QStyleOptionViewItem::Beginning;
    option.features = QStyleOptionViewItem::HasDecoration | QStyleOptionViewItem::HasDisplay;
    option.icon = QIcon();
    option.text = "test";
    QModelIndex index;
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_paint_onlyOneNoDecoration)
{
    // OnlyOne 且无 decoration → 走 text-only 布局
    static QPixmap pix(200, 100);
    static QPainter painter(&pix);
    QStyleOptionViewItem option;
    option.state = DStyle::State_Enabled;
    option.viewItemPosition = QStyleOptionViewItem::OnlyOne;
    option.features = QStyleOptionViewItem::HasDisplay;
    option.text = "hello";
    QModelIndex index;
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_BaseItemDelegate_paint_isVaild);
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_paint_invalidIndexCallsSuper)
{
    // 无效 index → 直接调用父类 paint 返回
    static QPixmap pix(100, 100);
    static QPainter painter(&pix);
    QStyleOptionViewItem option;
    QModelIndex index;  // 默认无效
    m_tester->paint(&painter, option, index);
}

TEST_F(UT_BaseItemDelegate, test_helpEvent_nullEvent)
{
    // e 或 view 为空 → 返回 false
    static QStyleOptionViewItem option;
    static QModelIndex index;
    EXPECT_FALSE(m_tester->helpEvent(nullptr, nullptr, option, index));
}

TEST_F(UT_BaseItemDelegate, test_helpEvent_nonToolTipEvent)
{
    // 非 ToolTip 事件 → 调用父类 helpEvent
    static QHelpEvent he(QEvent::MouseMove, QPoint(0, 0), QPoint(0, 0));
    static QWidget parent;
    static QTableView view(&parent);
    static QStyleOptionViewItem option;
    static QModelIndex index;
    m_tester->helpEvent(&he, &view, option, index);
}

TEST_F(UT_BaseItemDelegate, test_sizeHint_minHeight)
{
    // sizeHint 确保 max(36, h) 分支
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 100, 20);  // 小高度
    QModelIndex index;
    QSize s = m_tester->sizeHint(option, index);
    EXPECT_GE(s.height(), 36);
}

