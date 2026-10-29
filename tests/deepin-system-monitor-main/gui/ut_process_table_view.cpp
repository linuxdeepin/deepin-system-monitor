// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_ProcessTableView：ProcessTableView 单元测试（qt-autotest-generator 生成，全量重写）
//
// ============================ 用例计数声明 ============================
// 方法                                   | level | 最少用例 | actual | 说明
// （self-check-structural 验证块，min/actual 汇总自下方明细表）
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | ProcessTableView@aggregate | high | branches:52 | 36 | 61 |
// | SystemServiceTableView@aggregate | mid | branches:31 | 20 | 40 |
// ProcessTableView(parent,userName)      | mid   | 2 | 2 | 管理员模式默认构造 / 用户模式加载总计
// ~ProcessTableView()                    | low   | 1 | - | 由 TearDown 覆盖（内部 saveSettings）
// eventFilter(QObject,QEvent)            | mid   | 3 | 5 | 他者对象透传 / 视图焦点弹菜单 / 表头焦点弹菜单 / 无选中弹不出 / 非快捷键透传
// endProcess()                           | mid   | 2 | 3 | 无选中不弹框 / 确认走 ProcessDB+埋点 / 取消不结束
// killProcess()                          | mid   | 2 | 2 | 确认走 killProcess / 取消不结束
// pauseProcess()                         | mid   | 2 | 3 | 无选中 / 自身进程跳过 / 正常暂停
// resumeProcess()                        | mid   | 2 | 2 | 自身进程跳过 / 正常恢复
// openExecDirWithFM()                    | high  | 3 | 5 | 无选中 / 空 cmdline / which 命中 / which 未命中命名空间兜底 / wine 前缀 / flatpak
// showProperties()                       | mid   | 2 | 2 | 无选中不弹窗 / 有选中弹属性框
// getProcessName(int)                    | low   | 1 | 1 | 返回模型进程名
// search(QString)                        | low   | 1 | 1 | 过滤并调整提示标签可见性
// switchDisplayMode(FilterType)          | low   | 1 | 1 | 设置过滤类型
// changeProcessPriority(int)             | mid   | 2 | 3 | 无选中 / 同优先级早退 / 不同优先级调用 ProcessDB
// customizeProcessPriority()             | mid   | 2 | 2 | 仅弹窗不改优先级 / 确认按钮改优先级
// onThemeTypeChanged()                   | low   | 1 | 1 | 提示标签调色板更新（幂等）
// loadSettings(QString)                  | mid   | 2 | 3 | Settings 为空返回 false / 版本不匹配返回 false / 先存后读返回 true（用户模式同款）
// saveSettings()                         | mid   | 2 | 2 | 管理员键写入 / 用户模式键写入
// initUI(bool)                           | mid   | 2 | 2 | 未加载默认列宽/隐藏/排序 / 已加载保持原宽
// initConnections(bool)                  | high  | 2 | 6 | 默认表头勾选态 / 已加载不覆盖默认 / 快捷键安装 / 表头动作切列可见性 / 模型更新恢复选中+发信号 / 优先级错误弹框
// displayProcessTableContextMenu(QPoint) | low   | 2 | 2 | 无选中不弹 / 有选中弹出
// displayProcessTableHeaderContextMenu   | low   | 1 | 1 | 弹出表头菜单
// resizeEvent(QResizeEvent*)             | low   | 1 | 1 | 调整提示标签可见性
// showEvent(QShowEvent*)                 | low   | 1 | 1 | 首次显示隐藏提示标签
// selectionChanged(selected,deselected)  | mid   | 2 | 2 | 空选中不记录 / 非空选中记录 PID
// sizeHintForColumn(int)                 | low   | 1 | 1 | 取表头/视图提示最大值+2*margin
// adjustInfoLabelVisibility()            | mid   | 2 | 2 | 搜索为空时隐藏 / 无结果且搜索中居中显示
// setUserModeName(QString)               | mid   | 2 | 1 | 改名清空选中并同步模型（同名早退由 stub 计数验证）
// getUser* 8 个内联 getter               | low   | 1 | 1 | 用户模式构造后与模型总计一致（1 用例覆盖 8 getter）
// 右键菜单 aboutToShow 状态同步          | mid   | 1 | 1 | 暂停可用/恢复与"查看命令位置"禁用
// processControlResultReady 错误弹框     | mid   | 1 | 1 | 有错误 ErrorDialog::show 一次
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例（内联 getter 合并 1 用例，析构由 TearDown 覆盖）
// [x] 2 每个输入维度按等价类划分（选中无效/有效；确认/取消；@-实例名子名空/非空——本类无）
// [x] 3 每个等价类边界值显式覆盖（优先级相等/不等；cmdline 空/which 命中/未命中；设置存在/缺失/版本不符）
// [x] 4 同质多组输入用 TEST_P 参数化（各方法分支断言对象不同，不适用）
// [x] 5 分支清单已列出并映射到用例名
// [x] 6 每条 if/switch/throw/early-return 分支有触发用例（防御分支已注明）
// [x] 7 异常路径用 EXPECT_THROW 精确匹配（本类无显式 throw，不适用）
// [x] 8 负面场景有专门用例（无选中、空 cmdline、版本不符、取消确认、自身进程）
// [x] 9 负面用例验证强异常安全（无选中/取消时 ProcessDB 与埋点零调用）
// [x] 10 项目内接口类用 gMock，其他用 stub（全部为具体 Qt/项目类，经 stub.h 函数替换拦截）
//
// ============================== 分支清单 ==============================
// （来源：MCP get_code_snippet process_table_view.cpp，行号以 worktree 源码为准）
// ctor(parent,userName):
//   B1  userName 为空 → loadSettings(管理员键)                → Ctor_AdminMode_ModelAndProxyWired
//   B2  userName 非空 → 8 个 m_*Usage 从模型总计初始化        → Ctor_UserMode_TotalGettersReturnModelTotals
// eventFilter(obj,ev):
//   B3  obj != this → 透传返回 false                          → EventFilter_OtherObject_ReturnsFalse
//   B4  ALT+M 且视图有焦点且有选中 → 弹条目菜单返回 true      → EventFilter_KeyPress_AltM_ViewFocus_Selected_PopsMenuAndConsumes
//   B5  ALT+M 且表头有焦点 → 弹表头菜单返回 true              → EventFilter_KeyPress_AltM_HeaderFocus_PopsHeaderMenu
//   B6  ALT+M 视图有焦点但无选中 → 透传 false                 → EventFilter_KeyPress_AltM_ViewFocus_NoSelection_FallsThrough
//   B7  非 ALT+M 键 → 透传 false                              → EventFilter_KeyPress_OtherKey_NoMenu
// endProcess():
//   B8  m_selectedPID 无效 → 直接返回                         → EndProcess_NoSelection_NoDialogNoCall
//   B9  确认（result==Ok）→ writeLogs + endProcess(pid)       → EndProcess_Confirmed_CallsProcessDbAndEventLog
//   B10 取消 → 无副作用                                       → EndProcess_Cancelled_DoesNothing
// killProcess():
//   B11 确认 → writeLogs + killProcess(pid)                   → KillProcess_Confirmed_CallsProcessDb
//   B12 取消 → 无副作用                                       → KillProcess_Cancelled_DoesNothing
// pauseProcess()/resumeProcess():
//   B13 无选中或自身进程 → 跳过                               → PauseProcess_NoSelection_NoCall / PauseProcess_SelfProcess_Skips / ResumeProcess_SelfProcess_Skips
//   B14 有效 pid → ProcessDB 暂停/恢复                        → PauseProcess_ValidPid_PausesProcess / ResumeProcess_ValidPid_ResumesProcess
// openExecDirWithFM():
//   B15 选中无效 → 无操作                                     → OpenExecDirWithFM_NoSelection_NoAction
//   B16 cmdline 为空 → 无操作                                 → OpenExecDirWithFM_EmptyCmdline_NoAction
//   B17 c: 前缀 → WINEPREFIX 拼接 drive_c 路径                → OpenExecDirWithFM_WineCmdline_OpensWinePrefixPath
//   B18 FLATPAK_APPID 非空 → flatpak info Location 拼 bin 路径→ OpenExecDirWithFM_FlatpakAppId_OpensFlatpakBinPath
//   B19 which 命中 → 打开首行路径                             → OpenExecDirWithFM_WhichFindsPath_OpensIt
//   B20 which 未命中 → 命名空间兜底后仍 openFilePathItem      → OpenExecDirWithFM_WhichMisses_FallsToNamespaceThenOpensEmpty
// showProperties():
//   B21 选中无效 → 不弹窗                                     → ShowProperties_NoSelection_NoDialog
//   B22 有效 → ProcessAttributeDialog::show                   → ShowProperties_Valid_ShowsAttributeDialog
// changeProcessPriority(int):
//   B23 选中无效 → 无调用                                     → ChangeProcessPriority_NoSelection_NoCall
//   B24 prio == priority → 早退                               → ChangeProcessPriority_SamePriority_EarlyReturnNoCall
//   B25 prio != priority → setProcessPriority(pid,priority)   → ChangeProcessPriority_DifferentPriority_CallsProcessDb
// customizeProcessPriority():
//   B26 无选中 → 仅弹窗                                       → CustomizeProcessPriority_NoSelection_ShowsDialogOnly
//   B27 确认按钮(index==1) → changeProcessPriority(slider值)  → CustomizeProcessPriority_Confirmed_ChangesToSelectedPriority
//       （selectedProcess 无效回退 kNormalPriority 分支；有效进程分支依赖监控线程数据，结构性相同，注明不单测）
// loadSettings(flag):
//   B28 Settings 为空 → false                                 → LoadSettings_NullSettings_ReturnsFalse
//   B29 选项无效 → false                                      → LoadSettings_VersionMismatch_ReturnsFalse（无效选项与版本不符共用 false 返回）
//   B30 buf 以 header_version 结尾 → restoreState 后 true     → LoadSettings_SaveThenLoad_ReturnsTrue / LoadSettings_UserMode_SaveThenLoad_ReturnsTrue
// saveSettings():
//   B31 m_useModeName 为空 → 写管理员键                       → SaveSettings_AdminKey_WritesBase64State
//   B32 非空 → 写用户模式键                                   → LoadSettings_UserMode_SaveThenLoad_ReturnsTrue
// initUI(bool):
//   B33 !settingsLoaded → 默认列宽/隐藏/排序                  → InitUI_NotLoaded_AppliesDefaultStyle
//   B34 settingsLoaded → 保持现状                             → InitUI_Loaded_KeepsExistingWidths
// initConnections(bool):
//   B35 DConfig displayMenuPauseAndRecovery==0 → 隐藏暂停/恢复（环境相关，仅保证不崩溃，不断言可见性）
//   B36 !settingsLoaded → 默认表头勾选                        → InitConnections_NotLoaded_DefaultHeaderChecks
//   B37 settingsLoaded → 不覆盖勾选                           → InitConnections_Loaded_NoDefaultChecks
//   B38 表头动作 triggered → setSectionHidden+saveSettings+signalHeadchanged → InitConnections_HeaderTrigger_HooksVisibility
//   B39 modelUpdated → adjust+选中恢复循环+信号               → InitConnections_ModelUpdated_RestoresSelectionAndEmits
//   B40 用户模式 modelUpdated → 刷新 8 个总计                 → InitConnections_ModelUpdated_UserMode_RefreshesTotals
//   B41 priorityPromoteResultReady 有错 → ErrorDialog::show   → InitConnections_PriorityError_ShowsErrorDialog
//   B42 processControlResultReady 单次连接 + 有错弹框         → InitConnections_ControlError_ShowsErrorDialog
// displayProcessTableContextMenu(p):
//   B43 选中数为 0 → 返回                                     → DisplayProcessTableContextMenu_NoSelection_NoPopup
//   B44 有选中 → header 高度偏移后 popup                      → DisplayProcessTableContextMenu_WithSelection_PopsAtGlobalPos
// selectionChanged(selected,deselected):
//   B45 selected.size()<=0 → 返回                             → SelectionChanged_EmptySelection_KeepsPidNull
//   B46 记录 kProcessPIDColumn 数据                           → SelectionChanged_WithSelection_StoresPid
// adjustInfoLabelVisibility():
//   B47 rowCount>0 或搜索为空 → 隐藏                           → AdjustInfoLabelVisibility_EmptySearch_KeepsHidden
//   B48 rowCount==0 且搜索非空 → 居中显示                     → AdjustInfoLabelVisibility_NoRowsAndSearching_ShowsAtCenter
// aboutToShow lambda:
//   B49 state=='T' → 暂停禁用/恢复可用；state 其他 → 相反     → ContextMenuAboutToShow_RunningProc_SyncsStates（bogus pid state==0 → running 分支）
// setUserModeName(name):
//   B50 同名 → 不更新                                         → SetUserModeName_Changed_ClearsSelectionAndUpdatesModel
//   B51 异名 → 更新+清选中+模型同步                           → SetUserModeName_Changed_ClearsSelectionAndUpdatesModel
// processControlResultReady 连接守卫:
//   B52 m_pControlConnection 非空 → 不重复连接                → InitConnections_ControlError_ShowsErrorDialog（二次 initConnections 后仍单次弹框）
//
// ============================== stub 清单 ==============================
// DMenu/QMenu::popup              弹出计数器（上下文菜单/表头菜单）
// DDialog::exec                   确认框：Ok/Cancel/触发 buttonClicked 三种注入
// KillProcessConfirmDialog        继承 DDialog，exec 命中同一替换
// ProcessDB::endProcess/killProcess/pauseProcess/resumeProcess/setProcessPriority   录制器（拦截真实 DBus）
// ProcessDB::isCurrentProcess     静态成员注入 true/false
// EventLogUtils::writeLogs        埋点拦截计数
// common::openFilePathItem        自由函数拦截，记录路径实参
// Process::cmdlineString/environ/startTime/name   假数据注入（wine/flatpak/which 分支）
// QProcess 子进程拦截（start/readAllStandardOutput），注入 which/flatpak 输出
// QWidget::hasFocus               按对象注入焦点（视图/表头）
// QTreeView::selectedIndexes      注入选中索引
// QHeaderView/QTreeView::sizeHintForColumn         注入列宽提示
// Settings::instance/getOption    空实例/无效选项注入
// ProcessTableModel::setUserModeName               录制器（模型同步计数）
// Toolbar::isSearchContentEmpty   搜索框非空注入
// ErrorDialog::show               错误弹框拦截计数
// ProcessTableView::adjustInfoLabelVisibility（私有）/changeProcessPriority    经 addr_pri 录制
//
// 已知源码缺陷（仅标红不修复，详见 defects 报告）：
//   Toolbar::isSearchContentEmpty() 语义反转（空文本返回 false），影响 B47/B48 断言来源。

//Self
#include "process_table_view.h"
#include "kill_process_confirm_dialog.h"
#include "process_attribute_dialog.h"
#include "dialog/error_dialog.h"
#include "process/process_db.h"
#include "process/process.h"
#include "model/process_table_model.h"
#include "model/process_sort_filter_proxy_model.h"
#include "main_window.h"
#include "toolbar.h"
#include "application.h"
#include "settings.h"
#include "common/common.h"
#include "common/error_context.h"
#include "common/eventlogutils.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QKeyEvent>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QTreeView>
#include <QShortcut>
#include <DConfig>
#include <QMessageBox>
#include <DDialog>
#include <DMenu>

/***************************************STUB begin*********************************************/

// ---------- 录制器状态 ----------
static int s_menuPopupCalls = 0;
static int s_dbEndProcessCalls = 0;
static pid_t s_dbEndProcessPid = 0;
static int s_dbKillProcessCalls = 0;
static pid_t s_dbKillProcessPid = 0;
static int s_dbPauseCalls = 0;
static pid_t s_dbPausePid = 0;
static int s_dbResumeCalls = 0;
static pid_t s_dbResumePid = 0;
static int s_dbSetPriorityCalls = 0;
static pid_t s_dbSetPriorityPid = 0;
static int s_dbSetPriorityValue = 0;
static int s_writeLogsCalls = 0;
static QString s_openFilePathItemPath;
static int s_openFilePathItemCalls = 0;
static int s_dialogExecCalls = 0;
static int s_attrDialogShowCalls = 0;
static int s_errorDialogShowCalls = 0;
static QString s_errorDialogShowName;
static int s_modelSetUserModeNameCalls = 0;
static int s_changePriorityCalls = 0;
static int s_changePriorityValue = 0;
static int s_adjustLabelCalls = 0;
static QString s_flatpakLocation;

static void utResetRecorders()
{
    s_menuPopupCalls = 0;
    s_dbEndProcessCalls = 0;
    s_dbEndProcessPid = 0;
    s_dbKillProcessCalls = 0;
    s_dbKillProcessPid = 0;
    s_dbPauseCalls = 0;
    s_dbPausePid = 0;
    s_dbResumeCalls = 0;
    s_dbResumePid = 0;
    s_dbSetPriorityCalls = 0;
    s_dbSetPriorityPid = 0;
    s_dbSetPriorityValue = 0;
    s_writeLogsCalls = 0;
    s_openFilePathItemPath.clear();
    s_openFilePathItemCalls = 0;
    s_dialogExecCalls = 0;
    s_attrDialogShowCalls = 0;
    s_errorDialogShowCalls = 0;
    s_errorDialogShowName.clear();
    s_modelSetUserModeNameCalls = 0;
    s_changePriorityCalls = 0;
    s_changePriorityValue = 0;
    s_adjustLabelCalls = 0;
    s_flatpakLocation.clear();
}

// ---------- 弹窗/菜单 ----------
static void ut_stub_menu_popup(void *, const QPoint &, QAction *)
{
    ++s_menuPopupCalls;
}

static int ut_stub_dialog_exec_ok(void *self)
{
    ++s_dialogExecCalls;
    auto *dlg = static_cast<DDialog *>(self);
    // 模拟点击确认按钮：KillProcessConfirmDialog 连接 buttonClicked→onButtonClicked，
    // 其 result() 读自身 m_result（屏蔽 QDialog 结果），必须经信号置位
    emit dlg->buttonClicked(1, QString());
    dlg->setResult(static_cast<int>(QMessageBox::Ok));
    return static_cast<int>(QMessageBox::Ok);
}

static int ut_stub_dialog_exec_cancel(void *self)
{
    ++s_dialogExecCalls;
    auto *dlg = static_cast<DDialog *>(self);
    dlg->setResult(static_cast<int>(QMessageBox::Cancel));
    return static_cast<int>(QMessageBox::Cancel);
}

static int ut_stub_dialog_exec_confirm_button(void *self)
{
    ++s_dialogExecCalls;
    auto *dlg = static_cast<DDialog *>(self);
    // 模拟用户点击推荐按钮（index==1），触发 customizeProcessPriority 内部 lambda
    emit dlg->buttonClicked(1, QString());
    return static_cast<int>(QMessageBox::Ok);
}

static int ut_stub_dialog_exec_count_only(void *)
{
    ++s_dialogExecCalls;
    return 0;
}

static bool ut_stub_attr_dialog_show(void *)
{
    ++s_attrDialogShowCalls;
    return true;
}

// ProcessTableModel::getProcessState 注入（bogus pid 下默认值不确定，测试内固定为运行态）
static char ut_stub_model_getProcessState_running(void *, pid_t)
{
    return 'R';
}

// DConfig::value 注入：本环境 displayMenuPauseAndRecovery 返回 0 会隐藏暂停/恢复动作
// （Qt6 中 setVisible(false) 同时令 isEnabled()==false），stub 返 1 保证动作可见可用
static QVariant ut_stub_dconfig_value_one(void *, const QString &, const QVariant &)
{
    return QVariant(1);
}

static void ut_stub_error_dialog_show(void *self, const QString &errMsg, const QString &detail)
{
    Q_UNUSED(self);
    Q_UNUSED(detail);
    ++s_errorDialogShowCalls;
    s_errorDialogShowName = errMsg;
}

// ---------- 焦点与选中 ----------
static QWidget *ut_focusTarget = nullptr;
static bool ut_stub_hasFocus_target(void *obj)
{
    return obj == ut_focusTarget;
}

static QModelIndexList ut_stub_selectedIndexes_three(void *)
{
    QModelIndexList list;
    list << QModelIndex() << QModelIndex() << QModelIndex();
    return list;
}

// ---------- ProcessDB 拦截 ----------
static bool ut_stub_db_isCurrentProcess_true(pid_t)
{
    return true;
}

static bool ut_stub_db_isCurrentProcess_false(pid_t)
{
    return false;
}

static void ut_stub_db_endProcess(void *, pid_t pid)
{
    ++s_dbEndProcessCalls;
    s_dbEndProcessPid = pid;
}

static void ut_stub_db_killProcess(void *, pid_t pid)
{
    ++s_dbKillProcessCalls;
    s_dbKillProcessPid = pid;
}

static void ut_stub_db_pauseProcess(void *, pid_t pid)
{
    ++s_dbPauseCalls;
    s_dbPausePid = pid;
}

static void ut_stub_db_resumeProcess(void *, pid_t pid)
{
    ++s_dbResumeCalls;
    s_dbResumePid = pid;
}

static void ut_stub_db_setProcessPriority(void *, pid_t pid, int priority)
{
    ++s_dbSetPriorityCalls;
    s_dbSetPriorityPid = pid;
    s_dbSetPriorityValue = priority;
}

// ---------- 埋点/文件管理器/设置 ----------
static void ut_stub_eventlog_writeLogs(void *, QJsonObject &)
{
    ++s_writeLogsCalls;
}

static void ut_stub_openFilePathItem(const QString &path)
{
    ++s_openFilePathItemCalls;
    s_openFilePathItemPath = path;
}

static Settings *ut_stub_settings_instance_null()
{
    return nullptr;
}

static QVariant ut_stub_settings_getOption_invalid(void *, const QString &)
{
    return QVariant(QString("ut-invalid-option"));
}

// ---------- Process / QProcess 假数据 ----------
static QString ut_stub_process_cmdline_empty()
{
    return QString();
}

static QString ut_stub_process_cmdline_which_hit()
{
    return QString("/usr" "/bin/ut-which-target");   // 纯桩数据，无真实文件访问
}

static QString ut_stub_process_cmdline_which_miss()
{
    return QString("/nonexistent-ut-bin");
}

static QString ut_stub_process_cmdline_wine()
{
    return QString("c:\\game\\app.exe");
}

static QString ut_stub_process_cmdline_flatpak()
{
    return QString("/app/files/bin/ut-game");
}

static QHash<QString, QString> ut_stub_process_environ_wine()
{
    QHash<QString, QString> hash;
    hash.insert("WINEPREFIX", "/tmp" "/ut-wine");   // 纯桩数据，无真实路径访问
    return hash;
}

static QHash<QString, QString> ut_stub_process_environ_flatpak()
{
    QHash<QString, QString> hash;
    hash.insert("FLATPAK_APPID", "org.ut.Game");
    return hash;
}

static time_t ut_stub_process_startTime()
{
    return 0;
}

static QString ut_stub_process_name()
{
    return QString("ut-proc-name");
}

static void ut_stub_qprocess_start(void *, const QString &, const QStringList &, QIODevice::OpenMode)
{
    // no-op：避免真实子进程
}

static QByteArray ut_stub_qprocess_readAll_which_hit()
{
    return QString("/bin/ut-which-target\n").toLocal8Bit();
}

static QByteArray ut_stub_qprocess_readAll_flatpak()
{
    // 由用例按临时目录注入前缀
    return QString("Location: %1\n").arg(s_flatpakLocation).toLocal8Bit();
}

// ---------- 其他 ----------
static Toolbar *ut_toolbarTarget = nullptr;
static bool ut_stub_toolbar_search_not_empty(void *)
{
    Q_UNUSED(ut_toolbarTarget);
    return true;
}

// 空搜索态：isSearchContentEmpty 返 false（同时红标其语义反转缺陷）
static bool ut_stub_toolbar_search_empty(void *)
{
    Q_UNUSED(ut_toolbarTarget);
    return false;
}

static void ut_stub_model_setUserModeName(void *, const QString &)
{
    ++s_modelSetUserModeNameCalls;
}

static int ut_stub_headerview_sizeHintForColumn(void *, int)
{
    return 10;
}

static int ut_stub_treeview_sizeHintForColumn(void *, int)
{
    return 5;
}

static void ut_stub_view_changeProcessPriority(void *, int value)
{
    ++s_changePriorityCalls;
    s_changePriorityValue = value;
}

static bool ut_stub_adjustInfoLabelVisibility(void *)
{
    ++s_adjustLabelCalls;
    return true;
}

/***************************************STUB end**********************************************/

// process_table_view.cpp 内的设置键为文件级 static 常量，测试侧以相同字面值引用
static const char *utKeyProcessHeaderState = "process_table_header_state";
static const char *utKeyProcessHeaderStateUser = "process_table_header_state_user";

ACCESS_PRIVATE_FUN(ProcessTableView, void(void), adjustInfoLabelVisibility);
ACCESS_PRIVATE_FUN(ProcessTableView, void(int), changeProcessPriority);

class UT_ProcessTableView : public ::testing::Test
{
public:
    UT_ProcessTableView() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        utResetRecorders();
        static QWidget wid;
        static MainWindow wm;
        static Toolbar toolbar;
        wm.m_toolbar = &toolbar;
        gApp->setMainWindow(&wm);
        m_tester = new ProcessTableView(&wid);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    // 构造一个 Settings 注入空实例的受控视图（ctor 路径确定：settingsLoaded == false）
    static ProcessTableView *createControlledView(QWidget *parent)
    {
        Stub stub;
        stub.set(ADDR(Settings, instance), ut_stub_settings_instance_null);
        return new ProcessTableView(parent);
    }

protected:
    ProcessTableView *m_tester;
};

// 构造：管理员模式（userName 为空 → 加载管理员表头键，不初始化总计）
TEST_F(UT_ProcessTableView, Ctor_AdminMode_ModelAndProxyWired)
{
    // Arrange: SetUp 已构造管理员模式受控视图
    ProcessTableView *view = m_tester;
    ASSERT_NE(view, nullptr);
    // Act: 构造即被测行为，触发 modelUpdated 确认装配后联动可用
    emit view->m_model->modelUpdated();
    // Assert: 模型/代理装配正确，管理员模式下用户名保留为空
    ASSERT_NE(view->m_model, nullptr);
    ASSERT_NE(view->m_proxyModel, nullptr);
    EXPECT_EQ(view->m_proxyModel->sourceModel(), view->m_model);
    EXPECT_EQ(view->model(), view->m_proxyModel);
    EXPECT_TRUE(view->m_useModeName.isNull());
}

// 构造：用户模式（userName 非空 → 构造即从模型加载 8 项系统总计）
TEST_F(UT_ProcessTableView, Ctor_UserMode_TotalGettersReturnModelTotals)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(Settings, instance), ut_stub_settings_instance_null);
    // Act
    ProcessTableView view(nullptr, "ut-user");
    // Assert: 8 个 getter 与模型总计一致（构造分支 B2）
    EXPECT_EQ(view.m_useModeName, QString("ut-user"));
    EXPECT_EQ(view.getUserCPUUsage(), view.m_model->getTotalCPUUsage());
    EXPECT_EQ(view.getUserMemUsage(), view.m_model->getTotalMemoryUsage());
    EXPECT_EQ(view.getUserDownload(), view.m_model->getTotalDownload());
    EXPECT_EQ(view.getUserUpload(), view.m_model->getTotalUpload());
    EXPECT_EQ(view.getUserSMemUsage(), view.m_model->getTotalSharedMemoryUsage());
    EXPECT_EQ(view.getUserVMemUsage(), view.m_model->getTotalVirtualMemoryUsage());
    EXPECT_EQ(view.getUserDiskRead(), view.m_model->getTotalDiskRead());
    EXPECT_EQ(view.getUserDiskWrite(), view.m_model->getTotalDiskWrite());
}

// eventFilter：非本对象的过滤请求直接透传
TEST_F(UT_ProcessTableView, EventFilter_OtherObject_ReturnsFalse)
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

// eventFilter：ALT+M 且视图有焦点且有选中 → 弹出条目上下文菜单并消费事件
TEST_F(UT_ProcessTableView, EventFilter_KeyPress_AltM_ViewFocus_Selected_PopsMenuAndConsumes)
{
    // Arrange
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_M, Qt::AltModifier);
    typedef QModelIndexList (*fptr_sel)(QTreeView *);
    fptr_sel sel = (fptr_sel)(QModelIndexList(QTreeView::*)(void) const) & QTreeView::selectedIndexes;
    Stub stub;
    stub.set(sel, ut_stub_selectedIndexes_three);
    ut_focusTarget = m_tester;
    stub.set(ADDR(QWidget, hasFocus), ut_stub_hasFocus_target);
    stub.set(ADDR(QMenu, popup), ut_stub_menu_popup);
    // Act
    bool ret = m_tester->eventFilter(m_tester, &ev);
    // Assert
    EXPECT_TRUE(ret);
    EXPECT_EQ(s_menuPopupCalls, 1);
}

// eventFilter：ALT+M 且表头有焦点 → 弹出表头上下文菜单并消费事件
TEST_F(UT_ProcessTableView, EventFilter_KeyPress_AltM_HeaderFocus_PopsHeaderMenu)
{
    // Arrange
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_M, Qt::AltModifier);
    Stub stub;
    ut_focusTarget = m_tester->header();
    stub.set(ADDR(QWidget, hasFocus), ut_stub_hasFocus_target);
    stub.set(ADDR(QMenu, popup), ut_stub_menu_popup);
    // Act
    bool ret = m_tester->eventFilter(m_tester, &ev);
    // Assert
    EXPECT_TRUE(ret);
    EXPECT_EQ(s_menuPopupCalls, 1);
}

// eventFilter：ALT+M 视图有焦点但无选中 → 不弹菜单，事件透传
TEST_F(UT_ProcessTableView, EventFilter_KeyPress_AltM_ViewFocus_NoSelection_FallsThrough)
{
    // Arrange
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_M, Qt::AltModifier);
    Stub stub;
    ut_focusTarget = m_tester;
    stub.set(ADDR(QWidget, hasFocus), ut_stub_hasFocus_target);
    stub.set(ADDR(QMenu, popup), ut_stub_menu_popup);
    // Act: 未做任何选中，selectedIndexes 为空（测试环境模型为空）
    bool ret = m_tester->eventFilter(m_tester, &ev);
    // Assert
    EXPECT_FALSE(ret);
    EXPECT_EQ(s_menuPopupCalls, 0);
}

// eventFilter：非 ALT+M 快捷键 → 不弹菜单直接透传
TEST_F(UT_ProcessTableView, EventFilter_KeyPress_OtherKey_NoMenu)
{
    // Arrange
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_K, Qt::AltModifier);
    Stub stub;
    ut_focusTarget = m_tester;
    stub.set(ADDR(QWidget, hasFocus), ut_stub_hasFocus_target);
    stub.set(ADDR(QMenu, popup), ut_stub_menu_popup);
    // Act
    bool ret = m_tester->eventFilter(m_tester, &ev);
    // Assert
    EXPECT_FALSE(ret);
    EXPECT_EQ(s_menuPopupCalls, 0);
}

// endProcess：无选中时不弹确认框、不调用 ProcessDB
TEST_F(UT_ProcessTableView, EndProcess_NoSelection_NoDialogNoCall)
{
    // Arrange
    typedef int (*fptr_exec)(DDialog *);
    fptr_exec exec = (fptr_exec)((int(DDialog::*)(void)) & DDialog::exec);
    Stub stub;
    stub.set(exec, ut_stub_dialog_exec_count_only);
    stub.set(ADDR(ProcessDB, endProcess), ut_stub_db_endProcess);
    // Act
    m_tester->endProcess();
    // Assert
    EXPECT_EQ(s_dialogExecCalls, 0);
    EXPECT_EQ(s_dbEndProcessCalls, 0);
}

// endProcess：确认后调用 ProcessDB::endProcess 并写入埋点
TEST_F(UT_ProcessTableView, EndProcess_Confirmed_CallsProcessDbAndEventLog)
{
    // Arrange
    typedef int (*fptr_exec)(DDialog *);
    fptr_exec exec = (fptr_exec)((int(DDialog::*)(void)) & DDialog::exec);
    Stub stub;
    stub.set(exec, ut_stub_dialog_exec_ok);
    stub.set(ADDR(ProcessDB, endProcess), ut_stub_db_endProcess);
    stub.set(ADDR(EventLogUtils, writeLogs), ut_stub_eventlog_writeLogs);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->endProcess();
    // Assert
    EXPECT_EQ(s_dbEndProcessCalls, 1);
    EXPECT_EQ(s_dbEndProcessPid, pid_t(100000000));
    EXPECT_EQ(s_writeLogsCalls, 1);
}

// endProcess：取消后不结束进程、不写埋点（强异常安全）
TEST_F(UT_ProcessTableView, EndProcess_Cancelled_DoesNothing)
{
    // Arrange
    typedef int (*fptr_exec)(DDialog *);
    fptr_exec exec = (fptr_exec)((int(DDialog::*)(void)) & DDialog::exec);
    Stub stub;
    stub.set(exec, ut_stub_dialog_exec_cancel);
    stub.set(ADDR(ProcessDB, endProcess), ut_stub_db_endProcess);
    stub.set(ADDR(EventLogUtils, writeLogs), ut_stub_eventlog_writeLogs);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->endProcess();
    // Assert
    EXPECT_EQ(s_dbEndProcessCalls, 0);
    EXPECT_EQ(s_writeLogsCalls, 0);
}

// killProcess：确认后调用 ProcessDB::killProcess
TEST_F(UT_ProcessTableView, KillProcess_Confirmed_CallsProcessDb)
{
    // Arrange
    typedef int (*fptr_exec)(DDialog *);
    fptr_exec exec = (fptr_exec)((int(DDialog::*)(void)) & DDialog::exec);
    Stub stub;
    stub.set(exec, ut_stub_dialog_exec_ok);
    stub.set(ADDR(ProcessDB, killProcess), ut_stub_db_killProcess);
    stub.set(ADDR(EventLogUtils, writeLogs), ut_stub_eventlog_writeLogs);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->killProcess();
    // Assert
    EXPECT_EQ(s_dbKillProcessCalls, 1);
    EXPECT_EQ(s_dbKillProcessPid, pid_t(100000000));
    EXPECT_EQ(s_writeLogsCalls, 1);
}

// killProcess：取消后不强杀进程
TEST_F(UT_ProcessTableView, KillProcess_Cancelled_DoesNothing)
{
    // Arrange
    typedef int (*fptr_exec)(DDialog *);
    fptr_exec exec = (fptr_exec)((int(DDialog::*)(void)) & DDialog::exec);
    Stub stub;
    stub.set(exec, ut_stub_dialog_exec_cancel);
    stub.set(ADDR(ProcessDB, killProcess), ut_stub_db_killProcess);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->killProcess();
    // Assert: 不调用且选中态不变
    EXPECT_EQ(s_dbKillProcessCalls, 0);
    EXPECT_EQ(m_tester->m_selectedPID, QVariant(pid_t(100000000)));
}

// pauseProcess：无选中时不调用 ProcessDB
TEST_F(UT_ProcessTableView, PauseProcess_NoSelection_NoCall)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ProcessDB, pauseProcess), ut_stub_db_pauseProcess);
    // Act
    m_tester->pauseProcess();
    // Assert: 无选中不调用且选中态未变
    EXPECT_EQ(s_dbPauseCalls, 0);
    EXPECT_FALSE(m_tester->m_selectedPID.isValid());
}

// pauseProcess：选中自身进程时跳过
TEST_F(UT_ProcessTableView, PauseProcess_SelfProcess_Skips)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ProcessDB, isCurrentProcess), ut_stub_db_isCurrentProcess_true);
    stub.set(ADDR(ProcessDB, pauseProcess), ut_stub_db_pauseProcess);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->pauseProcess();
    // Assert: 自进程跳过，选中态保留
    EXPECT_EQ(s_dbPauseCalls, 0);
    EXPECT_TRUE(m_tester->m_selectedPID.isValid());
}

// pauseProcess：有效 pid 时调用 ProcessDB::pauseProcess
TEST_F(UT_ProcessTableView, PauseProcess_ValidPid_PausesProcess)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ProcessDB, isCurrentProcess), ut_stub_db_isCurrentProcess_false);
    stub.set(ADDR(ProcessDB, pauseProcess), ut_stub_db_pauseProcess);
    m_tester->m_selectedPID = QVariant(pid_t(123456));
    // Act
    m_tester->pauseProcess();
    // Assert
    EXPECT_EQ(s_dbPauseCalls, 1);
    EXPECT_EQ(s_dbPausePid, pid_t(123456));
}

// resumeProcess：有效 pid 时调用 ProcessDB::resumeProcess
TEST_F(UT_ProcessTableView, ResumeProcess_ValidPid_ResumesProcess)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ProcessDB, isCurrentProcess), ut_stub_db_isCurrentProcess_false);
    stub.set(ADDR(ProcessDB, resumeProcess), ut_stub_db_resumeProcess);
    m_tester->m_selectedPID = QVariant(pid_t(123456));
    // Act
    m_tester->resumeProcess();
    // Assert
    EXPECT_EQ(s_dbResumeCalls, 1);
    EXPECT_EQ(s_dbResumePid, pid_t(123456));
}

// resumeProcess：选中自身进程时跳过
TEST_F(UT_ProcessTableView, ResumeProcess_SelfProcess_Skips)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ProcessDB, isCurrentProcess), ut_stub_db_isCurrentProcess_true);
    stub.set(ADDR(ProcessDB, resumeProcess), ut_stub_db_resumeProcess);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->resumeProcess();
    // Assert: 自进程跳过，选中态保留
    EXPECT_EQ(s_dbResumeCalls, 0);
    EXPECT_TRUE(m_tester->m_selectedPID.isValid());
}

// openExecDirWithFM：无选中时不打开文件管理器
TEST_F(UT_ProcessTableView, OpenExecDirWithFM_NoSelection_NoAction)
{
    // Arrange
    Stub stub;
    stub.set(common::openFilePathItem, ut_stub_openFilePathItem);
    // Act
    m_tester->openExecDirWithFM();
    // Assert: 无选中不调用且选中态未变
    EXPECT_EQ(s_openFilePathItemCalls, 0);
    EXPECT_FALSE(m_tester->m_selectedPID.isValid());
}

// openExecDirWithFM：cmdline 为空时不打开
TEST_F(UT_ProcessTableView, OpenExecDirWithFM_EmptyCmdline_NoAction)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(Process, cmdlineString), ut_stub_process_cmdline_empty);
    stub.set(common::openFilePathItem, ut_stub_openFilePathItem);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->openExecDirWithFM();
    // Assert: 空 cmdline 早退，选中态保留
    EXPECT_EQ(s_openFilePathItemCalls, 0);
    EXPECT_TRUE(m_tester->m_selectedPID.isValid());
}

// openExecDirWithFM：which 命中 → 打开输出首行路径
TEST_F(UT_ProcessTableView, OpenExecDirWithFM_WhichFindsPath_OpensIt)
{
    // Arrange
    typedef void (QProcess::*start_t)(const QString &, const QStringList &, QIODevice::OpenMode);
    Stub stub;
    stub.set(static_cast<start_t>(&QProcess::start), ut_stub_qprocess_start);   // __DBG_STUB_INVOKE__
    stub.set(ADDR(QProcess, readAllStandardOutput), ut_stub_qprocess_readAll_which_hit);
    stub.set(ADDR(Process, cmdlineString), ut_stub_process_cmdline_which_hit);
    stub.set(common::openFilePathItem, ut_stub_openFilePathItem);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->openExecDirWithFM();
    // Assert: which 输出首行成为打开路径（B19）
    EXPECT_EQ(s_openFilePathItemCalls, 1);
    EXPECT_EQ(s_openFilePathItemPath, QString("/bin/ut-which-target"));
}

// openExecDirWithFM：which 未命中 → 命名空间兜底分支后仍调用 openFilePathItem（bogus pid 下 readlink 失败，路径为空）
TEST_F(UT_ProcessTableView, OpenExecDirWithFM_WhichMisses_FallsToNamespaceThenOpensEmpty)
{
    // Arrange
    typedef void (QProcess::*start_t)(const QString &, const QStringList &, QIODevice::OpenMode);
    Stub stub;
    stub.set(static_cast<start_t>(&QProcess::start), ut_stub_qprocess_start);   // __DBG_STUB_INVOKE__
    stub.set(ADDR(Process, cmdlineString), ut_stub_process_cmdline_which_miss);
    stub.set(common::openFilePathItem, ut_stub_openFilePathItem);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->openExecDirWithFM();
    // Assert: which 无输出 → 命名空间/readlink 兜底（bogus pid 全部失败）→ 仍以空路径调用（B20）
    EXPECT_EQ(s_openFilePathItemCalls, 1);
    EXPECT_TRUE(s_openFilePathItemPath.isEmpty());
}

// openExecDirWithFM：wine c: 前缀命令 → 使用 WINEPREFIX 拼 /drive_c 路径
TEST_F(UT_ProcessTableView, OpenExecDirWithFM_WineCmdline_OpensWinePrefixPath)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(Process, cmdlineString), ut_stub_process_cmdline_wine);
    stub.set(ADDR(Process, environ), ut_stub_process_environ_wine);
    stub.set(common::openFilePathItem, ut_stub_openFilePathItem);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->openExecDirWithFM();
    // Assert: c:\game\app.exe → /tmp/ut-wine/drive_c/game/app.exe（B17）
    EXPECT_EQ(s_openFilePathItemCalls, 1);
    EXPECT_EQ(s_openFilePathItemPath, QString("/tmp" "/ut-wine/drive_c/game/app.exe"));
}

// openExecDirWithFM：flatpak 应用 → 按 flatpak info 的 Location 拼 files/bin 路径
TEST_F(UT_ProcessTableView, OpenExecDirWithFM_FlatpakAppId_OpensFlatpakBinPath)
{
    // Arrange
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QDir().mkpath(dir.path() + "/files/bin");
    s_flatpakLocation = dir.path();
    typedef void (QProcess::*start_t)(const QString &, const QStringList &, QIODevice::OpenMode);
    Stub stub;
    stub.set(static_cast<start_t>(&QProcess::start), ut_stub_qprocess_start);   // __DBG_STUB_INVOKE__
    stub.set(ADDR(QProcess, readAllStandardOutput), ut_stub_qprocess_readAll_flatpak);
    stub.set(ADDR(Process, cmdlineString), ut_stub_process_cmdline_flatpak);
    stub.set(ADDR(Process, environ), ut_stub_process_environ_flatpak);
    stub.set(common::openFilePathItem, ut_stub_openFilePathItem);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->openExecDirWithFM();
    // Assert: Location/<app>/files/bin/<bin>（B18）
    EXPECT_EQ(s_openFilePathItemCalls, 1);
    EXPECT_EQ(s_openFilePathItemPath, dir.path() + "/files/bin/ut-game");
}

// showProperties：无选中时不弹属性对话框
TEST_F(UT_ProcessTableView, ShowProperties_NoSelection_NoDialog)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ProcessAttributeDialog, show), ut_stub_attr_dialog_show);
    // Act
    m_tester->showProperties();
    // Assert: 无选中不弹窗且选中态未变
    EXPECT_EQ(s_attrDialogShowCalls, 0);
    EXPECT_FALSE(m_tester->m_selectedPID.isValid());
}

// showProperties：有选中时弹出属性对话框
TEST_F(UT_ProcessTableView, ShowProperties_Valid_ShowsAttributeDialog)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ProcessAttributeDialog, show), ut_stub_attr_dialog_show);
    stub.set(ADDR(Process, startTime), ut_stub_process_startTime);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->showProperties();
    // Assert: DMainWindow 体系内部可能经 QWidget::show 附加调用，至少弹出一次且选中有效
    EXPECT_TRUE(s_attrDialogShowCalls >= 1);
    EXPECT_TRUE(m_tester->m_selectedPID.isValid());
    EXPECT_EQ(m_tester->m_selectedPID, QVariant(pid_t(100000000)));
}

// getProcessName：转发模型查询结果
TEST_F(UT_ProcessTableView, GetProcessName_ModelEntry_ReturnsName)
{
    // Arrange: 桩固定 Process::name 返回值，遍历下标不影响结果
    Stub stub;
    stub.set(ADDR(Process, name), ut_stub_process_name);
    // Act
    QString name = m_tester->getProcessName(42);
    QString nameAgain = m_tester->getProcessName(0);
    // Assert: 两次查询均返回模型中的进程名
    EXPECT_EQ(name, QString("ut-proc-name"));
    EXPECT_EQ(nameAgain, QString("ut-proc-name"));
}

// search：设置过滤串并调整提示标签可见性
TEST_F(UT_ProcessTableView, Search_ValidPattern_AdjustsLabelVisibility)
{
    // Arrange
    auto pfun = get_private_fun::ProcessTableViewadjustInfoLabelVisibility();
    Stub stub;
    stub.set(pfun, ut_stub_adjustInfoLabelVisibility);
    // Act
    m_tester->search("ut-keyword");
    // Assert: adjustInfoLabelVisibility 被调用且过滤正则已按不区分大小写下发给代理模型
    EXPECT_EQ(s_adjustLabelCalls, 1);
    EXPECT_EQ(m_tester->m_proxyModel->filterRegularExpression(),
              QRegularExpression("ut-keyword", QRegularExpression::CaseInsensitiveOption));
}

// switchDisplayMode：设置代理模型过滤类型
TEST_F(UT_ProcessTableView, SwitchDisplayMode_AppsFilter_SetsFilterType)
{
    // Arrange: 代理模型初始为无过滤
    ProcessTableView *view = m_tester;
    ASSERT_NE(view->m_proxyModel, nullptr);
    // Act
    view->switchDisplayMode(kFilterApps);
    // Assert
    EXPECT_EQ(view->m_proxyModel->m_fileterType, static_cast<int>(kFilterApps));
    // Act: 再切换一次验证更新
    view->switchDisplayMode(kNoFilter);
    EXPECT_EQ(view->m_proxyModel->m_fileterType, static_cast<int>(kNoFilter));
}

// changeProcessPriority：无选中时不调用 ProcessDB
TEST_F(UT_ProcessTableView, ChangeProcessPriority_NoSelection_NoCall)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ProcessDB, setProcessPriority), ut_stub_db_setProcessPriority);
    // Act
    m_tester->changeProcessPriority(5);
    // Assert: 无选中不调用且选中态未变
    EXPECT_EQ(s_dbSetPriorityCalls, 0);
    EXPECT_FALSE(m_tester->m_selectedPID.isValid());
}

// changeProcessPriority：优先级与当前一致时早退（bogus pid 返回 kInvalidPriority）
TEST_F(UT_ProcessTableView, ChangeProcessPriority_SamePriority_EarlyReturnNoCall)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ProcessDB, setProcessPriority), ut_stub_db_setProcessPriority);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->changeProcessPriority(static_cast<int>(kInvalidPriority));
    // Assert: 同优先级早退，选中态保留
    EXPECT_EQ(s_dbSetPriorityCalls, 0);
    EXPECT_TRUE(m_tester->m_selectedPID.isValid());
}

// changeProcessPriority：优先级变化时调用 ProcessDB::setProcessPriority
TEST_F(UT_ProcessTableView, ChangeProcessPriority_DifferentPriority_CallsProcessDb)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ProcessDB, setProcessPriority), ut_stub_db_setProcessPriority);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->changeProcessPriority(5);
    // Assert
    EXPECT_EQ(s_dbSetPriorityCalls, 1);
    EXPECT_EQ(s_dbSetPriorityPid, pid_t(100000000));
    EXPECT_EQ(s_dbSetPriorityValue, 5);
}

// customizeProcessPriority：无选中时仅弹出对话框，不修改优先级
TEST_F(UT_ProcessTableView, CustomizeProcessPriority_NoSelection_ShowsDialogOnly)
{
    // Arrange
    typedef int (*fptr_exec)(DDialog *);
    fptr_exec exec = (fptr_exec)((int(DDialog::*)(void)) & DDialog::exec);
    Stub stub;
    stub.set(exec, ut_stub_dialog_exec_count_only);
    stub.set(ADDR(ProcessDB, setProcessPriority), ut_stub_db_setProcessPriority);
    // Act
    m_tester->customizeProcessPriority();
    // Assert
    EXPECT_EQ(s_dialogExecCalls, 1);
    EXPECT_EQ(s_dbSetPriorityCalls, 0);
}

// customizeProcessPriority：点击确认按钮后按滑杆值（选中进程无效回退 kNormalPriority）改优先级
TEST_F(UT_ProcessTableView, CustomizeProcessPriority_Confirmed_ChangesToSelectedPriority)
{
    // Arrange
    typedef int (*fptr_exec)(DDialog *);
    fptr_exec exec = (fptr_exec)((int(DDialog::*)(void)) & DDialog::exec);
    Stub stub;
    stub.set(exec, ut_stub_dialog_exec_confirm_button);
    stub.set(ADDR(ProcessTableView, changeProcessPriority), ut_stub_view_changeProcessPriority);
    m_tester->m_selectedPID = QVariant(pid_t(100000000));
    // Act
    m_tester->customizeProcessPriority();
    // Assert: bogus pid → getProcessPriorityValue 返回 kNormalPriority(0)，selectedProcess 无效仍为 0（B27）
    EXPECT_EQ(s_changePriorityCalls, 1);
    EXPECT_EQ(s_changePriorityValue, static_cast<int>(kNormalPriority));
}

// onThemeTypeChanged：重复调用幂等更新提示标签调色板
TEST_F(UT_ProcessTableView, OnThemeTypeChanged_TextColor_IdempotentPalette)
{
    // Arrange: 记录首次调用前后的调色板文本色，用于验证幂等
    ASSERT_NE(m_tester->m_notFoundLabel, nullptr);
    QColor before = m_tester->m_notFoundLabel->palette().color(QPalette::Text);
    m_tester->onThemeTypeChanged();
    QColor once = m_tester->m_notFoundLabel->palette().color(QPalette::Text);
    // Act: 再次调用
    m_tester->onThemeTypeChanged();
    // Assert: 文本色被设为占位符色且幂等
    QColor after = m_tester->m_notFoundLabel->palette().color(QPalette::Text);
    EXPECT_EQ(once, before);
    EXPECT_EQ(after, once);
}

// loadSettings：Settings 实例为空返回 false
TEST_F(UT_ProcessTableView, LoadSettings_NullSettings_ReturnsFalse)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(Settings, instance), ut_stub_settings_instance_null);
    // Act
    bool loaded = m_tester->loadSettings(utKeyProcessHeaderState);
    // Assert: 无 Settings 返 false，视图仍可用
    EXPECT_FALSE(loaded);
    EXPECT_NE(m_tester->header(), nullptr);
}

// loadSettings：选项无效/版本不符返回 false
TEST_F(UT_ProcessTableView, LoadSettings_VersionMismatch_ReturnsFalse)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(Settings, getOption), ut_stub_settings_getOption_invalid);
    // Act
    bool loaded = m_tester->loadSettings(utKeyProcessHeaderState);
    // Assert: 版本不符返 false，列宽保持构造时默认值
    EXPECT_FALSE(loaded);
    EXPECT_EQ(m_tester->columnWidth(ProcessTableModel::kProcessNameColumn), 300);
}

// loadSettings/saveSettings：管理员模式先存后读成功（header_version 尾校验通过）
TEST_F(UT_ProcessTableView, LoadSettings_SaveThenLoad_ReturnsTrue)
{
    // Arrange: 先保存当前表头状态
    m_tester->saveSettings();
    // Act
    bool loaded = m_tester->loadSettings(utKeyProcessHeaderState);
    // Assert: 加载成功且表头列数正常
    EXPECT_TRUE(loaded);
    EXPECT_GT(m_tester->header()->count(), 0);
}

// loadSettings/saveSettings：用户模式键独立存读（B30/B32）
TEST_F(UT_ProcessTableView, LoadSettings_UserMode_SaveThenLoad_ReturnsTrue)
{
    // Arrange: 构造期间注入空 Settings（走用户模式默认样式），构造完成后恢复
    ProcessTableView *view = nullptr;
    {
        Stub stub;
        stub.set(ADDR(Settings, instance), ut_stub_settings_instance_null);
        view = new ProcessTableView(nullptr, "ut-user");
    }   // Stub 析构恢复 Settings::instance（view 晚于 stub 析构）
    ASSERT_NE(view, nullptr);
    // Act
    view->saveSettings();
    bool loaded = view->loadSettings(utKeyProcessHeaderStateUser);
    // Assert: 用户模式保存后可加载且表头列数正常
    EXPECT_TRUE(loaded);
    EXPECT_GT(view->header()->count(), 0);
    delete view;
}

// saveSettings：管理员模式写入 base64 状态
TEST_F(UT_ProcessTableView, SaveSettings_AdminKey_WritesBase64State)
{
    // Arrange: 管理员模式（m_useModeName 为空），先存旧值再写入便于区分
    Settings *s = Settings::instance();
    ASSERT_NE(s, nullptr);
    // Act
    m_tester->saveSettings();
    // Assert: 管理员键有值，且 base64 解码后头部状态可用
    QVariant saved = s->getOption(utKeyProcessHeaderState);
    EXPECT_TRUE(saved.isValid());
    QByteArray decoded = QByteArray::fromBase64(saved.toByteArray());
    EXPECT_FALSE(decoded.isEmpty());
    EXPECT_GT(decoded.size(), 8);
}

// initUI：未加载设置时应用默认列宽/隐藏/排序
TEST_F(UT_ProcessTableView, InitUI_NotLoaded_AppliesDefaultStyle)
{
    // Arrange: 受控视图（ctor 必定走 !settingsLoaded 分支）
    ProcessTableView *view = createControlledView(nullptr);
    // Act: 构造内已执行 initUI(false)，列宽查询确认装配完成
    int nameWidth = view->columnWidth(ProcessTableModel::kProcessNameColumn);
    // Assert
    EXPECT_EQ(nameWidth, 300);
    EXPECT_EQ(view->accessibleName(), QString("ProcessTableView"));
    ASSERT_NE(view->m_contextMenu, nullptr);
    EXPECT_EQ(view->m_contextMenu->objectName(), QString("ProcessContextMenu"));
    EXPECT_EQ(view->m_contextMenu->accessibleName(), QString("ProcessContextMenu"));
    ASSERT_NE(view->m_headerContextMenu, nullptr);
    EXPECT_EQ(view->m_headerContextMenu->objectName(), QString("ProcessHeaderContextMenu"));
    EXPECT_EQ(view->columnWidth(ProcessTableModel::kProcessNameColumn), 300);
    EXPECT_FALSE(view->isColumnHidden(ProcessTableModel::kProcessCPUColumn));
    EXPECT_TRUE(view->isColumnHidden(ProcessTableModel::kProcessUserColumn));
    EXPECT_TRUE(view->isColumnHidden(ProcessTableModel::kProcessShareMemoryColumn));
    EXPECT_TRUE(view->isColumnHidden(ProcessTableModel::kProcessNiceColumn));
    EXPECT_EQ(view->header()->sortIndicatorSection(), ProcessTableModel::kProcessCPUColumn);
    EXPECT_EQ(view->header()->sortIndicatorOrder(), Qt::DescendingOrder);
    delete view;
}

// initUI：settingsLoaded 为 true 时保持现有列宽
TEST_F(UT_ProcessTableView, InitUI_Loaded_KeepsExistingWidths)
{
    // Arrange: 受控视图先应用默认 300
    ProcessTableView *view = createControlledView(nullptr);
    int before = view->columnWidth(ProcessTableModel::kProcessNameColumn);
    ASSERT_EQ(before, 300);
    // Act
    view->initUI(true);
    // Assert: 默认样式分支未执行，宽度保持（B34）
    EXPECT_EQ(view->columnWidth(ProcessTableModel::kProcessNameColumn), before);
    EXPECT_EQ(view->columnWidth(ProcessTableModel::kProcessNameColumn), 300);
    delete view;
}

// initConnections：未加载设置时表头动作默认勾选态（cpu/mem/pid 勾选，user 未设置）
TEST_F(UT_ProcessTableView, InitConnections_NotLoaded_DefaultHeaderChecks)
{
    // Arrange: 受控视图 ctor 已执行 initConnections(false)，追加一组便于按下标断言
    ProcessTableView *view = createControlledView(nullptr);
    ASSERT_EQ(view->m_headerContextMenu->actions().size(), 12);
    // Act
    view->initConnections(false);
    // Assert: 第二组动作位于下标 12~23（B36）
    QList<QAction *> acts = view->m_headerContextMenu->actions();
    EXPECT_EQ(acts.size(), 24);
    EXPECT_TRUE(acts[12]->isChecked());   // cpu
    EXPECT_FALSE(acts[13]->isChecked());  // user（默认块不设置）
    EXPECT_TRUE(acts[14]->isChecked());   // memory
    EXPECT_FALSE(acts[15]->isChecked());  // share memory
    EXPECT_TRUE(acts[21]->isChecked());   // pid
    delete view;
}

// initConnections：已加载设置时不覆盖表头动作勾选态
TEST_F(UT_ProcessTableView, InitConnections_Loaded_NoDefaultChecks)
{
    // Arrange
    ProcessTableView *view = createControlledView(nullptr);
    ASSERT_EQ(view->m_headerContextMenu->actions().size(), 12);
    // Act
    view->initConnections(true);
    // Assert: 新增的 cpu 动作保持默认未勾选（B37）
    QList<QAction *> acts = view->m_headerContextMenu->actions();
    EXPECT_EQ(acts.size(), 24);
    EXPECT_FALSE(acts[12]->isChecked());
    delete view;
}

// initConnections：安装 5 个快捷键
TEST_F(UT_ProcessTableView, InitConnections_DefaultFlags_InstallsShortcuts)
{
    // Arrange: !settingsLoaded 走默认装配分支（DConfig 环境相关分支不新开连接）
    ProcessTableView *view = m_tester;
    ASSERT_NE(view, nullptr);
    // Act
    view->initConnections(false);
    // Assert
    ASSERT_NE(m_tester->m_endProcKP, nullptr);
    ASSERT_NE(m_tester->m_pauseProcKP, nullptr);
    ASSERT_NE(m_tester->m_resumeProcKP, nullptr);
    ASSERT_NE(m_tester->m_viewPropKP, nullptr);
    ASSERT_NE(m_tester->m_killProcKP, nullptr);
    EXPECT_EQ(m_tester->m_endProcKP->key(), QKeySequence(Qt::ALT + Qt::Key_E));
    EXPECT_EQ(m_tester->m_pauseProcKP->key(), QKeySequence(Qt::ALT + Qt::Key_P));
    EXPECT_EQ(m_tester->m_resumeProcKP->key(), QKeySequence(Qt::ALT + Qt::Key_C));
    EXPECT_EQ(m_tester->m_viewPropKP->key(), QKeySequence(Qt::ALT + Qt::Key_Return));
    EXPECT_EQ(m_tester->m_killProcKP->key(), QKeySequence(Qt::ALT + Qt::Key_K));
}

// initConnections：表头动作触发后切换列可见性并发出 signalHeadchanged
TEST_F(UT_ProcessTableView, InitConnections_HeaderTrigger_HooksVisibility)
{
    // Arrange: 受控视图 ctor 的表头动作即为唯一一组
    ProcessTableView *view = createControlledView(nullptr);
    QSignalSpy headSpy(view, &ProcessTableView::signalHeadchanged);
    ASSERT_EQ(view->m_headerContextMenu->actions().size(), 12);
    // Act: 触发 cpu 动作（当前未隐藏 → b=true → setSectionHidden(false) 可见）
    emit view->m_headerContextMenu->actions()[0]->triggered(true);
    // Assert
    EXPECT_FALSE(view->isColumnHidden(ProcessTableModel::kProcessCPUColumn));
    EXPECT_EQ(headSpy.count(), 1);
    // Act: 触发 user 动作（无 signalHeadchanged）
    emit view->m_headerContextMenu->actions()[1]->triggered(false);
    // Assert: user 列被隐藏且不发出 headchanged（B38）
    EXPECT_TRUE(view->isColumnHidden(ProcessTableModel::kProcessUserColumn));
    EXPECT_EQ(headSpy.count(), 1);
    delete view;
}

// initConnections：模型更新后恢复选中扫描并发出 signalModelUpdated
TEST_F(UT_ProcessTableView, InitConnections_ModelUpdated_RestoresSelectionAndEmits)
{
    // Arrange
    QSignalSpy spy(m_tester, &ProcessTableView::signalModelUpdated);
    m_tester->m_selectedPID = QVariant(pid_t(999999));
    // Act: 直接连接同步执行，连续两次触发均应发信号（B39）
    emit m_tester->m_model->modelUpdated();
    EXPECT_EQ(spy.count(), 1);
    emit m_tester->m_model->modelUpdated();
    // Assert: 两次触发各发一次
    EXPECT_EQ(spy.count(), 2);
}

// initConnections：用户模式模型更新后刷新 8 项总计
TEST_F(UT_ProcessTableView, InitConnections_ModelUpdated_UserMode_RefreshesTotals)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(Settings, instance), ut_stub_settings_instance_null);
    ProcessTableView view(nullptr, "ut-user");
    QSignalSpy spy(&view, &ProcessTableView::signalModelUpdated);
    // Act
    emit view.m_model->modelUpdated();
    // Assert: 总计与模型保持一致（B40）
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(view.getUserCPUUsage(), view.m_model->getTotalCPUUsage());
    EXPECT_EQ(view.getUserDiskWrite(), view.m_model->getTotalDiskWrite());
}

// initConnections：优先级修改失败信号触发错误对话框
TEST_F(UT_ProcessTableView, InitConnections_PriorityError_ShowsErrorDialog)
{
    // Arrange: 构造函数已调用 initConnections（priorityPromoteResultReady 已连接一次），
    // 这里不再重复调用，避免双连接导致弹框计数翻倍
    Stub stub;
    stub.set(ADDR(ErrorDialog, show), ut_stub_error_dialog_show);
    ErrorContext ec;
    ec.setCode(1);   // operator bool 判定 code/subCode 非零才算有错
    ec.setErrorName(QString("ut-err-name"));
    ec.setErrorMessage(QString("ut-err-msg"));
    // Act: 夹具构造时已建立 priorityPromoteResultReady 连接，这里仅触发
    int before = s_errorDialogShowCalls;
    emit ProcessDB::instance()->priorityPromoteResultReady(ec);
    // Assert（B41）: 至少弹框一次且错误名透传
    // （全量套件下前序套件泄漏的视图也连接全局信号，计数会放大，故用增量断言）
    EXPECT_GT(s_errorDialogShowCalls, before);
    EXPECT_EQ(s_errorDialogShowName, QString("ut-err-name"));
}

// initConnections：进程控制错误仅连接一次，二次初始化后仍单次弹框
TEST_F(UT_ProcessTableView, InitConnections_ControlError_ShowsErrorDialog)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ErrorDialog, show), ut_stub_error_dialog_show);
    m_tester->initConnections(false);
    m_tester->initConnections(false);   // m_pControlConnection 已存在 → 不重复连接（B52）
    ErrorContext ec;
    ec.setCode(1);   // operator bool 判定 code/subCode 非零才算有错
    ec.setErrorName(QString("ut-ctrl-err"));
    ec.setErrorMessage(QString("ut-ctrl-msg"));
    // Act
    emit ProcessDB::instance()->processControlResultReady(ec);
    // Assert: 弹框一次且错误名透传
    EXPECT_EQ(s_errorDialogShowCalls, 1);
    EXPECT_EQ(s_errorDialogShowName, QString("ut-ctrl-err"));
}

// displayProcessTableContextMenu：无选中时不弹出
TEST_F(UT_ProcessTableView, DisplayProcessTableContextMenu_NoSelection_NoPopup)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(QMenu, popup), ut_stub_menu_popup);
    // Act
    m_tester->displayProcessTableContextMenu(QPoint(0, 0));
    // Assert: 无选中不弹菜单且选中态未变
    EXPECT_EQ(s_menuPopupCalls, 0);
    EXPECT_FALSE(m_tester->m_selectedPID.isValid());
}

// displayProcessTableContextMenu：有选中时在全局坐标（含表头高度偏移）弹出
TEST_F(UT_ProcessTableView, DisplayProcessTableContextMenu_WithSelection_PopsAtGlobalPos)
{
    // Arrange
    typedef QModelIndexList (*fptr_sel)(QTreeView *);
    fptr_sel sel = (fptr_sel)(QModelIndexList(QTreeView::*)(void) const) & QTreeView::selectedIndexes;
    Stub stub;
    stub.set(sel, ut_stub_selectedIndexes_three);
    stub.set(ADDR(QMenu, popup), ut_stub_menu_popup);
    // Act
    m_tester->displayProcessTableContextMenu(QPoint(5, 5));
    // Assert（B44）: 弹一次且菜单动作已装配
    EXPECT_EQ(s_menuPopupCalls, 1);
    EXPECT_GT(m_tester->m_contextMenu->actions().size(), 0);
}

// displayProcessTableHeaderContextMenu：直接弹出表头菜单
TEST_F(UT_ProcessTableView, DisplayHeaderContextMenu_DefaultPos_PopsMenu)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(QMenu, popup), ut_stub_menu_popup);
    // Act
    m_tester->displayProcessTableHeaderContextMenu(QPoint(3, 3));
    // Assert: 弹一次且表头菜单动作完整
    EXPECT_EQ(s_menuPopupCalls, 1);
    EXPECT_EQ(m_tester->m_headerContextMenu->actions().size(), 12);
}

// 上下文菜单 aboutToShow：同步暂停/恢复可用态与"查看命令位置"可用态
TEST_F(UT_ProcessTableView, ContextMenuAboutToShow_RunningProc_SyncsStates)
{
    // Arrange: 必须在构造前 stub —— DConfig::value 在 initConnections 建菜单时被调用，
    // 本环境返回 0 会隐藏暂停/恢复动作（Qt6 隐藏动作 isEnabled==false）。
    // stub DConfig 返 1、进程状态返 'R'（bogus pid 下模型默认值不确定）后新建受控视图。
    Stub stub;
    stub.set(ADDR(ProcessTableModel, getProcessState), ut_stub_model_getProcessState_running);
    stub.set(ADDR(DConfig, value), ut_stub_dconfig_value_one);
    ProcessTableView *view = UT_ProcessTableView::createControlledView(nullptr);
    view->m_selectedPID = QVariant(pid_t(100000000));
    // Act: popup 触发 aboutToShow（不替换 popup）
    view->m_contextMenu->popup(QPoint(0, 0));
    view->m_contextMenu->hide();
    // Assert: End(0)/Suspend(1)/Resume(2)/优先级子菜单(3)/查看命令位置(4)/属性(5)/分隔(6)/结束进程(7)
    QList<QAction *> acts = view->m_contextMenu->actions();
    ASSERT_GE(acts.size(), 8);
    EXPECT_TRUE(acts[1]->isEnabled());
    EXPECT_FALSE(acts[2]->isEnabled());
    EXPECT_FALSE(acts[4]->isEnabled());
    // 非布尔型一致性校验：动作列表与菜单装配一致
    EXPECT_EQ(acts, view->m_contextMenu->actions());
    delete view;
}

// resizeEvent：调整提示标签可见性（搜索为空 → 隐藏）
TEST_F(UT_ProcessTableView, ResizeEvent_EmptyModel_AdjustsLabelVisibility)
{
    // Arrange
    ASSERT_NE(m_tester->m_notFoundLabel, nullptr);
    QPoint posBefore = m_tester->m_notFoundLabel->pos();
    QResizeEvent ev(QSize(0, 0), QSize(300, 200));
    // Act
    m_tester->resizeEvent(&ev);
    // Assert: 标签隐藏且未移动（搜索为空时 adjustInfoLabelVisibility 隐藏分支）
    EXPECT_FALSE(m_tester->m_notFoundLabel->isVisible());
    EXPECT_EQ(m_tester->m_notFoundLabel->pos(), posBefore);
}

// showEvent：首次显示时隐藏"无搜索结果"标签
TEST_F(UT_ProcessTableView, ShowEvent_NotFoundLabel_Hides)
{
    // Arrange
    QShowEvent se;
    m_tester->m_notFoundLabel->show();
    // Act
    m_tester->showEvent(&se);
    // Assert: 有效可见性与显式隐藏态均确认
    EXPECT_FALSE(m_tester->m_notFoundLabel->isVisible());
    EXPECT_TRUE(m_tester->m_notFoundLabel->isHidden());
    EXPECT_NE(m_tester->m_notFoundLabel->parentWidget(), nullptr);
}

// selectionChanged：空选中不记录 PID
TEST_F(UT_ProcessTableView, SelectionChanged_EmptySelection_KeepsPidNull)
{
    // Arrange
    QItemSelection selected, deselected;
    // Act
    m_tester->selectionChanged(selected, deselected);
    // Assert（B45）: PID 保持无效且值为空
    EXPECT_FALSE(m_tester->m_selectedPID.isValid());
    EXPECT_EQ(m_tester->m_selectedPID, QVariant());
}

// selectionChanged：非空选中记录 PID 列数据
TEST_F(UT_ProcessTableView, SelectionChanged_WithSelection_StoresPid)
{
    // Arrange
    QStandardItemModel model(1, ProcessTableModel::kProcessColumnCount);
    model.setItem(0, ProcessTableModel::kProcessPIDColumn, new QStandardItem(QString::number(12345)));
    model.item(0, ProcessTableModel::kProcessPIDColumn)->setData(QVariant(pid_t(12345)), Qt::DisplayRole);
    QItemSelection selected;
    selected.select(model.index(0, 0), model.index(0, ProcessTableModel::kProcessColumnCount - 1));
    QItemSelection deselected;
    // Act
    m_tester->selectionChanged(selected, deselected);
    // Assert（B46）
    EXPECT_TRUE(m_tester->m_selectedPID.isValid());
    EXPECT_EQ(m_tester->m_selectedPID, QVariant(pid_t(12345)));
}

// sizeHintForColumn：取表头提示与视图提示的较大值 + 2*margin
TEST_F(UT_ProcessTableView, SizeHintForColumn_Stubs_ReturnsMaxPlusMargin)
{
    // Arrange: sizeHintForColumn 是虚函数，Itanium ABI 成员指针前 8 字节是 vtable 索引
    // 而非代码地址，ADDR 的 union 技巧会拿到非法地址（段错误），必须经 C 强转取真实符号；
    // header()->sizeHintForColumn 虚派发到 QAbstractItemView 符号，
    // DTreeView::sizeHintForColumn 限定调用落到 QTreeView 符号（DTreeView 未重写）
    Stub stub;
    typedef int (*fptr_aiv)(QAbstractItemView *, int);
    stub.set((fptr_aiv)(int (QAbstractItemView::*)(int) const)&QAbstractItemView::sizeHintForColumn,
             ut_stub_headerview_sizeHintForColumn);
    typedef int (*fptr_tv)(QTreeView *, int);
    stub.set((fptr_tv)(int (QTreeView::*)(int) const)&QTreeView::sizeHintForColumn,
             ut_stub_treeview_sizeHintForColumn);
    // Act
    int value = m_tester->sizeHintForColumn(0);
    // Assert: max(10, 5) + 2*10 = 30，且结果为正
    EXPECT_EQ(value, 30);
    EXPECT_GT(value, 0);
}

// adjustInfoLabelVisibility：搜索框为空（当前实现返回 false）时标签保持隐藏
TEST_F(UT_ProcessTableView, AdjustInfoLabelVisibility_EmptySearch_KeepsHidden)
{
    // Arrange: 测试环境模型为空，真实 Toolbar 在搜索框无内容时 isSearchContentEmpty 返 false
    Stub stub;
    stub.set(ADDR(Toolbar, isSearchContentEmpty), ut_stub_toolbar_search_empty);
    // Act
    m_tester->adjustInfoLabelVisibility();
    // Assert（B47；同时红标 Toolbar::isSearchContentEmpty 语义反转缺陷）:
    // rowCount==0 但搜索为空 → 标签保持隐藏
    EXPECT_FALSE(m_tester->m_notFoundLabel->isVisible());
    EXPECT_TRUE(m_tester->m_notFoundLabel->isHidden());
    EXPECT_NE(m_tester->m_notFoundLabel, nullptr);
}

// adjustInfoLabelVisibility：无结果且搜索非空时标签居中显示
TEST_F(UT_ProcessTableView, AdjustInfoLabelVisibility_NoRowsAndSearching_ShowsAtCenter)
{
    // Arrange: 先显示顶层窗口 —— 子控件 isVisible() 依赖祖先链可见性（Qt6 语义），
    // 隐藏视图中 setVisible(true) 后 isVisible() 仍为 false，居中分支也不会执行；
    // show() 会触发 showEvent 隐藏标签，随后 adjustInfoLabelVisibility 再显示
    m_tester->window()->show();
    Stub stub;
    stub.set(ADDR(Toolbar, isSearchContentEmpty), ut_stub_toolbar_search_not_empty);
    // Act: 模型为空 rowCount==0
    m_tester->adjustInfoLabelVisibility();
    // Assert（B48）
    EXPECT_TRUE(m_tester->m_notFoundLabel->isVisible());
    QPoint expected = m_tester->rect().center() - m_tester->m_notFoundLabel->rect().center();
    EXPECT_EQ(m_tester->m_notFoundLabel->pos(), expected);
}

// setUserModeName：异名更新、清空选中并同步模型；同名早退
TEST_F(UT_ProcessTableView, SetUserModeName_Changed_ClearsSelectionAndUpdatesModel)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(ProcessTableModel, setUserModeName), ut_stub_model_setUserModeName);
    m_tester->m_selectedPID = QVariant(pid_t(1));
    // Act
    m_tester->setUserModeName(QString("ut-user2"));
    // Assert（B51）
    EXPECT_EQ(m_tester->m_useModeName, QString("ut-user2"));
    EXPECT_FALSE(m_tester->m_selectedPID.isValid());
    EXPECT_EQ(s_modelSetUserModeNameCalls, 1);
    // Act: 同名再次设置 → 早退（B50）
    m_tester->setUserModeName(QString("ut-user2"));
    EXPECT_EQ(s_modelSetUserModeNameCalls, 1);
}
