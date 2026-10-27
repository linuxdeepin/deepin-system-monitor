// Copyright (C) 2026 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "user_list_view.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QKeyEvent>
#include <QMouseEvent>

// 分支清单（来源：get_code_snippet UserListView::keyPressEvent 实源码；共 14 条）
// B1: key == Key_Up                        → 进入 Up 键处理
// B2: Up: nextIndex.isValid() 为真          → 走移动/发送 clicked 路径
// B3: Up: isRowHidden(m_currentIndex-1)    → 跳过隐藏行的递归路径
// B4: Up: m_currentIndex-1 <= 0            → 顶部隐藏行边界，停止
// B5: Up: 边界提前 return                  → 不移动不发送信号
// B6: Up: 递归后提前 return                → 通过递归完成移动
// B7: Up: 处理完后 return                  → 事件被消费
// B8: key == Key_Down                      → 进入 Down 键处理
// B9: Down: nextIndex.isValid() 为真       → 走移动/发送 clicked 路径
// B10: Down: isRowHidden(m_currentIndex+1) → 跳过隐藏行的递归路径
// B11: Down: rowCount-1 <= m_currentIndex+1 → 底部隐藏行边界，停止
// B12: Down: 边界提前 return               → 不移动不发送信号
// B13: Down: 递归后提前 return             → 通过递归完成移动
// B14: Down: 处理完后 return               → 事件被消费
//
// 用例映射：
// - KeyNav_UpFromRow1_MovesToRow0AndEmitsClicked     → B1 B2 B7
// - KeyNav_UpSkipsHiddenRow_RecursesToVisibleRow     → B1 B2 B3 B6
// - KeyNav_UpAtTopHiddenBoundary_StaysOnCurrentRow   → B1 B2 B3 B4 B5
// - KeyNav_UpWithInvalidNeighbor_KeepsCurrentRow     → B1 B7（B2 为假路径）
// - KeyNav_DownFromRow1_MovesToRow2AndEmitsClicked   → B8 B9 B14
// - KeyNav_DownSkipsHiddenRow_RecursesToVisibleRow   → B8 B9 B10 B13
// - KeyNav_DownAtBottomHiddenBoundary_StaysOnCurrentRow → B8 B9 B10 B11 B12
// - KeyNav_DownWithInvalidNeighbor_KeepsCurrentRow   → B8 B14（B9 为假路径）
// - KeyNav_OtherKey_FallsBackToBaseHandling          → B1/B8 为假路径，走基类

// 用例计数声明表（方法 | level | factors | min | actual）
// | keyPressEvent | high | complexity:8;cognitive:24;recursive | 8 | 9 |
// | mousePressEvent | low | - | 1 | 2 |
// | resetStatus | mid | name_pattern:resetStatus | 2 | 2 |
// | moveCursor | low | - | 1 | 2 |
// | currentChanged | low | - | 1 | 1 |
// | UserListView | low | - | 1 | 1 |

class UT_UserListView : public ::testing::Test
{
public:
    UT_UserListView() : m_tester(nullptr), m_model(nullptr) {}

    void SetUp() override
    {
        m_model = new QStandardItemModel(0, 1, nullptr);
        for (int i = 0; i < 4; ++i) {
            m_model->appendRow(new QStandardItem(QString("user %1").arg(i)));
        }
        m_tester = new UserListView();
        m_tester->setModel(m_model);
    }

    void TearDown() override
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        if (m_model) {
            delete m_model;
            m_model = nullptr;
        }
    }

    // Arrange 辅助：通过公开接口 resetStatus 把当前行定位到 row
    void gotoRow(int row)
    {
        m_tester->resetStatus(m_model->index(row, 0));
    }

protected:
    UserListView *m_tester;
    QStandardItemModel *m_model;
};

// B1 B2 B7：Up 键非隐藏行直接上移并发送 clicked
TEST_F(UT_UserListView, KeyNav_UpFromRow1_MovesToRow0AndEmitsClicked)
{
    // Arrange
    QSignalSpy spyClicked(m_tester, &UserListView::clicked);
    gotoRow(1);
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
    // Act
    m_tester->keyPressEvent(&ev);
    // Assert
    EXPECT_EQ(spyClicked.count(), 1);
    EXPECT_EQ(m_tester->currentIndex().row(), 0);
}

// B1 B2 B3 B6：Up 键遇到隐藏行递归跳过，最终停在最近可见行
TEST_F(UT_UserListView, KeyNav_UpSkipsHiddenRow_RecursesToVisibleRow)
{
    // Arrange
    m_tester->setRowHidden(2, true);
    QSignalSpy spyClicked(m_tester, &UserListView::clicked);
    gotoRow(3);
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
    // Act
    m_tester->keyPressEvent(&ev);
    // Assert
    EXPECT_EQ(spyClicked.count(), 1);
    EXPECT_EQ(m_tester->currentIndex().row(), 1);
}

// B1 B2 B3 B4 B5：Up 键在顶部隐藏行边界处停止，不移动不发送信号
TEST_F(UT_UserListView, KeyNav_UpAtTopHiddenBoundary_StaysOnCurrentRow)
{
    // Arrange
    m_tester->setRowHidden(0, true);
    QSignalSpy spyClicked(m_tester, &UserListView::clicked);
    gotoRow(1);
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
    // Act
    m_tester->keyPressEvent(&ev);
    // Assert
    EXPECT_EQ(spyClicked.count(), 0);
    EXPECT_EQ(m_tester->currentIndex().row(), 1);
}

// B1 B7（B2 为假）：Up 键越出首行（目标索引无效），保持当前行
TEST_F(UT_UserListView, KeyNav_UpWithInvalidNeighbor_KeepsCurrentRow)
{
    // Arrange
    QSignalSpy spyClicked(m_tester, &UserListView::clicked);
    gotoRow(0);
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
    // Act
    m_tester->keyPressEvent(&ev);
    // Assert
    EXPECT_EQ(spyClicked.count(), 0);
    EXPECT_EQ(m_tester->currentIndex().row(), 0);
}

// B8 B9 B14：Down 键非隐藏行直接下移并发送 clicked
TEST_F(UT_UserListView, KeyNav_DownFromRow1_MovesToRow2AndEmitsClicked)
{
    // Arrange
    QSignalSpy spyClicked(m_tester, &UserListView::clicked);
    gotoRow(1);
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    // Act
    m_tester->keyPressEvent(&ev);
    // Assert
    EXPECT_EQ(spyClicked.count(), 1);
    EXPECT_EQ(m_tester->currentIndex().row(), 2);
}

// B8 B9 B10 B13：Down 键遇到隐藏行递归跳过，最终停在最近可见行
TEST_F(UT_UserListView, KeyNav_DownSkipsHiddenRow_RecursesToVisibleRow)
{
    // Arrange
    m_tester->setRowHidden(2, true);
    QSignalSpy spyClicked(m_tester, &UserListView::clicked);
    gotoRow(1);
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    // Act
    m_tester->keyPressEvent(&ev);
    // Assert
    EXPECT_EQ(spyClicked.count(), 1);
    EXPECT_EQ(m_tester->currentIndex().row(), 3);
}

// B8 B9 B10 B11 B12：Down 键在底部隐藏行边界处停止，不移动不发送信号
TEST_F(UT_UserListView, KeyNav_DownAtBottomHiddenBoundary_StaysOnCurrentRow)
{
    // Arrange
    m_tester->setRowHidden(3, true);
    QSignalSpy spyClicked(m_tester, &UserListView::clicked);
    gotoRow(2);
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    // Act
    m_tester->keyPressEvent(&ev);
    // Assert
    EXPECT_EQ(spyClicked.count(), 0);
    EXPECT_EQ(m_tester->currentIndex().row(), 2);
}

// B8 B14（B9 为假）：Down 键越出末行（目标索引无效），保持当前行
TEST_F(UT_UserListView, KeyNav_DownWithInvalidNeighbor_KeepsCurrentRow)
{
    // Arrange
    QSignalSpy spyClicked(m_tester, &UserListView::clicked);
    gotoRow(3);
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    // Act
    m_tester->keyPressEvent(&ev);
    // Assert
    EXPECT_EQ(spyClicked.count(), 0);
    EXPECT_EQ(m_tester->currentIndex().row(), 3);
}

// B1/B8 为假：其他按键交给基类处理，不发送 clicked
TEST_F(UT_UserListView, KeyNav_OtherKey_FallsBackToBaseHandling)
{
    // Arrange
    QSignalSpy spyClicked(m_tester, &UserListView::clicked);
    gotoRow(1);
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    // Act
    m_tester->keyPressEvent(&ev);
    // Assert
    EXPECT_EQ(spyClicked.count(), 0);
    EXPECT_EQ(m_tester->currentIndex().row(), 1);
}

// 右键按下发送 signalRightButtonClicked 且不进入基类按压流程
TEST_F(UT_UserListView, MousePress_RightButton_EmitsRightClickSignal)
{
    // Arrange
    QSignalSpy spyRight(m_tester, &UserListView::signalRightButtonClicked);
    QSignalSpy spyPressed(m_tester, &UserListView::pressed);
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(5.0, 7.0), QPointF(5.0, 7.0),
                   Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    // Act
    m_tester->mousePressEvent(&ev);
    // Assert
    EXPECT_EQ(spyRight.count(), 1);
    EXPECT_EQ(spyRight.first().at(0).toPoint(), QPoint(5, 7));
    EXPECT_EQ(spyPressed.count(), 0);
}

// 左键按下走基类默认流程，不发送右键信号
TEST_F(UT_UserListView, MousePress_LeftButton_FallsBackToBaseNoRightSignal)
{
    // Arrange
    QSignalSpy spyRight(m_tester, &UserListView::signalRightButtonClicked);
    QSignalSpy spyPressed(m_tester, &UserListView::pressed);
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(5.0, 7.0), QPointF(5.0, 7.0),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    // Act
    m_tester->mousePressEvent(&ev);
    // Assert
    EXPECT_EQ(spyRight.count(), 0);
    EXPECT_EQ(spyPressed.count(), 1);
}

// resetStatus 设置当前行、单选模式并选中该行
TEST_F(UT_UserListView, ResetStatus_ValidIndex_SetsCurrentRowAndSingleSelection)
{
    // Arrange
    QModelIndex idx = m_model->index(2, 0);
    // Act
    m_tester->resetStatus(idx);
    // Assert
    EXPECT_EQ(m_tester->currentIndex().row(), 2);
    EXPECT_EQ(m_tester->selectionMode(), DListView::SingleSelection);
    EXPECT_EQ(m_tester->selectionModel()->selectedIndexes().size(), 1);
}

// resetStatus 二次调用切换到新行并保持单选
TEST_F(UT_UserListView, ResetStatus_ReapplyDifferentRow_UpdatesCurrentRow)
{
    // Arrange
    m_tester->resetStatus(m_model->index(0, 0));
    // Act
    m_tester->resetStatus(m_model->index(3, 0));
    // Assert
    EXPECT_EQ(m_tester->currentIndex().row(), 3);
    EXPECT_EQ(m_tester->selectionModel()->selectedIndexes().size(), 1);
}

// moveCursor 切换为多选模式并返回上移一行的索引
TEST_F(UT_UserListView, MoveCursor_MoveUp_ReturnsRowAboveAndMultiSelection)
{
    // Arrange
    gotoRow(1);
    // Act
    QModelIndex res = m_tester->moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);
    // Assert
    EXPECT_EQ(res.row(), 0);
    EXPECT_EQ(m_tester->selectionMode(), DListView::MultiSelection);
}

// moveCursor 切换为多选模式并返回下移一行的索引
TEST_F(UT_UserListView, MoveCursor_MoveDown_ReturnsRowBelowAndMultiSelection)
{
    // Arrange
    gotoRow(1);
    // Act
    QModelIndex res = m_tester->moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);
    // Assert
    EXPECT_EQ(res.row(), 2);
    EXPECT_EQ(m_tester->selectionMode(), DListView::MultiSelection);
}

// currentChanged 任何切换都强制单选模式
TEST_F(UT_UserListView, CurrentChanged_Transition_EnforcesSingleSelection)
{
    // Arrange
    gotoRow(1);
    // Act
    m_tester->currentChanged(m_model->index(0, 0), m_model->index(1, 0));
    // Assert
    EXPECT_EQ(m_tester->selectionMode(), DListView::SingleSelection);
    EXPECT_EQ(m_tester->currentIndex().row(), 1);
}

// 构造函数关闭横向滚动条且默认无模型
TEST_F(UT_UserListView, Constructor_DefaultState_HorizontalScrollDisabled)
{
    // Arrange
    const Qt::ScrollBarPolicy kOff = Qt::ScrollBarAlwaysOff;
    // Act: 在用例内构造独立实例查询默认状态
    UserListView *view = new UserListView();
    // Assert
    EXPECT_EQ(view->horizontalScrollBarPolicy(), kOff);
    EXPECT_EQ(view->model(), nullptr);
    delete view;
}
