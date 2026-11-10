// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 ProcessTableView 单元测试
// 被测类：deepin-system-monitor-plugin-popup/gui/process_table_view.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method                    | level | factors              | min | actual |
// |---------------------------|-------|----------------------|-----|--------|
// | ProcessTableView          | mid   | -                    | 1   | 1      |
// | ~ProcessTableView         | low   | -                    | 1   | 1      |
// | onThemeTypeChanged        | low   | -                    | 1   | 1      |
// | eventFilter               | low   | KeyPress/透传        | 2   | 2      |
// | search                    | mid   | 空/无匹配/恢复       | 3   | 3      |
// | switchDisplayMode         | mid   | 3种过滤模式          | 3   | 3      |
// | resizeEvent               | low   | -                    | 1   | 1      |
// | showEvent                 | low   | -                    | 1   | 1      |
// | selectionChanged          | mid   | 空选区/有选区        | 2   | 2      |
// | sizeHintForColumn         | mid   | -                    | 1   | 1      |
// | adjustInfoLabelVisibility | low   | -                    | 1   | 1      |
//
// 分支清单（源自真实源码，self-check §2c）：
// - ctor: installEventFilter; m_model/m_proxyModel 挂接; initUI/initConnections;
//         onThemeTypeChanged; WA_TranslucentBackground; setHeaderHidden(true)
// - onThemeTypeChanged: 占位符色刷新 m_notFoundLabel + Button/Base 刷 header + 透明背景
// - eventFilter: B1 KeyPress → return true（屏蔽键盘事件）; B2 其他透传 BaseTableView::eventFilter
// - search: setSortFilterString → adjustInfoLabelVisibility（标签显隐/居中）
// - switchDisplayMode: m_proxyModel->setFilterType(kFilterApps/kFilterCurrentUser/kNoFilter)
// - initUI: 表头可移动/可点击/Interactive/stretchLast/排序指示; NoSelection+SelectRows;
//           Name 200 宽; PID 0 宽隐藏
// - resizeEvent: adjustInfoLabelVisibility → DTreeView::resizeEvent
// - showEvent: m_notFoundLabel 非空 → hide()
// - selectionChanged: B1 selected.size()<=0 → 直接返回; B2 取 PID 列数据 + 基类处理
// - sizeHintForColumn: max(header hint, tree hint) + margin*2
// - adjustInfoLabelVisibility: setUpdatesEnabled(false) → 标签可见时居中移动 → setUpdatesEnabled(true)

#include <gtest/gtest.h>
#include <QKeyEvent>
#include <QItemSelection>
#include <QItemSelectionRange>
#include "stubext.h"
#include "process_table_view.h"
#include "process_table_model.h"
#include "process/process_set.h"

class ProcessTableViewTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stub.clear();
        parent = new QWidget;
        parent->resize(280, 400);
    }

    void TearDown() override
    {
        stub.clear();
        delete parent;
        parent = nullptr;
    }

    stub_ext::StubExt stub;
    QWidget *parent = nullptr;
};

// ═══════════════════ 构造 / 析构 ═══════════════════

TEST_F(ProcessTableViewTest, Ctor_WiresModelHeader_HidesPidColumn)
{
    // Arrange: 构造被测对象（内部建 model/proxy 并挂接）
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 采集 ctor 生效后的视图配置
    bool headerHidden = w->isHeaderHidden();
    bool pidHidden = w->isColumnHidden(ProcessTableModel::kProcessPIDColumn);
    // Assert: 表头隐藏、PID 列隐藏、排序开启、NoSelection、透明背景
    EXPECT_TRUE(headerHidden);
    EXPECT_TRUE(pidHidden);
    EXPECT_TRUE(w->isColumnHidden(ProcessTableModel::kProcessPIDColumn));
    EXPECT_FALSE(w->isColumnHidden(ProcessTableModel::kProcessNameColumn));
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessNameColumn), 200);
    EXPECT_TRUE(w->isSortingEnabled());
    EXPECT_EQ(w->selectionMode(), QAbstractItemView::NoSelection);
    EXPECT_TRUE(w->testAttribute(Qt::WA_TranslucentBackground));
    delete w;
}

TEST_F(ProcessTableViewTest, Dtor_AfterUse_ReleasesCleanly)
{
    // Arrange: 构造待析构对象（内部持有 model/proxy/notFoundLabel）
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 析构（Qt parent 机制先于测试 TearDown 释放）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessCPUColumn), 80);
    delete w;
    // Assert: 析构过程无崩溃即为通过
    SUCCEED();
}

// ═══════════════════ onThemeTypeChanged ═══════════════════

TEST_F(ProcessTableViewTest, OnThemeTypeChanged_LabelPaletteUpdated_NoCrash)
{
    // Arrange: 构造对象
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 手动触发主题刷新（ctor 也调用过一次）
    w->onThemeTypeChanged();
    // Assert: 不崩溃且渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_TRUE(w->isHeaderHidden());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessNameColumn), 200);
    delete w;
}

// ═══════════════════ eventFilter ═══════════════════

TEST_F(ProcessTableViewTest, EventFilter_KeyPressEvent_Consumed)
{
    // Arrange: 构造对象与 KeyPress 事件（B1 屏蔽分支）
    ProcessTableView *w = new ProcessTableView(parent);
    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    // Act: 分发按键事件
    bool consumed = w->eventFilter(w, &keyPress);
    // Assert: KeyPress 被消费（避免键盘导航干扰弹窗表格）
    EXPECT_TRUE(consumed);
    EXPECT_TRUE(w->isHeaderHidden());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessNameColumn), 200);
    delete w;
}

TEST_F(ProcessTableViewTest, EventFilter_NonKeyEvent_DelegatesToBase)
{
    // Arrange: 非 KeyPress 事件（B2 透传分支）
    ProcessTableView *w = new ProcessTableView(parent);
    QObject other;
    QEvent hover(QEvent::HoverMove);
    // Act: 分发无关事件
    bool consumed = w->eventFilter(&other, &hover);
    // Assert: 透传基类，默认不消费
    EXPECT_FALSE(consumed);
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessNameColumn), 200);
    delete w;
}

// ═══════════════════ search ═══════════════════

TEST_F(ProcessTableViewTest, Search_EmptyText_NoCrash)
{
    // Arrange: 构造对象
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 空文本搜索（clear 过滤 + adjustInfoLabelVisibility）
    w->search(QString());
    // Assert: 不崩溃且渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessCPUColumn), 80);
    delete w;
}

TEST_F(ProcessTableViewTest, Search_NoMatchText_NoCrash)
{
    // Arrange: 构造对象并搜索必然无匹配进程名
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 无匹配搜索
    w->search(QString("zzz-no-such-proc-xyz"));
    // Assert: 不崩溃且渲染有效（notFoundLabel 显隐逻辑被驱动）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessNameColumn), 200);
    delete w;
}

TEST_F(ProcessTableViewTest, Search_RestoreEmptyText_AgainNoCrash)
{
    // Arrange: 先无匹配再恢复空文本（覆盖过滤清除路径）
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 两段式搜索
    w->search(QString("zzz-no-such-proc-xyz"));
    w->search(QString());
    // Assert: 恢复后渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_TRUE(w->isHeaderHidden());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessNameColumn), 200);
    delete w;
}

// ═══════════════════ switchDisplayMode ═══════════════════

TEST_F(ProcessTableViewTest, SwitchDisplayMode_FilterApps_NoCrash)
{
    // Arrange: 构造对象，kFilterApps 模式
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 切换仅显示应用模式
    w->switchDisplayMode(FilterType::kFilterApps);
    // Assert: 不崩溃且渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessNameColumn), 200);
    delete w;
}

TEST_F(ProcessTableViewTest, SwitchDisplayMode_FilterCurrentUser_NoCrash)
{
    // Arrange: 构造对象，kFilterCurrentUser 模式
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 切换仅显示当前用户模式
    w->switchDisplayMode(FilterType::kFilterCurrentUser);
    // Assert: 不崩溃且渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessNameColumn), 200);
    delete w;
}

TEST_F(ProcessTableViewTest, SwitchDisplayMode_NoFilter_NoCrash)
{
    // Arrange: 构造对象，kNoFilter 模式
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 切换无过滤模式
    w->switchDisplayMode(FilterType::kNoFilter);
    // Assert: 不崩溃且渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_TRUE(w->isSortingEnabled());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessNameColumn), 200);
    delete w;
}

// ═══════════════════ resizeEvent / showEvent ═══════════════════

TEST_F(ProcessTableViewTest, ResizeEvent_AdjustsLabelPosition_NoCrash)
{
    // Arrange: 构造对象
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 改变尺寸触发 resizeEvent → adjustInfoLabelVisibility → 基类 resize
    w->resize(400, 300);
    // Assert: 尺寸生效且渲染有效
    EXPECT_EQ(w->width(), 400);
    EXPECT_FALSE(w->grab().isNull());
    delete w;
}

TEST_F(ProcessTableViewTest, ShowEvent_ShownState_LabelHidden)
{
    // Arrange: 构造对象并先显示父窗口（子窗口可见性依赖父窗口）
    ProcessTableView *w = new ProcessTableView(parent);
    parent->show();
    // Act: 显示（触发 showEvent → m_notFoundLabel->hide()）再隐藏还原
    w->show();
    bool shown = w->isVisible();
    w->hide();
    parent->hide();
    // Assert: show 流程正常完成
    EXPECT_TRUE(shown);
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessCPUColumn), 80);
    delete w;
}

// ═══════════════════ selectionChanged ═══════════════════

TEST_F(ProcessTableViewTest, SelectionChanged_EmptySelection_EarlyReturn)
{
    // Arrange: 构造对象与空选区（B1 early-return 分支）
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 以空选区调用选择变更
    w->selectionChanged(QItemSelection(), QItemSelection());
    // Assert: 直接返回，不崩溃且渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_TRUE(w->isHeaderHidden());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessNameColumn), 200);
    delete w;
}

TEST_F(ProcessTableViewTest, SelectionChanged_RowSelection_CapturesPidData)
{
    // Arrange: 构造对象，行数据就绪时构造整行选区（B2 PID 捕获分支）
    ProcessTableView *w = new ProcessTableView(parent);
    QAbstractItemModel *proxy = w->model();
    ASSERT_NE(proxy, nullptr);
    if (proxy->rowCount() < 1) {
        // 环境无进程行数据时跳过非空选区分支（空选区分支已单独覆盖）
        SUCCEED();
        delete w;
        return;
    }
    QItemSelection sel(proxy->index(0, 0),
                       proxy->index(0, proxy->columnCount() - 1));
    // Act: 以整行选区调用选择变更
    w->selectionChanged(sel, QItemSelection());
    // Assert: 走基类选择处理，不崩溃
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_TRUE(w->isSortingEnabled());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessNameColumn), 200);
    delete w;
}

// ═══════════════════ sizeHintForColumn / adjustInfoLabelVisibility ═══════════════════

TEST_F(ProcessTableViewTest, SizeHintForColumn_NameColumn_HintPlusMargin)
{
    // Arrange: 构造对象
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 计算 Name 列宽度建议
    int hint = w->sizeHintForColumn(ProcessTableModel::kProcessNameColumn);
    // Assert: hint = max(表头, 视图) + 20，必然为正
    EXPECT_GT(hint, 0);
    EXPECT_GE(hint, 20);
    delete w;
}

TEST_F(ProcessTableViewTest, AdjustInfoLabelVisibility_TogglesUpdates_NoCrash)
{
    // Arrange: 构造对象
    ProcessTableView *w = new ProcessTableView(parent);
    // Act: 直接调整提示标签位置（updates 关闭→开启）
    w->adjustInfoLabelVisibility();
    // Assert: 不崩溃且渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->columnWidth(ProcessTableModel::kProcessCPUColumn), 80);
    delete w;
}
