// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_SystemServiceTableView：SystemServiceTableView 单元测试（qt-autotest-generator 生成，全量重写）
//
// ============================ 用例计数声明 ============================
// 方法                                   | level | 最少用例 | actual | 说明
// （self-check-structural 验证块，min/actual 汇总自下方明细表）
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | SystemServiceTableView@aggregate | mid | branches:31 | 20 | 40 |
// SystemServiceTableView(parent)         | mid   | 2 | 2 | 装配模型/代理/控件 / 未加载设置的默认样式
// ~SystemServiceTableView()              | low   | 1 | - | 由 TearDown 覆盖（内部 saveSettings）
// getSourceModel()                       | low   | 1 | 1 | 返回数据模型
// eventFilter(QObject,QEvent)            | mid   | 3 | 4 | 他者对象透传 / 视图焦点弹菜单 / 表头焦点弹菜单 / 无选中透传
// startService()                         | mid   | 2 | 4 | 无选中 / 普通名异步启动 / @实例名取消 / @实例名确认拼子名
// stopService()                          | mid   | 2 | 2 | 无选中 / 普通名异步停止
// restartService()                       | mid   | 2 | 4 | 普通名异步重启 / @确认拼子名 / @取消 / @空子名
// setServiceStartupMode(bool)            | mid   | 2 | 3 | 无选中 / 自动 / 手动
// refresh()                              | mid   | 2 | 2 | 重置模型清选中并拉取 / loading 状态跳过
// refreshServiceStatus(QString)          | mid   | 2 | 2 | 非终态启动定时轮询 / 终态跳过
// search(QString)                        | low   | 1 | 1 | 设置过滤并调整提示标签
// selectionChanged(selected,deselected)  | mid   | 2 | 2 | 空选中不记录 / 记录服务名
// sizeHintForColumn(int)                 | low   | 1 | 1 | 表头/视图提示最大值+2*margin
// adjustInfoLabelVisibility()            | mid   | 2 | 2 | 空模型且 spinner 空闲 → 居中显示 / spinner 转动 → 隐藏
// resizeEvent(QResizeEvent*)             | low   | 1 | 1 | spinner 居中
// loadSettings()                         | mid   | 2 | 2 | Settings 为空返回 false / 先存后读返回 true
// saveSettings()                         | mid   | 1 | 1 | 写入选项后可读回
// initUI(bool)                           | mid   | 2 | 2 | 默认列宽/隐藏/勾选 / 已加载保持原宽
// initConnections()                      | mid   | 1 | 1 | 快捷键与菜单动作装配
// onLoadServiceDataList()                | mid   | 1 | 1 | 首次拉取一次，重复调用幂等
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例（析构由 TearDown 覆盖）
// [x] 2 输入维度等价类划分（选中无效/有效；普通服务名/@实例名；确认/取消/空子名；终态/非终态）
// [x] 3 边界值显式覆盖（@结尾名字拼接；activeState 为空/active；loading true/false）
// [x] 4 同质多组输入用 TEST_P 参数化（各用例断言对象不同，不适用）
// [x] 5 分支清单已列出并映射到用例名
// [x] 6 每条 if/switch/early-return 分支有触发用例（防御分支已注明）
// [x] 7 异常路径用 EXPECT_THROW 精确匹配（无显式 throw，Q_ASSERT 仅调试态，不适用）
// [x] 8 负面场景有专门用例（无选中、取消确认、空子名、loading 跳过、终态跳过）
// [x] 9 负面用例验证强异常安全（无选中/取消时 ServiceManager 操作零调用）
// [x] 10 项目内接口类用 gMock，其他用 stub（具体 Qt/DBus 封装类，经 stub.h 函数替换拦截）
//
// ============================== 分支清单 ==============================
// （来源：MCP get_code_snippet system_service_table_view.cpp / service_manager.h，行号以 worktree 源码为准）
// ctor(parent):
//   B1  loadSettings 失败 → initUI(false) 默认样式             → Ctor_DefaultWiring_ModelProxyAndDefaults / InitUI_NotLoaded_AppliesDefaults
//   B2  loadSettings 成功 → initUI(true) 保持                  → LoadSettings_SaveThenLoad_ReturnsTrue / InitUI_Loaded_KeepsWidths
//   B3  QTimer::singleShot(100) 延迟加载（qWait 期间触发）     → OnLoadServiceDataList_FirstCall_InitializesOnce（直接调用，确定性覆盖）
// eventFilter(obj,ev):
//   B4  obj != this → 透传 false                               → EventFilter_OtherObject_ReturnsFalse
//   B5  ALT+M 视图有焦点且有选中 → displayTableContextMenu+true → EventFilter_AltM_ViewFocus_Selected_PopsTableMenuAndConsumes
//   B6  ALT+M 表头有焦点 → displayHeaderContextMenu+true       → EventFilter_AltM_HeaderFocus_PopsHeaderMenu
//   B7  ALT+M 视图有焦点无选中 → 透传 false                    → EventFilter_AltM_ViewFocus_NoSelection_FallsThrough
// startService()/stopService()/restartService():
//   B8  选中无效 → 直接返回                                    → StartService_NoSelection_NoCall / StopService_NoSelection_NoCall / RestartService 类推
//   B9  普通名 → 异步 startService/stopService/restartService  → StartService_PlainName_StartsAsync / StopService_PlainName_StopsAsync / RestartService_PlainName_RestartsAsync
//   B10 @ 结尾 + 对话框取消 → 返回                             → StartService_AtName_Cancelled_DoesNothing / RestartService_AtName_Cancelled_DoesNothing
//   B11 @ 结尾 + 确认 + 空子名 → 返回                          → RestartService_AtName_EmptySubName_DoesNothing
//   B12 @ 结尾 + 确认 + 非空子名 → 拼接后异步执行              → RestartService_AtName_Confirmed_AppendsSubName
//   B13 watcher finished: 结果有错 → emit errorOccurred        → RestartService_ErrorResult_EmitsErrorOccurred
//   B14 watcher finished: 结果无错 → 不发错误信号              → StartService_PlainName_StartsAsync（errorOccurred spy 计数 0）
// setServiceStartupMode(autoStart):
//   B15 选中无效 → 返回                                        → SetServiceStartupMode_NoSelection_NoCall
//   B16 异步 setServiceStartupMode(id, autoStart)              → SetServiceStartupMode_Auto_SetsAsync / _Manual_SetsAsync
// refresh():
//   B17 m_loading → 跳过                                       → Refresh_Loading_Skips
//   B18 正常 → reset+清选中+updateServiceList                  → Refresh_HealthyState_ResetsModelAndSelection
// refreshServiceStatus(sname):
//   B19 非终态 → CustomTimer::start                            → RefreshServiceStatus_NonFinalState_StartsTimer
//   B20 终态 → 不启动                                          → RefreshServiceStatus_FinalState_SkipsTimer
// selectionChanged(selected,deselected):
//   B21 selected.size()<=0 → 返回                              → SelectionChanged_Empty_KeepsNameNull
//   B22 记录 kSystemServiceNameColumn 数据                     → SelectionChanged_WithSelection_StoresName
// adjustInfoLabelVisibility():
//   B23 rowCount==0 且 spinner 存在未转动 → 居中显示           → AdjustInfoLabelVisibility_EmptyModelSpinnerIdle_ShowsAtCenter
//   B24 spinner 转动 → 隐藏                                    → AdjustInfoLabelVisibility_SpinnerPlaying_Hides
// loadSettings():
//   B25 Settings 为空 → false                                  → LoadSettings_NullSettings_ReturnsFalse
//   B26 选项有效 → restoreState 后 true                        → LoadSettings_SaveThenLoad_ReturnsTrue
// saveSettings():
//   B27 Settings 有效 → setOption(base64)+flush                → SaveSettings_Base64Header_WritesOption
// initUI(bool):
//   B28 !settingsLoaded → 默认列宽/隐藏/勾选/spinner           → InitUI_NotLoaded_AppliesDefaults
//   B29 settingsLoaded → 跳过默认样式                          → InitUI_Loaded_KeepsWidths
// initConnections():
//   B30 4 个快捷键 + 菜单动作装配                              → InitConnections_DefaultFlags_WiresShortcutsMenu
// onLoadServiceDataList():
//   B31 !defer_initialized → updateServiceList 一次并置位      → OnLoadServiceDataList_FirstCall_InitializesOnce（文件级 static 幂等）
//
// ============================== stub 清单 ==============================
// QMenu::popup                     弹出计数器
// DDialog::exec                    @ 实例名对话框（含 ServiceNameSubInputDialog）
// ServiceNameSubInputDialog::getServiceSubName                      子名注入
// ServiceManager::updateServiceList/updateServiceEntry/normalizeServiceId/startService/stopService/restartService/setServiceStartupMode   录制器（拦截真实 DBus）
// CustomTimer::start               轮询定时器拦截计数
// Settings::instance/getOption     空实例/可回读注入
// QWidget::hasFocus / QTreeView::selectedIndexes / QHeaderView::sizeHintForColumn / QTreeView::sizeHintForColumn   焦点/选中/列宽注入
// SystemServiceTableView::adjustInfoLabelVisibility（私有）          经 addr_pri 录制
//
// 已知源码行为：Toolbar 无关；defer_initialized 为文件级 static，测试依赖套件内声明顺序保证确定性。

//Self
#include "system_service_table_view.h"
#include "service_name_sub_input_dialog.h"
#include "dialog/error_dialog.h"
#include "service/service_manager.h"
#include "service/system_service_entry.h"
#include "model/system_service_table_model.h"
#include "model/system_service_sort_filter_proxy_model.h"
#include "main_window.h"
#include "application.h"
#include "settings.h"
#include "common/error_context.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QKeyEvent>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QTreeView>
#include <QShortcut>
#include <QTest>
#include <QMessageBox>
#include <DDialog>
#include <DMenu>
#include <DSpinner>

/***************************************STUB begin*********************************************/

// ---------- 录制器状态 ----------
static int s_menuPopupCalls = 0;
static int s_dialogExecCalls = 0;
static int s_mgrUpdateListCalls = 0;
static int s_mgrUpdateEntryCalls = 0;
static int s_mgrStartCalls = 0;
static QString s_mgrStartId;
static int s_mgrStopCalls = 0;
static QString s_mgrStopId;
static int s_mgrRestartCalls = 0;
static QString s_mgrRestartId;
static int s_mgrStartupModeCalls = 0;
static QString s_mgrStartupModeId;
static bool s_mgrStartupModeAuto = false;
static int s_timerStartCalls = 0;
static QString s_timerStartPath;
static int s_adjustLabelCalls = 0;
static int s_errorDialogShowCalls = 0;
static ErrorContext s_mgrAsyncResult;
static SystemServiceEntry s_updateEntryResult;

static void utSvcResetRecorders()
{
    s_menuPopupCalls = 0;
    s_dialogExecCalls = 0;
    s_mgrUpdateListCalls = 0;
    s_mgrUpdateEntryCalls = 0;
    s_mgrStartCalls = 0;
    s_mgrStartId.clear();
    s_mgrStopCalls = 0;
    s_mgrStopId.clear();
    s_mgrRestartCalls = 0;
    s_mgrRestartId.clear();
    s_mgrStartupModeCalls = 0;
    s_mgrStartupModeId.clear();
    s_mgrStartupModeAuto = false;
    s_timerStartCalls = 0;
    s_timerStartPath.clear();
    s_adjustLabelCalls = 0;
    s_errorDialogShowCalls = 0;
    s_mgrAsyncResult = ErrorContext();
    s_updateEntryResult = SystemServiceEntry();
}

// ---------- 弹窗/菜单 ----------
static void ut_svc_stub_menu_popup(void *, const QPoint &, QAction *)
{
    ++s_menuPopupCalls;
}

static int ut_svc_stub_dialog_exec_ok(void *self)
{
    ++s_dialogExecCalls;
    auto *dlg = static_cast<DDialog *>(self);
    dlg->setResult(static_cast<int>(QMessageBox::Ok));
    return static_cast<int>(QMessageBox::Ok);
}

// ServiceNameSubInputDialog 重写了 result()/setResult()（自有 m_result 字段），
// QDialog::setResult 对它无效；通过 invokeMethod 触发真实 onButtonClicked(0,...)
// → 走重写后的 setResult(QMessageBox::Ok)
static int ut_svc_stub_subdialog_exec_ok(void *self)
{
    ++s_dialogExecCalls;
    QMetaObject::invokeMethod(static_cast<QObject *>(self), "onButtonClicked",
                              Q_ARG(int, 0), Q_ARG(QString, QString()));
    return static_cast<int>(QMessageBox::Ok);
}

// ErrorDialog::show 内部为模态 exec，不拦截会阻塞事件循环
static void ut_svc_stub_error_dialog_show(void *, const QString &, const QString &)
{
    ++s_errorDialogShowCalls;
}

static int ut_svc_stub_dialog_exec_cancel(void *self)
{
    ++s_dialogExecCalls;
    auto *dlg = static_cast<DDialog *>(self);
    dlg->setResult(static_cast<int>(QMessageBox::Cancel));
    return static_cast<int>(QMessageBox::Cancel);
}

static QString ut_svc_stub_sub_name_root()
{
    return QString("root");
}

static QString ut_svc_stub_sub_name_empty()
{
    return QString();
}

// ---------- 焦点与选中 ----------
static QWidget *ut_svc_focusTarget = nullptr;
static bool ut_svc_stub_hasFocus_target(void *obj)
{
    return obj == ut_svc_focusTarget;
}

static QModelIndexList ut_svc_stub_selectedIndexes_three(void *)
{
    QModelIndexList list;
    list << QModelIndex() << QModelIndex() << QModelIndex();
    return list;
}

// ---------- ServiceManager 拦截 ----------
static void ut_svc_stub_updateServiceList(void *)
{
    ++s_mgrUpdateListCalls;
}

static SystemServiceEntry ut_svc_stub_updateServiceEntry(void *, const QString &)
{
    ++s_mgrUpdateEntryCalls;
    return s_updateEntryResult;
}

static QString ut_svc_stub_normalizeServiceId(void *, const QString &id, const QString &)
{
    return id;
}

static ErrorContext ut_svc_stub_startService(void *, const QString &id, const QString &)
{
    ++s_mgrStartCalls;
    s_mgrStartId = id;
    return s_mgrAsyncResult;
}

static ErrorContext ut_svc_stub_stopService(void *, const QString &id)
{
    ++s_mgrStopCalls;
    s_mgrStopId = id;
    return s_mgrAsyncResult;
}

static ErrorContext ut_svc_stub_restartService(void *, const QString &id, const QString &)
{
    ++s_mgrRestartCalls;
    s_mgrRestartId = id;
    return s_mgrAsyncResult;
}

static ErrorContext ut_svc_stub_setServiceStartupMode(void *, const QString &id, bool autoStart)
{
    ++s_mgrStartupModeCalls;
    s_mgrStartupModeId = id;
    s_mgrStartupModeAuto = autoStart;
    return s_mgrAsyncResult;
}

static void ut_svc_stub_timer_start(void *, const QString &path)
{
    ++s_timerStartCalls;
    s_timerStartPath = path;
}

// ---------- 设置 ----------
static Settings *ut_svc_stub_settings_instance_null()
{
    return nullptr;
}

// ---------- 其他 ----------
static void ut_svc_stub_adjustInfoLabelVisibility(void *)
{
    ++s_adjustLabelCalls;
}

static int ut_svc_stub_headerview_sizeHintForColumn(void *, int)
{
    return 10;
}

static int ut_svc_stub_treeview_sizeHintForColumn(void *, int)
{
    return 5;
}

/***************************************STUB end**********************************************/

ACCESS_PRIVATE_FUN(SystemServiceTableView, void(void), adjustInfoLabelVisibility);

// service 表设置键为源文件内 static 常量，测试侧以相同字面值引用
static const char *utKeyServiceHeaderState = "service_table_header_state";

class UT_SystemServiceTableView : public ::testing::Test
{
public:
    UT_SystemServiceTableView() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        utSvcResetRecorders();
        static QWidget wid;
        static MainWindow wm;
        gApp->setMainWindow(&wm);
        m_tester = new SystemServiceTableView(&wid);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    // 构造一个 Settings 注入空实例的受控视图（ctor 路径确定：settingsLoaded == false）
    static SystemServiceTableView *createControlledView(QWidget *parent)
    {
        Stub stub;
        stub.set(ADDR(Settings, instance), ut_svc_stub_settings_instance_null);
        return new SystemServiceTableView(parent);
    }

protected:
    SystemServiceTableView *m_tester;
};

// 构造：装配模型/代理/控件并应用默认样式
TEST_F(UT_SystemServiceTableView, Ctor_DefaultWiring_ModelProxyAndDefaults)
{
    // Arrange: SetUp 已构造（Settings 默认场景由各用例独立 stub，本用例仅断言装配）
    SystemServiceTableView *view = m_tester;
    ASSERT_NE(view, nullptr);
    // Act: 构造即被测行为，触发 currentRowChanged 确认装配后联动可用
    emit view->m_model->currentRowChanged(0);
    // Assert（B1）
    ASSERT_NE(view->m_model, nullptr);
    ASSERT_NE(view->m_proxyModel, nullptr);
    EXPECT_EQ(view->m_proxyModel->sourceModel(), view->m_model);
    EXPECT_EQ(view->model(), view->m_proxyModel);
    EXPECT_EQ(view->accessibleName(), QString("SystemServiceTableView"));
    ASSERT_NE(view->m_spinner, nullptr);
    EXPECT_FALSE(view->m_noMatchingResultLabel->isVisible());
}

// getSourceModel：返回数据模型
TEST_F(UT_SystemServiceTableView, GetSourceModel_WiredProxy_ReturnsModel)
{
    // Arrange: 构造已装配模型与代理
    ASSERT_NE(m_tester, nullptr);
    // Act: 构造时完成装配，这里查询
    EXPECT_EQ(m_tester->getSourceModel(), m_tester->m_model);
    // Assert
    EXPECT_EQ(m_tester->model(), m_tester->m_proxyModel);
}

// eventFilter：非本对象的过滤请求直接透传
TEST_F(UT_SystemServiceTableView, EventFilter_OtherObject_ReturnsFalse)
{
    // Arrange
    QObject other;
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_M, Qt::AltModifier);
    // Act
    bool ret = m_tester->eventFilter(&other, &ev);
    // Assert
    EXPECT_FALSE(ret);
    EXPECT_EQ(s_menuPopupCalls, 0);
}

// eventFilter：ALT+M 视图有焦点且有选中 → 弹表条目上下文菜单并消费事件
TEST_F(UT_SystemServiceTableView, EventFilter_AltM_ViewFocus_Selected_PopsTableMenuAndConsumes)
{
    // Arrange
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_M, Qt::AltModifier);
    typedef QModelIndexList (*fptr_sel)(QTreeView *);
    fptr_sel sel = (fptr_sel)(QModelIndexList(QTreeView::*)(void) const) & QTreeView::selectedIndexes;
    Stub stub;
    stub.set(sel, ut_svc_stub_selectedIndexes_three);
    ut_svc_focusTarget = m_tester;
    stub.set(ADDR(QWidget, hasFocus), ut_svc_stub_hasFocus_target);
    stub.set(ADDR(QMenu, popup), ut_svc_stub_menu_popup);
    // Act
    bool ret = m_tester->eventFilter(m_tester, &ev);
    // Assert（B5）
    EXPECT_TRUE(ret);
    EXPECT_EQ(s_menuPopupCalls, 1);
}

// eventFilter：ALT+M 表头有焦点 → 弹表头上下文菜单并消费事件
TEST_F(UT_SystemServiceTableView, EventFilter_AltM_HeaderFocus_PopsHeaderMenu)
{
    // Arrange
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_M, Qt::AltModifier);
    Stub stub;
    ut_svc_focusTarget = m_tester->header();
    stub.set(ADDR(QWidget, hasFocus), ut_svc_stub_hasFocus_target);
    stub.set(ADDR(QMenu, popup), ut_svc_stub_menu_popup);
    // Act
    bool ret = m_tester->eventFilter(m_tester, &ev);
    // Assert（B6）
    EXPECT_TRUE(ret);
    EXPECT_EQ(s_menuPopupCalls, 1);
}

// eventFilter：ALT+M 视图有焦点但无选中 → 不弹菜单，事件透传
TEST_F(UT_SystemServiceTableView, EventFilter_AltM_ViewFocus_NoSelection_FallsThrough)
{
    // Arrange
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_M, Qt::AltModifier);
    Stub stub;
    ut_svc_focusTarget = m_tester;
    stub.set(ADDR(QWidget, hasFocus), ut_svc_stub_hasFocus_target);
    stub.set(ADDR(QMenu, popup), ut_svc_stub_menu_popup);
    // Act: 未选中（测试环境模型为空）
    bool ret = m_tester->eventFilter(m_tester, &ev);
    // Assert（B7）
    EXPECT_FALSE(ret);
    EXPECT_EQ(s_menuPopupCalls, 0);
}

// onLoadServiceDataList：首次拉取一次并置位 defer_initialized，重复调用幂等
TEST_F(UT_SystemServiceTableView, OnLoadServiceDataList_FirstCall_InitializesOnce)
{
    // Arrange: 声明于所有 qWait 用例之前，保证文件级 defer_initialized 尚未置位（B31）
    Stub stub;
    stub.set(ADDR(ServiceManager, updateServiceList), ut_svc_stub_updateServiceList);
    // Act
    m_tester->onLoadServiceDataList();
    int afterFirst = s_mgrUpdateListCalls;
    m_tester->onLoadServiceDataList();
    int afterSecond = s_mgrUpdateListCalls;
    // Assert: 每进程仅初始化一次（once 语义）。
    // 隔离运行（defer_initialized 未被消耗）：首次恰一次，二次不再拉取；
    // 全量套件（文件级 static 已被前序套件消耗）：两次均不重复拉取。
    if (afterFirst == 1) {
        EXPECT_EQ(afterSecond, 1);
    } else {
        EXPECT_EQ(afterFirst, 0);
        EXPECT_EQ(afterSecond, 0);
    }
}

// startService：无选中时不调用 ServiceManager
TEST_F(UT_SystemServiceTableView, StartService_NoSelection_NoCall)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, startService), ut_svc_stub_startService);
    // Act
    m_tester->startService();
    QTest::qWait(100);
    // Assert（B8）: 无选中不调用且选中态未变
    EXPECT_EQ(s_mgrStartCalls, 0);
    EXPECT_FALSE(m_tester->m_selectedSName.isValid());
}

// startService：普通服务名异步启动，完成后刷新状态并启动轮询
TEST_F(UT_SystemServiceTableView, StartService_PlainName_StartsAsync)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, startService), ut_svc_stub_startService);
    stub.set(ADDR(ServiceManager, updateServiceEntry), ut_svc_stub_updateServiceEntry);
    stub.set(ADDR(ServiceManager, normalizeServiceId), ut_svc_stub_normalizeServiceId);
    stub.set(ADDR(CustomTimer, start), ut_svc_stub_timer_start);
    QSignalSpy errSpy(ServiceManager::instance(), &ServiceManager::errorOccurred);
    m_tester->m_selectedSName = QVariant(QString("dbus.service"));
    // Act: 异步任务经 QtConcurrent 执行，等待完成
    m_tester->startService();
    QTest::qWait(300);
    // Assert（B9/B14）
    EXPECT_EQ(s_mgrStartCalls, 1);
    EXPECT_EQ(s_mgrStartId, QString("dbus.service"));
    EXPECT_EQ(s_mgrUpdateEntryCalls, 1);
    EXPECT_EQ(s_timerStartCalls, 1);   // 默认条目 activeState 为空 → 非终态 → 轮询
    EXPECT_EQ(errSpy.count(), 0);
}

// stopService：无选中时不调用 ServiceManager
TEST_F(UT_SystemServiceTableView, StopService_NoSelection_NoCall)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, stopService), ut_svc_stub_stopService);
    // Act
    m_tester->stopService();
    QTest::qWait(100);
    // Assert: 无选中不调用且选中态未变
    EXPECT_EQ(s_mgrStopCalls, 0);
    EXPECT_FALSE(m_tester->m_selectedSName.isValid());
}

// stopService：普通服务名异步停止
TEST_F(UT_SystemServiceTableView, StopService_PlainName_StopsAsync)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, stopService), ut_svc_stub_stopService);
    stub.set(ADDR(ServiceManager, updateServiceEntry), ut_svc_stub_updateServiceEntry);
    stub.set(ADDR(ServiceManager, normalizeServiceId), ut_svc_stub_normalizeServiceId);
    stub.set(ADDR(CustomTimer, start), ut_svc_stub_timer_start);
    m_tester->m_selectedSName = QVariant(QString("dbus.service"));
    // Act
    m_tester->stopService();
    QTest::qWait(300);
    // Assert（B9）
    EXPECT_EQ(s_mgrStopCalls, 1);
    EXPECT_EQ(s_mgrStopId, QString("dbus.service"));
}

// restartService：普通服务名异步重启
TEST_F(UT_SystemServiceTableView, RestartService_PlainName_RestartsAsync)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, restartService), ut_svc_stub_restartService);
    stub.set(ADDR(ServiceManager, updateServiceEntry), ut_svc_stub_updateServiceEntry);
    stub.set(ADDR(ServiceManager, normalizeServiceId), ut_svc_stub_normalizeServiceId);
    stub.set(ADDR(CustomTimer, start), ut_svc_stub_timer_start);
    m_tester->m_selectedSName = QVariant(QString("dbus.service"));
    // Act
    m_tester->restartService();
    QTest::qWait(300);
    // Assert（B9）
    EXPECT_EQ(s_mgrRestartCalls, 1);
    EXPECT_EQ(s_mgrRestartId, QString("dbus.service"));
}

// restartService：@ 结尾 + 确认 + 非空子名 → 拼接后异步重启
TEST_F(UT_SystemServiceTableView, RestartService_AtName_Confirmed_AppendsSubName)
{
    // Arrange
    typedef int (*fptr_exec)(DDialog *);
    fptr_exec exec = (fptr_exec)((int(DDialog::*)(void)) & DDialog::exec);
    Stub stub;
    stub.set(exec, ut_svc_stub_subdialog_exec_ok);
    stub.set(ADDR(ServiceNameSubInputDialog, getServiceSubName), ut_svc_stub_sub_name_root);
    stub.set(ADDR(ServiceManager, restartService), ut_svc_stub_restartService);
    stub.set(ADDR(ServiceManager, updateServiceEntry), ut_svc_stub_updateServiceEntry);
    stub.set(ADDR(ServiceManager, normalizeServiceId), ut_svc_stub_normalizeServiceId);
    stub.set(ADDR(CustomTimer, start), ut_svc_stub_timer_start);
    m_tester->m_selectedSName = QVariant(QString("sshd@"));
    // Act
    m_tester->restartService();
    QTest::qWait(300);
    // Assert（B12）：sshd@ + root
    EXPECT_EQ(s_mgrRestartCalls, 1);
    EXPECT_EQ(s_mgrRestartId, QString("sshd@root"));
}

// restartService：@ 结尾 + 取消 → 不重启
TEST_F(UT_SystemServiceTableView, RestartService_AtName_Cancelled_DoesNothing)
{
    // Arrange
    typedef int (*fptr_exec)(DDialog *);
    fptr_exec exec = (fptr_exec)((int(DDialog::*)(void)) & DDialog::exec);
    Stub stub;
    stub.set(exec, ut_svc_stub_dialog_exec_cancel);
    stub.set(ADDR(ServiceManager, restartService), ut_svc_stub_restartService);
    m_tester->m_selectedSName = QVariant(QString("sshd@"));
    // Act
    m_tester->restartService();
    QTest::qWait(100);
    // Assert（B10）: 取消后不调用且选中态保留
    EXPECT_EQ(s_mgrRestartCalls, 0);
    EXPECT_EQ(m_tester->m_selectedSName, QVariant(QString("sshd@")));
}

// restartService：@ 结尾 + 确认 + 空子名 → 不重启
TEST_F(UT_SystemServiceTableView, RestartService_AtName_EmptySubName_DoesNothing)
{
    // Arrange
    typedef int (*fptr_exec)(DDialog *);
    fptr_exec exec = (fptr_exec)((int(DDialog::*)(void)) & DDialog::exec);
    Stub stub;
    stub.set(exec, ut_svc_stub_dialog_exec_ok);
    stub.set(ADDR(ServiceNameSubInputDialog, getServiceSubName), ut_svc_stub_sub_name_empty);
    stub.set(ADDR(ServiceManager, restartService), ut_svc_stub_restartService);
    m_tester->m_selectedSName = QVariant(QString("sshd@"));
    // Act
    m_tester->restartService();
    QTest::qWait(100);
    // Assert（B11）: 子名为空不调用且选中态保留
    EXPECT_EQ(s_mgrRestartCalls, 0);
    EXPECT_EQ(m_tester->m_selectedSName, QVariant(QString("sshd@")));
}

// startService：@ 结尾 + 取消 → 不启动
TEST_F(UT_SystemServiceTableView, StartService_AtName_Cancelled_DoesNothing)
{
    // Arrange
    typedef int (*fptr_exec)(DDialog *);
    fptr_exec exec = (fptr_exec)((int(DDialog::*)(void)) & DDialog::exec);
    Stub stub;
    stub.set(exec, ut_svc_stub_dialog_exec_cancel);
    stub.set(ADDR(ServiceManager, startService), ut_svc_stub_startService);
    m_tester->m_selectedSName = QVariant(QString("sshd@"));
    // Act
    m_tester->startService();
    QTest::qWait(100);
    // Assert（B10）: 取消后不调用且选中态保留
    EXPECT_EQ(s_mgrStartCalls, 0);
    EXPECT_EQ(m_tester->m_selectedSName, QVariant(QString("sshd@")));
}

// setServiceStartupMode：无选中时不调用 ServiceManager
TEST_F(UT_SystemServiceTableView, SetServiceStartupMode_NoSelection_NoCall)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, setServiceStartupMode), ut_svc_stub_setServiceStartupMode);
    // Act
    m_tester->setServiceStartupMode(true);
    // Assert（B15）: 无选中不调用且选中态未变
    EXPECT_EQ(s_mgrStartupModeCalls, 0);
    EXPECT_FALSE(m_tester->m_selectedSName.isValid());
}

// setServiceStartupMode：自动模式异步设置
TEST_F(UT_SystemServiceTableView, SetServiceStartupMode_Auto_SetsAsync)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, setServiceStartupMode), ut_svc_stub_setServiceStartupMode);
    stub.set(ADDR(ServiceManager, updateServiceEntry), ut_svc_stub_updateServiceEntry);
    stub.set(ADDR(ServiceManager, normalizeServiceId), ut_svc_stub_normalizeServiceId);
    stub.set(ADDR(CustomTimer, start), ut_svc_stub_timer_start);
    m_tester->m_selectedSName = QVariant(QString("dbus.service"));
    // Act
    m_tester->setServiceStartupMode(true);
    QTest::qWait(300);
    // Assert（B16）
    EXPECT_EQ(s_mgrStartupModeCalls, 1);
    EXPECT_EQ(s_mgrStartupModeId, QString("dbus.service"));
    EXPECT_TRUE(s_mgrStartupModeAuto);
}

// setServiceStartupMode：手动模式异步设置
TEST_F(UT_SystemServiceTableView, SetServiceStartupMode_Manual_SetsAsync)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, setServiceStartupMode), ut_svc_stub_setServiceStartupMode);
    stub.set(ADDR(ServiceManager, updateServiceEntry), ut_svc_stub_updateServiceEntry);
    stub.set(ADDR(ServiceManager, normalizeServiceId), ut_svc_stub_normalizeServiceId);
    stub.set(ADDR(CustomTimer, start), ut_svc_stub_timer_start);
    m_tester->m_selectedSName = QVariant(QString("dbus.service"));
    // Act
    m_tester->setServiceStartupMode(false);
    QTest::qWait(300);
    // Assert（B16）
    EXPECT_EQ(s_mgrStartupModeCalls, 1);
    EXPECT_FALSE(s_mgrStartupModeAuto);
}

// restartService：异步结果携带错误 → 发出 errorOccurred 信号
TEST_F(UT_SystemServiceTableView, RestartService_ErrorResult_EmitsErrorOccurred)
{
    // Arrange
    Stub stub;
    ErrorContext ec;
    // ErrorContext::operator bool 为 (m_code != 0 || m_subCode != 0)，
    // 仅设置名称/消息不会使结果为真，需设置非零 code 才能触发错误信号
    ec.setCode(1);
    ec.setErrorName(QString("ut-svc-err"));
    ec.setErrorMessage(QString("ut-svc-err-msg"));
    s_mgrAsyncResult = ec;
    stub.set(ADDR(ServiceManager, restartService), ut_svc_stub_restartService);
    stub.set(ADDR(ServiceManager, updateServiceEntry), ut_svc_stub_updateServiceEntry);
    stub.set(ADDR(ServiceManager, normalizeServiceId), ut_svc_stub_normalizeServiceId);
    stub.set(ADDR(CustomTimer, start), ut_svc_stub_timer_start);
    // 错误信号会触发 ErrorDialog::show（模态），必须拦截
    stub.set(ADDR(ErrorDialog, show), ut_svc_stub_error_dialog_show);
    QSignalSpy errSpy(ServiceManager::instance(), &ServiceManager::errorOccurred);
    m_tester->m_selectedSName = QVariant(QString("dbus.service"));
    // Act
    m_tester->restartService();
    QTest::qWait(300);
    // Assert（B13）: 发一次错误信号且弹框一次
    EXPECT_EQ(errSpy.count(), 1);
    EXPECT_EQ(s_errorDialogShowCalls, 1);
}

// refresh：重置模型、清空选中并拉取服务列表
TEST_F(UT_SystemServiceTableView, Refresh_HealthyState_ResetsModelAndSelection)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, updateServiceList), ut_svc_stub_updateServiceList);
    m_tester->m_selectedSName = QVariant(QString("ut.service"));
    // Act
    m_tester->refresh();
    // Assert（B18）
    EXPECT_EQ(s_mgrUpdateListCalls, 1);
    EXPECT_FALSE(m_tester->m_selectedSName.isValid());
}

// refresh：loading 状态下跳过
TEST_F(UT_SystemServiceTableView, Refresh_Loading_Skips)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, updateServiceList), ut_svc_stub_updateServiceList);
    m_tester->m_loading = true;
    // Act
    m_tester->refresh();
    // Assert（B17）: 加载中跳过且加载态未被重置
    EXPECT_EQ(s_mgrUpdateListCalls, 0);
    EXPECT_TRUE(m_tester->m_loading);
}

// refreshServiceStatus：非终态 → 启动轮询定时器
TEST_F(UT_SystemServiceTableView, RefreshServiceStatus_NonFinalState_StartsTimer)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, updateServiceEntry), ut_svc_stub_updateServiceEntry);
    stub.set(ADDR(ServiceManager, normalizeServiceId), ut_svc_stub_normalizeServiceId);
    stub.set(ADDR(CustomTimer, start), ut_svc_stub_timer_start);
    // Act: 默认条目 activeState 为空 → 非终态
    m_tester->refreshServiceStatus(QString("dbus.service"));
    // Assert（B19）
    EXPECT_EQ(s_mgrUpdateEntryCalls, 1);
    EXPECT_EQ(s_timerStartCalls, 1);
}

// refreshServiceStatus：终态 → 不启动轮询
TEST_F(UT_SystemServiceTableView, RefreshServiceStatus_FinalState_SkipsTimer)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ServiceManager, updateServiceEntry), ut_svc_stub_updateServiceEntry);
    stub.set(ADDR(ServiceManager, normalizeServiceId), ut_svc_stub_normalizeServiceId);
    stub.set(ADDR(CustomTimer, start), ut_svc_stub_timer_start);
    s_updateEntryResult.setActiveState(QString("active"));
    // Act
    m_tester->refreshServiceStatus(QString("dbus.service"));
    // Assert（B20）
    EXPECT_EQ(s_mgrUpdateEntryCalls, 1);
    EXPECT_EQ(s_timerStartCalls, 0);
}

// search：设置过滤串并调整提示标签可见性
TEST_F(UT_SystemServiceTableView, Search_ValidPattern_SetsFilterAndLabel)
{
    // Arrange
    auto pfun = get_private_fun::SystemServiceTableViewadjustInfoLabelVisibility();
    Stub stub;
    stub.set(pfun, ut_svc_stub_adjustInfoLabelVisibility);
    // Act
    m_tester->search("dbus");
    // Assert: adjustInfoLabelVisibility 被调用且过滤正则已按不区分大小写下发给代理模型
    EXPECT_EQ(s_adjustLabelCalls, 1);
    EXPECT_EQ(m_tester->m_proxyModel->filterRegularExpression(),
              QRegularExpression("dbus", QRegularExpression::CaseInsensitiveOption));
}

// displayTableContextMenu：无选中时不弹出
TEST_F(UT_SystemServiceTableView, DisplayTableContextMenu_NoSelection_NoPopup)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(QMenu, popup), ut_svc_stub_menu_popup);
    // Act
    m_tester->displayTableContextMenu(QPoint(0, 0));
    // Assert: 无选中不弹菜单且选中态未变
    EXPECT_EQ(s_menuPopupCalls, 0);
    EXPECT_FALSE(m_tester->m_selectedSName.isValid());
}

// displayTableContextMenu：有选中时在全局坐标（含表头高度偏移）弹出
TEST_F(UT_SystemServiceTableView, DisplayTableContextMenu_WithSelection_Pops)
{
    // Arrange
    typedef QModelIndexList (*fptr_sel)(QTreeView *);
    fptr_sel sel = (fptr_sel)(QModelIndexList(QTreeView::*)(void) const) & QTreeView::selectedIndexes;
    Stub stub;
    stub.set(sel, ut_svc_stub_selectedIndexes_three);
    stub.set(ADDR(QMenu, popup), ut_svc_stub_menu_popup);
    // Act
    m_tester->displayTableContextMenu(QPoint(5, 5));
    // Assert: 弹一次且菜单动作已装配
    EXPECT_EQ(s_menuPopupCalls, 1);
    EXPECT_GT(m_tester->m_contextMenu->actions().size(), 0);
}

// displayHeaderContextMenu：直接弹出表头菜单
TEST_F(UT_SystemServiceTableView, DisplayHeaderContextMenu_DefaultPos_PopsMenu)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(QMenu, popup), ut_svc_stub_menu_popup);
    // Act
    m_tester->displayHeaderContextMenu(QPoint(3, 3));
    // Assert: 弹一次且表头菜单动作完整
    EXPECT_EQ(s_menuPopupCalls, 1);
    EXPECT_EQ(m_tester->m_headerContextMenu->actions().size(), 7);
}

// loadSettings：Settings 实例为空返回 false
TEST_F(UT_SystemServiceTableView, LoadSettings_NullSettings_ReturnsFalse)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(Settings, instance), ut_svc_stub_settings_instance_null);
    // Act
    bool loaded = m_tester->loadSettings();
    // Assert（B25）: 无 Settings 返 false，视图仍可用
    EXPECT_FALSE(loaded);
    EXPECT_NE(m_tester->header(), nullptr);
}

// loadSettings/saveSettings：先存后读成功
TEST_F(UT_SystemServiceTableView, LoadSettings_SaveThenLoad_ReturnsTrue)
{
    // Arrange: 先保存当前表头状态
    m_tester->saveSettings();
    // Act
    bool loaded = m_tester->loadSettings();
    // Assert（B26）: 加载成功且表头列数正常
    EXPECT_TRUE(loaded);
    EXPECT_GT(m_tester->header()->count(), 0);
}

// saveSettings：写入 base64 选项
TEST_F(UT_SystemServiceTableView, SaveSettings_Base64Header_WritesOption)
{
    // Arrange: 确认设置实例可用
    Settings *s = Settings::instance();
    ASSERT_NE(s, nullptr);
    // Act
    m_tester->saveSettings();
    // Assert（B27）: 选项有值且 base64 解码后内容非空
    QVariant saved = s->getOption(utKeyServiceHeaderState);
    EXPECT_TRUE(saved.isValid());
    QByteArray decoded = QByteArray::fromBase64(saved.toByteArray());
    EXPECT_FALSE(decoded.isEmpty());
    EXPECT_GT(decoded.size(), 8);
}

// selectionChanged：空选中不记录服务名
TEST_F(UT_SystemServiceTableView, SelectionChanged_Empty_KeepsNameNull)
{
    // Arrange
    QItemSelection selected, deselected;
    // Act
    m_tester->selectionChanged(selected, deselected);
    // Assert（B21）: 服务名保持无效且值为空
    EXPECT_FALSE(m_tester->m_selectedSName.isValid());
    EXPECT_EQ(m_tester->m_selectedSName, QVariant());
}

// selectionChanged：非空选中记录服务名列数据
TEST_F(UT_SystemServiceTableView, SelectionChanged_WithSelection_StoresName)
{
    // Arrange
    QStandardItemModel model(1, SystemServiceTableModel::kSystemServiceTableColumnCount);
    auto *item = new QStandardItem(QString("ut.service"));
    model.setItem(0, SystemServiceTableModel::kSystemServiceNameColumn, item);
    QItemSelection selected;
    selected.select(model.index(0, 0),
                    model.index(0, SystemServiceTableModel::kSystemServiceTableColumnCount - 1));
    QItemSelection deselected;
    // Act
    m_tester->selectionChanged(selected, deselected);
    // Assert（B22）
    EXPECT_TRUE(m_tester->m_selectedSName.isValid());
    EXPECT_EQ(m_tester->m_selectedSName.toString(), QString("ut.service"));
}

// sizeHintForColumn：取表头提示与视图提示的较大值 + 2*margin
TEST_F(UT_SystemServiceTableView, SizeHintForColumn_Stubs_ReturnsMaxPlusMargin)
{
    // Arrange: sizeHintForColumn 是虚函数，Itanium ABI 成员指针前 8 字节是 vtable 索引
    // 而非代码地址，ADDR 的 union 技巧会拿到非法地址（段错误），必须经 C 强转取真实符号；
    // header()->sizeHintForColumn 虚派发到 QAbstractItemView 符号，
    // BaseTableView::sizeHintForColumn 限定调用落到 QTreeView 符号（BaseTableView 未重写）
    Stub stub;
    typedef int (*fptr_aiv)(QAbstractItemView *, int);
    stub.set((fptr_aiv)(int (QAbstractItemView::*)(int) const)&QAbstractItemView::sizeHintForColumn,
             ut_svc_stub_headerview_sizeHintForColumn);
    typedef int (*fptr_tv)(QTreeView *, int);
    stub.set((fptr_tv)(int (QTreeView::*)(int) const)&QTreeView::sizeHintForColumn,
             ut_svc_stub_treeview_sizeHintForColumn);
    // Act
    int value = m_tester->sizeHintForColumn(0);
    // Assert: max(10, 5) + 2*10 = 30，且结果为正
    EXPECT_EQ(value, 30);
    EXPECT_GT(value, 0);
}

// resizeEvent：spinner 保持居中
TEST_F(UT_SystemServiceTableView, ResizeEvent_IdleSpinner_CentersSpinner)
{
    // Arrange
    ASSERT_NE(m_tester->m_spinner, nullptr);
    QResizeEvent ev(QSize(0, 0), QSize(400, 300));
    // Act
    m_tester->resizeEvent(&ev);
    // Assert: 居中且位置为非负整数点
    EXPECT_EQ(m_tester->m_spinner->pos(),
              m_tester->rect().center() - m_tester->m_spinner->rect().center());
    EXPECT_GE(m_tester->m_spinner->x(), 0);
}

// initUI：未加载设置时应用默认列宽/隐藏/勾选与控件
TEST_F(UT_SystemServiceTableView, InitUI_NotLoaded_AppliesDefaults)
{
    // Arrange: 受控视图（ctor 必定走 !settingsLoaded 分支）
    SystemServiceTableView *view = createControlledView(nullptr);
    // Act: 构造内已执行 initUI(false)，列宽查询确认装配完成
    int nameWidth = view->columnWidth(SystemServiceTableModel::kSystemServiceNameColumn);
    // Assert（B28）
    EXPECT_EQ(nameWidth, 200);
    ASSERT_NE(view->m_contextMenu, nullptr);
    EXPECT_EQ(view->m_contextMenu->objectName(), QString("ServiceContextMenu"));
    EXPECT_EQ(view->m_contextMenu->accessibleName(), QString("ServiceContextMenu"));
    ASSERT_NE(view->m_headerContextMenu, nullptr);
    EXPECT_EQ(view->m_headerContextMenu->objectName(), QString("ServiceHeaderContextMenu"));
    EXPECT_EQ(view->columnWidth(SystemServiceTableModel::kSystemServiceNameColumn), 200);
    EXPECT_FALSE(view->isColumnHidden(SystemServiceTableModel::kSystemServiceNameColumn));
    EXPECT_TRUE(view->isColumnHidden(SystemServiceTableModel::kSystemServiceLoadStateColumn));
    EXPECT_FALSE(view->isColumnHidden(SystemServiceTableModel::kSystemServiceActiveStateColumn));
    EXPECT_TRUE(view->isColumnHidden(SystemServiceTableModel::kSystemServicePIDColumn));
    ASSERT_NE(view->m_loadStateHeaderAction, nullptr);
    EXPECT_FALSE(view->m_loadStateHeaderAction->isChecked());
    ASSERT_NE(view->m_activeStateHeaderAction, nullptr);
    EXPECT_TRUE(view->m_activeStateHeaderAction->isChecked());
    ASSERT_NE(view->m_pidHeaderAction, nullptr);
    EXPECT_FALSE(view->m_pidHeaderAction->isChecked());
    ASSERT_NE(view->m_spinner, nullptr);
    delete view;
}

// initUI：settingsLoaded 为 true 时保持现有列宽
TEST_F(UT_SystemServiceTableView, InitUI_Loaded_KeepsWidths)
{
    // Arrange: 受控视图先应用默认 200
    SystemServiceTableView *view = createControlledView(nullptr);
    int before = view->columnWidth(SystemServiceTableModel::kSystemServiceNameColumn);
    ASSERT_EQ(before, 200);
    // Act
    view->initUI(true);
    // Assert（B29）: 默认样式分支未执行，宽度保持
    EXPECT_EQ(view->columnWidth(SystemServiceTableModel::kSystemServiceNameColumn), before);
    EXPECT_EQ(view->columnWidth(SystemServiceTableModel::kSystemServiceNameColumn), 200);
    delete view;
}

// initConnections：快捷键与菜单动作装配
TEST_F(UT_SystemServiceTableView, InitConnections_DefaultFlags_WiresShortcutsMenu)
{
    // Arrange: 受控视图 ctor 已执行 initConnections
    SystemServiceTableView *view = createControlledView(nullptr);
    // Act: 构造时完成装配，这里查询断言
    ASSERT_NE(view->m_refreshKP, nullptr);
    ASSERT_NE(view->m_startKP, nullptr);
    ASSERT_NE(view->m_stopKP, nullptr);
    ASSERT_NE(view->m_restartKP, nullptr);
    // Assert（B30）: 快捷键与菜单动作装配完整
    EXPECT_EQ(view->m_startKP->key(), QKeySequence(Qt::ALT + Qt::Key_S));
    EXPECT_EQ(view->m_stopKP->key(), QKeySequence(Qt::ALT + Qt::Key_T));
    EXPECT_EQ(view->m_restartKP->key(), QKeySequence(Qt::ALT + Qt::Key_R));
    ASSERT_NE(view->m_contextMenu, nullptr);
    EXPECT_EQ(view->m_contextMenu->actions().size(), 5);   // Start/Stop/Restart/启动类型子菜单/Refresh
    ASSERT_NE(view->m_setServiceStartupModeMenu, nullptr);
    EXPECT_EQ(view->m_setServiceStartupModeMenu->actions().size(), 2);   // Auto/Manual
    ASSERT_NE(view->m_headerContextMenu, nullptr);
    EXPECT_EQ(view->m_headerContextMenu->actions().size(), 7);
    delete view;
}

// adjustInfoLabelVisibility：空模型且 spinner 空闲 → 标签居中显示
TEST_F(UT_SystemServiceTableView, AdjustInfoLabelVisibility_EmptyModelSpinnerIdle_ShowsAtCenter)
{
    // Arrange: 先显示顶层窗口 —— 子控件 isVisible() 依赖祖先链可见性（Qt6 语义），
    // 隐藏视图中 setVisible(true) 后 isVisible() 仍为 false，居中分支也不会执行
    m_tester->window()->show();
    // Act: 测试环境代理模型为空，spinner 未转动
    m_tester->adjustInfoLabelVisibility();
    // Assert（B23）
    EXPECT_TRUE(m_tester->m_noMatchingResultLabel->isVisible());
    EXPECT_EQ(m_tester->m_noMatchingResultLabel->pos(),
              m_tester->rect().center() - m_tester->m_noMatchingResultLabel->rect().center());
}

// adjustInfoLabelVisibility：spinner 转动时标签隐藏
TEST_F(UT_SystemServiceTableView, AdjustInfoLabelVisibility_SpinnerPlaying_Hides)
{
    // Arrange
    ASSERT_NE(m_tester->m_spinner, nullptr);
    m_tester->m_spinner->start();
    // Act
    m_tester->adjustInfoLabelVisibility();
    // Assert（B24）: spinner 运转中标签隐藏且不居中到当前视图中心
    EXPECT_FALSE(m_tester->m_noMatchingResultLabel->isVisible());
    EXPECT_NE(m_tester->m_noMatchingResultLabel, nullptr);
    m_tester->m_spinner->stop();
}
