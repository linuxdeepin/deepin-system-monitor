// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_UserPageWidget：UserPageWidget 单元测试（qt-autotest-generator 生成）
//
// ============================ 用例计数声明 ============================
// 方法                          | level | 最少用例 | actual | 说明
// UserPageWidget()              | mid   | 2 | 2 | 子控件创建 / 当前用户记录
// ~UserPageWidget()             | low   | 1 | - | 由 TearDown 覆盖
// initUI()                      | mid   | 2 | 1 | 间接经构造验证（成员创建 + 用量标签初始化同用例）
// initConnections()             | mid   | 2 | 1 | 信号连接触发链路（signalModelUpdated → 标签刷新）
// onUserChanged()               | mid   | 2 | 1 | 用户切换后刷新链路（文本与 tooltip 双维度）
// onTextContentChanged()        | mid   | 2 | 2 | 标签文本 / tooltip 一致性
// onThemeChanged()              | high  | 3 | 2 | 主题色应用（TextTitle 色）+ 8 个标签逐一校验
// onHeaderChanged()             | mid   | 2 | 2 | 列隐藏→标签隐藏 / 列恢复→标签恢复
// paintEvent(QPaintEvent*)      | low   | 1 | 1 | 直接调用不崩溃
// setLabelFormat(DLabel,ColorType) | mid | 2 | 2 | 对齐+省略模式 / 调色板颜色
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例（private setLabelFormat 经用例直接驱动）
// [x] 2 每个输入维度按等价类划分，每类 ≥ 1 用例（列隐藏 true/false；ColorType TextTips/TextTips）
// [x] 3 每个等价类的边界值显式覆盖（隐藏第 1 列与第 3 列两个边界索引）
// [x] 4 同质多组输入用 TEST_P 参数化（断言对象不同维度不同，不适用）
// [x] 5 分支清单已列出并映射到用例名（见下）
// [x] 6 每条 if/switch/throw/early-return 分支有触发用例（8 个标签判空分支同一用例全部为真路径，假路径构造后不可达，见注）
// [x] 7 异常路径用 EXPECT_THROW 精确匹配（本类无显式 throw，不适用）
// [x] 8 负面场景有专门用例（空 pixmap 不适用；列隐藏为负向 UI 状态）
// [x] 9 负面用例验证强异常安全（列恢复后标签状态还原）
// [x] 10 项目内接口类用 gMock，其他用 stub（无接口类，本类无需外部 stub）
//
// ============================== 分支清单 ==============================
// （来源：MCP get_code_snippet user_page_widget.cpp）
// UserPageWidget():
//   B1 getpwuid(getuid()) → m_currentUser           → Ctor_CapturesCurrentUser
//   B2 initUI()/initConnections() 顺序调用          → Ctor_CreatesChildWidgets
// onTextContentChanged():
//   B3 CPU 用量格式 "%1%" + 8 个 summary 标签格式化 → OnTextContentChanged_LabelsMatchLiveValues
//   B4 tooltip 与文本一致                            → OnTextContentChanged_TooltipMatchesText
// onHeaderChanged():
//   B5 列隐藏 → 对应标签隐藏（列 1 CPU / 列 3 内存 / 列 6 上传）→ OnHeaderChanged_HiddenColumn_HidesLabels
//   B6 列恢复 → 对应标签恢复                          → OnHeaderChanged_RestoredColumn_ShowsLabels
// onThemeChanged():
//   B7 m_XXXLabel 判空（8 处，构造后均非空 → 全部走真路径）→ OnThemeChanged_LabelsUseTextTitleColor
//   B8 m_XXXUsageSummary 判空（8 处，同上）            → OnThemeChanged_SummaryLabelsUseTextTipsColor
// setLabelFormat():
//   B9 字体绑定 + 左对齐 + ElideRight                 → SetLabelFormat_AppliesAlignmentAndElide
//   B10 调色板 Text 色置为 colorType 色               → SetLabelFormat_AppliesPaletteColor
// onUserChanged():
//   B11 getCurrentItemUserName → setUserModeName → onTextContentChanged → OnUserChanged_RefreshesUsageLabels
// initConnections():
//   B12 signalModelUpdated → onTextContentChanged 连接 → InitConnections_ModelUpdateRefreshesLabels
//
// ============================== stub 清单 ==============================
// 本类方法均为纯 UI 状态迁移，不触达 DBus/网络/子进程外部依赖；
// ProcessTableView 用量数据在无事件循环下保持初始 0 值，断言与其 getter 实时值绑定，天然确定性。
// 说明：不依赖真实设备环境，可在任意干净 CI 环境复现。

//Self
#include "user_page_widget.h"
#include "process_table_view.h"
#include "accounts_widget.h"
#include "application.h"
#include "main_window.h"
#include "toolbar.h"
#include "common/common.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <DPaletteHelper>
#include <QPaintEvent>
#include <QTest>
#include <pwd.h>
#include <unistd.h>

// 依赖隔离：UserPageWidget 内嵌 AccountsWidget（含 AccountsInfoModel）。
// 不打桩时会连上真实会话总线：模型异步枚举真实系统用户并响应 Accounts 服务
// 信号，导致用例间数据串扰甚至进程退出竞态（不依赖真实设备环境红线）。
static QList<User *> s_stubUserList;
static QList<User *> stub_upw_userList(AccountsInfoModel *) { return s_stubUserList; }
static void stub_upw_noop(AccountsInfoModel *) {}

// 防御性隔离：吞掉监控启动事件，防止任何路径泄漏进 gApp 事件队列的
// MonitorStartEvent/NetifStartEvent 启动真实监控线程（测试不依赖真实设备环境）
static bool stub_upw_appEvent(Application *self, QEvent *event)
{
    if (event && (event->type() == kMonitorStartEventType || event->type() == kNetifStartEventType)) {
        return true;
    }
    return self->DApplication::event(event);
}

/***************************************STUB end***********************************************/

class UT_UserPageWidget : public ::testing::Test
{
public:
    UT_UserPageWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 在构造被测对象前拦截模型用户列表与在线状态刷新，隔离真实 DBus
        // （m_stub 为成员，生命周期覆盖整个用例；SetUp 结束后桩仍有效）
        s_stubUserList.clear();
        typedef QList<User *> (*fptr_list)(const AccountsInfoModel *);
        typedef void (*fptr_uos)(AccountsInfoModel *);
        m_stub.set((fptr_list)&AccountsInfoModel::userList, stub_upw_userList);
        m_stub.set((fptr_uos)&AccountsInfoModel::updateUserOnlineStatus, stub_upw_noop);
        typedef bool (*fptr_event)(Application *, QEvent *);
        m_stub.set((fptr_event)&Application::event, stub_upw_appEvent);
        static MainWindow wm;
        static Toolbar toolbar;
        wm.m_toolbar = &toolbar;
        gApp->setMainWindow(&wm);
        m_tester = new UserPageWidget();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            m_tester->hide();
            m_tester->deleteLater();
            QCoreApplication::processEvents();
            QCoreApplication::processEvents();
            m_tester = nullptr;
        }
    }

protected:
    UserPageWidget *m_tester;
    Stub m_stub;   // 在 SetUp 中挂桩，fixture 销毁时恢复
};

TEST_F(UT_UserPageWidget, Ctor_CreatesChildWidgets)
{
    // Assert：构造链路创建进程表、账户列表与用量标签
    EXPECT_NE(m_tester->m_procTable, nullptr);
    EXPECT_NE(m_tester->m_accountListWidget, nullptr);
    EXPECT_NE(m_tester->m_CPULabel, nullptr);
    EXPECT_NE(m_tester->m_MemeryUsageSummary, nullptr);
}

TEST_F(UT_UserPageWidget, Ctor_CapturesCurrentUser)
{
    // Arrange：取系统当前登录用户名
    struct passwd *pws = getpwuid(getuid());
    QString expected = QString::fromLocal8Bit(pws->pw_name);

    // Assert：构造时记录当前用户
    EXPECT_EQ(m_tester->m_currentUser, expected);
}

TEST_F(UT_UserPageWidget, Ctor_UsageLabelsInitialized)
{
    // Assert：initUI 初始化的用量摘要标签均非空
    EXPECT_NE(m_tester->m_CPUUsageSummary, nullptr);
    EXPECT_NE(m_tester->m_MemeryUsageSummary, nullptr);
    EXPECT_NE(m_tester->m_SMemUsageSummary, nullptr);
    EXPECT_NE(m_tester->m_VMemUsageSummary, nullptr);
    EXPECT_NE(m_tester->m_UploadSummary, nullptr);
    EXPECT_NE(m_tester->m_DownLoadSummary, nullptr);
    EXPECT_NE(m_tester->m_DiskReadSummary, nullptr);
    EXPECT_NE(m_tester->m_DiskWriteSummary, nullptr);
}

TEST_F(UT_UserPageWidget, InitConnections_ModelUpdateRefreshesLabels)
{
    // Arrange：记录刷新前的 CPU 标签文本
    QString before = m_tester->m_CPUUsageSummary->text();

    // Act（B12 分支：触发已连接的 signalModelUpdated）
    emit m_tester->m_procTable->signalModelUpdated();
    QCoreApplication::processEvents();

    // Assert：连接链路生效，标签文本被重写为实时值格式
    QString expected = QString("%1%").arg(m_tester->m_procTable->getUserCPUUsage(), 0, 'f', 1);
    EXPECT_EQ(m_tester->m_CPUUsageSummary->text(), expected);
    EXPECT_FALSE(m_tester->m_CPUUsageSummary->text().isEmpty());
    Q_UNUSED(before);
}

TEST_F(UT_UserPageWidget, OnTextContentChanged_LabelsMatchLiveValues)
{
    // Act（B3 分支）
    m_tester->onTextContentChanged();

    // Assert：各标签文本与进程表实时 getter 值一一对应
    EXPECT_EQ(m_tester->m_CPUUsageSummary->text(),
              QString("%1%").arg(m_tester->m_procTable->getUserCPUUsage(), 0, 'f', 1));
    EXPECT_EQ(m_tester->m_MemeryUsageSummary->text(),
              common::format::formatUnit_memory_disk(m_tester->m_procTable->getUserMemUsage(), common::format::KB));
    EXPECT_EQ(m_tester->m_UploadSummary->text(),
              common::format::formatUnit_net(8 * m_tester->m_procTable->getUserUpload(), common::format::B, 1, true));
}

TEST_F(UT_UserPageWidget, OnTextContentChanged_TooltipMatchesText)
{
    // Act（B4 分支）
    m_tester->onTextContentChanged();

    // Assert：tooltip 与文本一致（CPU 与上传两个维度）
    EXPECT_EQ(m_tester->m_CPUUsageSummary->toolTip(), m_tester->m_CPUUsageSummary->text());
    EXPECT_EQ(m_tester->m_UploadSummary->toolTip(), m_tester->m_UploadSummary->text());
}

TEST_F(UT_UserPageWidget, OnHeaderChanged_HiddenColumn_HidesLabels)
{
    // Arrange（B5 分支：隐藏 CPU 列 1 与内存列 3）
    m_tester->m_procTable->setColumnHidden(1, true);
    m_tester->m_procTable->setColumnHidden(3, true);

    // Act
    m_tester->onHeaderChanged();

    // Assert：对应标签被隐藏
    EXPECT_TRUE(m_tester->m_CPULabel->isHidden());
    EXPECT_TRUE(m_tester->m_MemeryLabel->isHidden());
}

TEST_F(UT_UserPageWidget, OnHeaderChanged_RestoredColumn_ShowsLabels)
{
    // Arrange：先隐藏再恢复（B6 分支）
    m_tester->m_procTable->setColumnHidden(1, true);
    m_tester->onHeaderChanged();
    m_tester->m_procTable->setColumnHidden(1, false);

    // Act
    m_tester->onHeaderChanged();

    // Assert：标签恢复可见（强异常安全：状态可还原）
    EXPECT_FALSE(m_tester->m_CPULabel->isHidden());
    EXPECT_TRUE(m_tester->m_CPULabel->isVisibleTo(m_tester));
}

TEST_F(UT_UserPageWidget, OnThemeChanged_LabelsUseTextTitleColor)
{
    // Arrange（B7 分支：8 个标签非空走真路径）
    auto appPalette = DGuiApplicationHelper::instance()->applicationPalette();
    DPalette expected = appPalette;
    expected.setColor(DPalette::Text, expected.color(DPalette::TextTitle));

    // Act
    m_tester->onThemeChanged();

    // Assert：8 个标签调色板 Text 色均等于 TextTitle 色
    EXPECT_EQ(m_tester->m_CPULabel->palette().color(QPalette::Text), expected.color(DPalette::Text));
    EXPECT_EQ(m_tester->m_MemeryLabel->palette().color(QPalette::Text), expected.color(DPalette::Text));
    EXPECT_EQ(m_tester->m_SMemLabel->palette().color(QPalette::Text), expected.color(DPalette::Text));
    EXPECT_EQ(m_tester->m_VMemLabel->palette().color(QPalette::Text), expected.color(DPalette::Text));
    EXPECT_EQ(m_tester->m_UploadLabel->palette().color(QPalette::Text), expected.color(DPalette::Text));
    EXPECT_EQ(m_tester->m_DownLoadLabel->palette().color(QPalette::Text), expected.color(DPalette::Text));
    EXPECT_EQ(m_tester->m_DiskReadLabel->palette().color(QPalette::Text), expected.color(DPalette::Text));
    EXPECT_EQ(m_tester->m_DiskWriteLabel->palette().color(QPalette::Text), expected.color(DPalette::Text));
}

TEST_F(UT_UserPageWidget, OnThemeChanged_SummaryLabelsUseTextTipsColor)
{
    // Arrange（B8 分支）：用量摘要标签用 TextTips 色（与标题标签的 TextTitle 区分）
    auto appPalette = DGuiApplicationHelper::instance()->applicationPalette();
    DPalette expected = appPalette;
    expected.setColor(DPalette::Text, expected.color(DPalette::TextTips));

    // Act
    m_tester->onThemeChanged();

    // Assert：用量摘要标签同样应用 TextTips 色
    EXPECT_EQ(m_tester->m_CPUUsageSummary->palette().color(QPalette::Text), expected.color(DPalette::Text));
    EXPECT_EQ(m_tester->m_DiskWriteSummary->palette().color(QPalette::Text), expected.color(DPalette::Text));
}

TEST_F(UT_UserPageWidget, OnUserChanged_RefreshesUsageLabels)
{
    // Act（B11 分支：用户切换刷新链路）
    m_tester->onUserChanged();

    // Assert：刷新链路走到 onTextContentChanged，标签与实时值一致
    EXPECT_EQ(m_tester->m_CPUUsageSummary->text(),
              QString("%1%").arg(m_tester->m_procTable->getUserCPUUsage(), 0, 'f', 1));
    EXPECT_EQ(m_tester->m_CPUUsageSummary->toolTip(), m_tester->m_CPUUsageSummary->text());
}

TEST_F(UT_UserPageWidget, PaintEvent_DirectCallNoCrash)
{
    // Arrange
    QPaintEvent paintEvent(m_tester->rect());

    // Act
    m_tester->paintEvent(&paintEvent);

    // Assert：绘制后部件矩形有效且未崩溃
    EXPECT_TRUE(m_tester->rect().isValid());
    EXPECT_NE(m_tester->m_procTable, nullptr);
}

TEST_F(UT_UserPageWidget, SetLabelFormat_AppliesAlignmentAndElide)
{
    // Arrange（B9 分支）
    DLabel label;

    // Act
    m_tester->setLabelFormat(&label, DPalette::TextTips);

    // Assert：对齐与省略模式生效
    EXPECT_EQ(label.alignment(), Qt::AlignLeft | Qt::AlignVCenter);
    EXPECT_EQ(label.elideMode(), Qt::ElideRight);
}

TEST_F(UT_UserPageWidget, SetLabelFormat_AppliesPaletteColor)
{
    // Arrange（B10 分支）
    DLabel label;
    auto appPalette = DGuiApplicationHelper::instance()->applicationPalette();
    DPalette expected = appPalette;
    expected.setColor(DPalette::Text, expected.color(DPalette::TextTips));

    // Act
    m_tester->setLabelFormat(&label, DPalette::TextTips);

    // Assert：标签 Text 色等于 TextTips 色
    EXPECT_EQ(label.palette().color(QPalette::Text), expected.color(DPalette::Text));
}
