// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * UT_BaseTableView
 *
 * 测试对象: deepin-system-monitor-plugin-popup/gui/base/base_table_view.cpp
 *           BaseTableView 类
 * 外部依赖: 无 DBus/网络/设备依赖;视图渲染到内存 QPixmap,
 *           悬浮/触摸/焦点/鼠标事件通过 sendEvent 直接送入 viewport。
 *
 * 分支覆盖声明 (viewportEvent, complexity=14, 源码行 96-153):
 * | #  | 分支条件                                        | 覆盖用例                                   |
 * |----|-------------------------------------------------|--------------------------------------------|
 * | 1  | HoverLeave 且 m_hover 有效 → 清除并局部刷新      | viewportEvent_hoverMove_updatesHoverIndex  |
 * | 2  | HoverMove 与旧悬浮不同 → 刷新旧区域              | viewportEvent_hoverMove_updatesHoverIndex  |
 * | 3  | HoverMove 悬浮有效 → 刷新新区域                  | viewportEvent_hoverMove_updatesHoverIndex  |
 * | 4  | MouseMove/Release/DblClick/Press → return false | viewportEvent_mouseEvents_returnFalse      |
 * | 5  | TouchEnd → 清空 m_hover                         | viewportEvent_touchEnd_clearsHover         |
 * | 6  | default → 基类处理                               | viewportEvent_touchEnd_clearsHover         |
 *
 * 分支覆盖声明 (scrollTo, complexity=10, 源码行 155-207):
 * | #  | 分支条件                                        | 覆盖用例                                   |
 * |----|-------------------------------------------------|--------------------------------------------|
 * | 7  | index 无效/非本模型 → return                    | scrollTo_invalidOrForeignIndex_noScroll    |
 * | 8  | EnsureVisible 且已可见 → 局部刷新不动滚动条      | scrollTo_visibleIndex_keepsScrollBar       |
 * | 9  | PositionAtBottom → 滚到底部                     | scrollTo_farIndex_scrollsToBottom          |
 * |10  | EnsureVisible 且在上方 → 回滚到顶部              | scrollTo_farIndex_scrollsToBottom          |
 * |11  | 滚动前重置 m_hover                              | scrollTo_farIndex_scrollsToBottom          |
 *
 * 用例声明表:
 * |------------------------------------------------|-------------------|---------|------|-------------------------|
 * | 测试用例名                                      | 测试类            | 测试级别 | 类型 | 覆盖函数                |
 * |------------------------------------------------|-------------------|---------|------|-------------------------|
 * | constructor_defaults_headerAndPolicy           | UT_BaseTableView  | Level_1 | AUTO | ctor                    |
 * | setModel_connectsReset_resetsHoverAndPressed   | UT_BaseTableView  | Level_2 | AUTO | setModel                |
 * | viewportEvent_hoverMove_updatesHoverIndex      | UT_BaseTableView  | Level_1 | AUTO | viewportEvent(分支1-3)  |
 * | viewportEvent_mouseEvents_returnFalse          | UT_BaseTableView  | Level_2 | AUTO | viewportEvent(分支4)    |
 * | viewportEvent_touchEnd_clearsHover             | UT_BaseTableView  | Level_2 | AUTO | viewportEvent(分支5/6)  |
 * | focusInEvent_Reasons_RecordsLastFocus                | UT_BaseTableView  | Level_2 | AUTO | focusInEvent            |
 * | currentChanged_NewIndex_UpdatesCurrentRow             | UT_BaseTableView  | Level_1 | AUTO | currentChanged          |
 * | paintEvent_rendersRows_withCurrentSelection    | UT_BaseTableView  | Level_1 | AUTO | paintEvent              |
 * | drawRow_pressedAndHover_renderDiffers          | UT_BaseTableView  | Level_2 | AUTO | drawRow                 |
 * | scrollTo_invalidOrForeignIndex_noScroll        | UT_BaseTableView  | Level_2 | AUTO | scrollTo(分支7)         |
 * | scrollTo_visibleIndex_keepsScrollBar           | UT_BaseTableView  | Level_1 | AUTO | scrollTo(分支8)         |
 * | scrollTo_farIndex_scrollsToBottom              | UT_BaseTableView  | Level_1 | AUTO | scrollTo(分支9-11)      |
 *
 * 用例计数声明(方法 | level | factors | min | actual):
 * | drawRow | high | complexity:10 | 3 | 3 |
 * | viewportEvent | high | complexity:14 | 3 | 3 |
 * | 构造函数 | mid | lines:61 | 1 | 1 |
 * | paintEvent | mid | lines:57 | 1 | 1 |
 * | scrollTo | mid | complexity:6 | 2 | 3 |
 * | setModel | low | - | 1 | 1 |
 * | currentChanged | low | - | 1 | 1 |
 * | focusInEvent | low | - | 1 | 1 |
 *
 * 风险检查清单:
 * 1. 对每一个测试文件中的 stub 都执行了 reset?  [是] 未使用进程级 stub
 * 2. 对每一个测试文件中的 stub 都执行了 restore? [是] 无进程级 stub
 * 3. 所有的测试使用 ASSERT/EXPECT 宏而不是布尔判断? [是]
 * 4. 所有的 ASSERT 宏的返回值都被检查? [是]
 * 5. 用例覆盖了所有分支? [是] viewportEvent/scrollTo 主要分支见上方分支声明表
 * 6. 本文件与类定义的头文件一致(用例使用的是头文件中公开的方法)? [是]
 * 7. 所有的属性和方法都被测试到了? [是] 私有成员 m_hover/m_pressed/m_focusReason/m_itemDelegate/m_headerView 直接访问
 * 8. 测试代码可读性高,测试方法意义明确? [是]
 * 9. 任何测试不能对其它测试产生依赖? [是] 每用例独立构造视图与模型
 * 10. 使用的是 Google Test 而不是 QtTest? [是]
 */

#include <QApplication>
#include <QFocusEvent>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QScroller>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QTouchEvent>
#include <DGuiApplicationHelper>
#include <gtest/gtest.h>

#include "gui/base/base_header_view.h"
#include "gui/base/base_item_delegate.h"
#include "gui/base/base_table_view.h"

DGUI_USE_NAMESPACE

// ==================== 私有成员访问说明 ====================
// 测试目标由 tests/CMakeLists.txt 全局追加 -fno-access-control 编译,
// 测试代码可直接访问 private/protected 成员,无需 addr_pri.h 宏。

// ==================== 测试夹具 ====================
class UT_BaseTableView : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_tester = new BaseTableView();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
    }

    // 构造 rows x 2 列的标准模型
    QStandardItemModel *makeModel(int rows)
    {
        QStandardItemModel *model = new QStandardItemModel(rows, 2);
        for (int r = 0; r < rows; ++r) {
            model->setItem(r, 0, new QStandardItem(QStringLiteral("item-%1").arg(r)));
            model->setItem(r, 1, new QStandardItem(QStringLiteral("%1%").arg(r)));
        }
        return model;
    }

    // 显示视图并完成一次布局
    void showView(int w, int h)
    {
        m_tester->resize(w, h);
        m_tester->show();
        QApplication::processEvents();
    }

    BaseTableView *m_tester = nullptr;
};

// ==================== 测试用例 ====================

// 用例名称: constructor_defaults_headerAndPolicy
// 测试类型: 正常场景
// 覆盖函数: 构造函数
// 操作步骤: 构造视图,检查表头/代理类型与交互属性
// 预期结果: 表头与代理为 Base 类型;触摸接受、节头可移动可点击、末列拉伸、排序指示可见、强焦点
TEST_F(UT_BaseTableView, constructor_defaults_headerAndPolicy)
{
    // Arrange: 夹具默认构造视图
    // Act: 读取私有部件指针
    BaseItemDelegate *delegate = m_tester->m_itemDelegate;
    BaseHeaderView *header = m_tester->m_headerView;
    // Assert: 类型与属性
    EXPECT_NE(dynamic_cast<BaseItemDelegate *>(delegate), nullptr);
    EXPECT_NE(dynamic_cast<BaseHeaderView *>(header), nullptr);
    EXPECT_TRUE(m_tester->testAttribute(Qt::WA_AcceptTouchEvents));
    EXPECT_EQ(m_tester->frameShape(), QFrame::NoFrame);
    EXPECT_FALSE(m_tester->rootIsDecorated());
    EXPECT_TRUE(header->sectionsMovable());
    EXPECT_TRUE(header->sectionsClickable());
    EXPECT_TRUE(header->stretchLastSection());
    EXPECT_TRUE(header->isSortIndicatorShown());
    // 构造函数未开启模型排序(排序点击由 header 视图自身处理),QTreeView 默认关闭
    EXPECT_FALSE(m_tester->isSortingEnabled());
    EXPECT_EQ(m_tester->focusPolicy(), Qt::StrongFocus);
    EXPECT_EQ(m_tester->m_focusReason, int(Qt::TabFocusReason));
}

// 用例名称: setModel_connectsReset_resetsHoverAndPressed
// 测试类型: 边界场景
// 覆盖函数: setModel(模型复位信号联动)
// 操作步骤: 设置模型后置有效悬浮/按压,再触发模型复位
// 预期结果: 复位后悬浮与按压索引被重置
TEST_F(UT_BaseTableView, setModel_connectsReset_resetsHoverAndPressed)
{
    // Arrange: 模型与有效悬浮/按压
    QScopedPointer<QStandardItemModel> model(makeModel(4));
    m_tester->setModel(model.data());
    showView(200, 200);
    m_tester->m_hover = model->index(0, 0);
    m_tester->m_pressed = model->index(1, 0);
    EXPECT_TRUE(m_tester->m_hover.isValid());
    EXPECT_TRUE(m_tester->m_pressed.isValid());
    // Act: 模型复位
    model->beginResetModel();
    model->endResetModel();
    QApplication::processEvents();
    // Assert: 状态重置
    EXPECT_FALSE(m_tester->m_hover.isValid());
    EXPECT_FALSE(m_tester->m_pressed.isValid());
}

// 用例名称: viewportEvent_hoverMove_updatesHoverIndex
// 测试类型: 正常场景
// 覆盖函数: viewportEvent(分支1-3: HoverMove/HoverLeave)
// 操作步骤: 向 viewport 依次发送行上方 HoverMove 与 HoverLeave
// 预期结果: 悬浮索引随事件建立与清除
TEST_F(UT_BaseTableView, viewportEvent_hoverMove_updatesHoverIndex)
{
    // Arrange: 显示 4 行视图
    QScopedPointer<QStandardItemModel> model(makeModel(4));
    m_tester->setModel(model.data());
    showView(200, 200);
    const QRect rowRect = m_tester->visualRect(model->index(0, 0));
    ASSERT_TRUE(rowRect.isValid());
    // Act: 悬浮至第 0 行
    QHoverEvent moveEv(QEvent::HoverMove, rowRect.center(), QPointF(-1, -1));
    QApplication::sendEvent(m_tester->viewport(), &moveEv);
    // Assert: 悬浮索引指向第 0 行
    EXPECT_TRUE(m_tester->m_hover.isValid());
    EXPECT_EQ(m_tester->m_hover.row(), 0);
    // Act: 悬浮离开
    QHoverEvent leaveEv(QEvent::HoverLeave, QPointF(-1, -1), rowRect.center());
    QApplication::sendEvent(m_tester->viewport(), &leaveEv);
    // Assert: 悬浮清除
    EXPECT_FALSE(m_tester->m_hover.isValid());
}

// 用例名称: viewportEvent_mouseEvents_returnFalse
// 测试类型: 边界场景
// 覆盖函数: viewportEvent(分支4: 鼠标事件返回 false 交回基类)
// 操作步骤: 向 viewport 发送 Press/Release/DblClick/Move 四类鼠标事件
// 预期结果: 事件处理结果均为 false
TEST_F(UT_BaseTableView, viewportEvent_mouseEvents_returnFalse)
{
    // Arrange: 显示视图
    QScopedPointer<QStandardItemModel> model(makeModel(4));
    m_tester->setModel(model.data());
    showView(200, 200);
    const QPoint pos = m_tester->visualRect(model->index(0, 0)).center();
    // 说明:sendEvent 经 viewport 过滤器链后最终返回值会被 QWidget::event 改写为 true,
    // 无法直接观测 viewportEvent 的返回值,故直接调用被测函数断言其返回契约。
    // Act: 执行上述步骤
    // Assert: 四类鼠标事件
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(pos), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    EXPECT_FALSE(m_tester->viewportEvent(&press));
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(pos), Qt::LeftButton,
                        Qt::LeftButton, Qt::NoModifier);
    EXPECT_FALSE(m_tester->viewportEvent(&release));
    QMouseEvent dblClick(QEvent::MouseButtonDblClick, QPointF(pos), Qt::LeftButton,
                         Qt::LeftButton, Qt::NoModifier);
    EXPECT_FALSE(m_tester->viewportEvent(&dblClick));
    QMouseEvent move(QEvent::MouseMove, QPointF(pos), Qt::NoButton,
                     Qt::NoButton, Qt::NoModifier);
    EXPECT_FALSE(m_tester->viewportEvent(&move));
}

// 用例名称: viewportEvent_touchEnd_clearsHover
// 测试类型: 边界场景
// 覆盖函数: viewportEvent(分支5/6: TouchEnd 清悬浮)
// 操作步骤: 先建立悬浮,再发送 TouchEnd
// 预期结果: TouchEnd 后悬浮索引被清空
TEST_F(UT_BaseTableView, viewportEvent_touchEnd_clearsHover)
{
    // Arrange: 显示视图并建立悬浮(HoverMove 直接调用被测函数,与 sendEvent 路径等效)
    QScopedPointer<QStandardItemModel> model(makeModel(4));
    m_tester->setModel(model.data());
    showView(200, 200);
    const QPointF rowPos = m_tester->visualRect(model->index(0, 0)).center();
    QHoverEvent moveEv(QEvent::HoverMove, rowPos, QPointF(-1, -1));
    m_tester->viewportEvent(&moveEv);
    EXPECT_TRUE(m_tester->m_hover.isValid());
    // Act: 触摸结束(QTouchEvent 保证基类尾路径的安全转换)
    QTouchEvent touchEndEv(QEvent::TouchEnd, QPointingDevice::primaryPointingDevice());
    m_tester->viewportEvent(&touchEndEv);
    // Assert: 悬浮清空
    EXPECT_FALSE(m_tester->m_hover.isValid());
}

// 用例名称: focusInEvent_Reasons_RecordsLastFocus
// 测试类型: 边界场景
// 覆盖函数: focusInEvent
// 操作步骤: 直接发送不同原因的 FocusIn 事件
// 预期结果: m_focusReason 记录最近一次焦点原因
TEST_F(UT_BaseTableView, focusInEvent_Reasons_RecordsLastFocus)
{
    // Arrange: 夹具视图无模型焦点,记录初值
    const int initialReason = m_tester->m_focusReason;
    // Act: 鼠标焦点进入
    QFocusEvent mouseFocus(QEvent::FocusIn, Qt::MouseFocusReason);
    QApplication::sendEvent(m_tester, &mouseFocus);
    // Assert: 原因被记录
    EXPECT_EQ(m_tester->m_focusReason, int(Qt::MouseFocusReason));
    EXPECT_NE(m_tester->m_focusReason, initialReason);
    // Act: Tab 焦点进入
    QFocusEvent tabFocus(QEvent::FocusIn, Qt::TabFocusReason);
    QApplication::sendEvent(m_tester, &tabFocus);
    // Assert: 原因更新
    EXPECT_EQ(m_tester->m_focusReason, int(Qt::TabFocusReason));
}

// 用例名称: currentChanged_NewIndex_UpdatesCurrentRow
// 测试类型: 正常场景
// 覆盖函数: currentChanged
// 操作步骤: 设置模型后依次 setCurrentIndex
// 预期结果: 当前索引随调用更新
TEST_F(UT_BaseTableView, currentChanged_NewIndex_UpdatesCurrentRow)
{
    // Arrange: 显示 4 行视图
    QScopedPointer<QStandardItemModel> model(makeModel(4));
    m_tester->setModel(model.data());
    showView(200, 200);
    // Act: 设置当前行
    m_tester->setCurrentIndex(model->index(1, 0));
    // Assert: 当前行更新
    EXPECT_EQ(m_tester->currentIndex().row(), 1);
    // Act: 切换当前行
    m_tester->setCurrentIndex(model->index(2, 0));
    // Assert: 当前行切换
    EXPECT_EQ(m_tester->currentIndex().row(), 2);
}

// 用例名称: paintEvent_rendersRows_withCurrentSelection
// 测试类型: 正常场景
// 覆盖函数: paintEvent
// 操作步骤: 显示视图,设置当前行与选中行后离屏抓图两次(当前行切换)
// 预期结果: 渲染完成且两次图像不同(当前行指示位置变化)
TEST_F(UT_BaseTableView, paintEvent_rendersRows_withCurrentSelection)
{
    // Arrange: 显示 4 行视图,选中第 1 行
    QScopedPointer<QStandardItemModel> model(makeModel(4));
    m_tester->setModel(model.data());
    showView(200, 200);
    m_tester->selectionModel()->select(model->index(1, 0), QItemSelectionModel::Select);
    m_tester->setCurrentIndex(model->index(1, 0));
    // Act: 两次抓图(当前行不同)
    const QImage first = m_tester->grab().toImage();
    m_tester->setCurrentIndex(model->index(2, 0));
    const QImage second = m_tester->grab().toImage();
    // Assert: 渲染有效且随状态变化
    EXPECT_FALSE(first.isNull());
    EXPECT_EQ(first.width(), 200);
    EXPECT_NE(first, second);
}

// 用例名称: drawRow_pressedAndHover_renderDiffers
// 测试类型: 边界场景
// 覆盖函数: drawRow(按压/悬浮背景分支)
// 操作步骤: 通过私有字段注入按压与悬浮索引后抓图,再对比普通态渲染
// 预期结果: 按压/悬浮态与普通态渲染结果不同
TEST_F(UT_BaseTableView, drawRow_pressedAndHover_renderDiffers)
{
    // Arrange: 显示 4 行视图,当前行 1
    QScopedPointer<QStandardItemModel> model(makeModel(4));
    m_tester->setModel(model.data());
    showView(200, 200);
    m_tester->setCurrentIndex(model->index(1, 0));
    const QImage plain = m_tester->grab().toImage();
    // Act: 注入按压与悬浮(第 1 行)后抓图
    m_tester->m_pressed = model->index(1, 0);
    m_tester->m_hover = model->index(1, 0);
    const QImage pressedHover = m_tester->grab().toImage();
    // Assert: 渲染差异
    EXPECT_FALSE(pressedHover.isNull());
    EXPECT_NE(plain, pressedHover);
}

// 用例名称: drawRow_hoverOnly_HighlightsHoverRow
// 测试类型: 正常场景
// 覆盖函数: drawRow(仅悬浮分支)
// 操作步骤: 仅注入悬浮索引(不按压)后抓图,对比普通态
// 预期结果: 悬浮行高亮,渲染与普通态不同且图像有效
TEST_F(UT_BaseTableView, drawRow_hoverOnly_HighlightsHoverRow)
{
    // Arrange: 显示 4 行视图,记录普通态
    QScopedPointer<QStandardItemModel> model(makeModel(4));
    m_tester->setModel(model.data());
    showView(200, 200);
    const QImage plain = m_tester->grab().toImage();
    // Act: 仅注入悬浮索引后抓图
    m_tester->m_hover = model->index(2, 0);
    const QImage hovered = m_tester->grab().toImage();
    // Assert: 悬浮高亮生效
    EXPECT_FALSE(hovered.isNull());
    EXPECT_NE(plain, hovered);
}

// 用例名称: drawRow_pressedOnly_HighlightsPressedRow
// 测试类型: 正常场景
// 覆盖函数: drawRow(仅按压分支)
// 操作步骤: 仅注入按压索引(不悬浮)后抓图,对比普通态
// 预期结果: 按压行背景变化,渲染与普通态不同
TEST_F(UT_BaseTableView, drawRow_pressedOnly_HighlightsPressedRow)
{
    // Arrange: 显示 4 行视图,记录普通态
    QScopedPointer<QStandardItemModel> model(makeModel(4));
    m_tester->setModel(model.data());
    showView(200, 200);
    const QImage plain = m_tester->grab().toImage();
    // Act: 仅注入按压索引后抓图
    m_tester->m_pressed = model->index(3, 0);
    const QImage pressed = m_tester->grab().toImage();
    // Assert: 按压高亮生效
    EXPECT_FALSE(pressed.isNull());
    EXPECT_NE(plain, pressed);
}

// 用例名称: scrollTo_invalidOrForeignIndex_noScroll
// 测试类型: 边界场景
// 覆盖函数: scrollTo(分支7: 无效/外来 index 直接返回)
// 操作步骤: 用无效 index 与另一模型的 index 调用 scrollTo
// 预期结果: 滚动条保持原位
TEST_F(UT_BaseTableView, scrollTo_invalidOrForeignIndex_noScroll)
{
    // Arrange: 显示 4 行视图
    QScopedPointer<QStandardItemModel> model(makeModel(4));
    m_tester->setModel(model.data());
    showView(200, 200);
    QScopedPointer<QStandardItemModel> other(makeModel(4));
    // Act: 执行上述步骤
    // Assert: 无效与外来 index 均不滚动
    m_tester->scrollTo(QModelIndex());
    EXPECT_EQ(m_tester->verticalScrollBar()->value(), 0);
    m_tester->scrollTo(other->index(0, 0));
    EXPECT_EQ(m_tester->verticalScrollBar()->value(), 0);
}

// 用例名称: scrollTo_visibleIndex_keepsScrollBar
// 测试类型: 正常场景
// 覆盖函数: scrollTo(分支8: EnsureVisible 已可见,只刷新不滚动)
// 操作步骤: 对可见的首行调用 scrollTo(EnsureVisible)
// 预期结果: 滚动条保持 0
TEST_F(UT_BaseTableView, scrollTo_visibleIndex_keepsScrollBar)
{
    // Arrange: 显示 4 行视图
    QScopedPointer<QStandardItemModel> model(makeModel(4));
    m_tester->setModel(model.data());
    showView(200, 300);
    // Act: 滚动到已可见行
    m_tester->scrollTo(model->index(0, 0), QAbstractItemView::EnsureVisible);
    // Assert: 滚动条不动且首行仍在可视区
    EXPECT_EQ(m_tester->verticalScrollBar()->value(), 0);
    EXPECT_TRUE(m_tester->visualRect(model->index(0, 0)).intersects(m_tester->viewport()->rect()));
}

// 用例名称: scrollTo_farIndex_scrollsToBottom
// 测试类型: 正常场景
// 覆盖函数: scrollTo(分支9-11: 远处行滚动 + 悬浮重置 + 上方回滚)
// 操作步骤: 50 行模型滚动到底部行,验证滚动条与悬浮重置,再 EnsureVisible 回顶部
// 预期结果: 底部滚动值大于 0 且悬浮被清空;回滚后滚动条归 0
TEST_F(UT_BaseTableView, scrollTo_farIndex_scrollsToBottom)
{
    // Arrange: 50 行视图并预置悬浮
    QScopedPointer<QStandardItemModel> model(makeModel(50));
    m_tester->setModel(model.data());
    showView(200, 300);
    m_tester->m_hover = model->index(0, 0);
    // Act: 滚动到底部行
    m_tester->scrollTo(model->index(49, 0), QAbstractItemView::PositionAtBottom);
    // Assert: 滚动发生且悬浮被重置
    EXPECT_GT(m_tester->verticalScrollBar()->value(), 0);
    EXPECT_FALSE(m_tester->m_hover.isValid());
    // Act: EnsureVisible 回到顶部行
    m_tester->scrollTo(model->index(0, 0), QAbstractItemView::EnsureVisible);
    // Assert: 滚动条回到顶部
    EXPECT_EQ(m_tester->verticalScrollBar()->value(), 0);
}
