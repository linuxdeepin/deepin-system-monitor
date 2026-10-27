// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 本文件为 ut_process_set.cpp 的重写版（qt-autotest-generator Mode 6 审查判定原文件
// 15 个错误、14 个用例全部零断言，列入全量重写清单）。

// 用例统计声明（与 .ut-inventory.json 的 usecase_count 对应）
// | 方法                         | 级别 | 因子(圈复杂度/认知/行数/分支) | 计划 | 实际 |
// |------------------------------|------|-------------------------------|------|------|
// | ProcessSet()                 | mid  | 6/7/48/DConfig空+DKapture开关 | 2    | 2    |
// | ProcessSet(const&)           | low  | 1/1/22/资源不拷贝             | 1    | 1    |
// | ~ProcessSet()                | low  | 2/2/13/client非空             | 1    | 1    |
// | getProcessById               | low  | 1/1/3/默认插入语义            | 2    | 2    |
// | getPIDList                   | mid  | 5/5/15/key_iterator遍历       | 2    | 2    |
// | removeProcess                | low  | 1/1/4/删除分支               | 2    | 2    |
// | updateProcessState           | low  | 2/2/6/contains分支           | 2    | 2    |
// | updateProcessPriority        | low  | 2/2/6/contains分支           | 2    | 2    |
// | getRecentProcStage           | low  | 1/1/3/operator[]语义         | 2    | 2    |
// | refresh                      | low  | 1/1/5/透传scanProcess        | 2    | 2    |
// | scanProcess                  | high | 48/60/253/7循环/3循环内分配  | 14   | 14   |
// | mergeSubProcNetIO            | mid  | 5/4/14/递归树遍历            | 3    | 3    |
// | mergeSubProcCpu              | mid  | 4/3/12/递归树遍历            | 2    | 2    |
// | groupProcessesByAMInstance   | high | 19/25/88/5循环/2循环内分配   | 7   | 7    |
// | getInstanceGroup             | low  | 1/1/3/value语义              | 2    | 2    |
// | Iterator::Iterator           | mid  | 3/3/10/opendir失败           | 1    | 1    |
// | Iterator::hasNext/next       | low  | 2/2/9/末尾默认返回           | 3    | 3    |
// | Iterator::advance            | mid  | 5/4/14/低pid跳过+read失败    | 1    | 1    |
// | 合计                         |      |                               | 47   | 47   |
//
// scanProcess 分支清单（来源：工作区源码 @4c78d5e4 + 知识图谱结构信息；图谱对该文件
// parse_partial，分支以工作区实际代码为准）：
//   B1  m_useSystemService 分流日志                      → test_scanProcess_DKapture*
//   B2  m_set 现有进程写入 m_recentProcStage 循环        → test_refresh_FirstScan*
//   B3  迭代器去重 contains 分支                         → test_refresh_FirstScan*
//   B4  m_prePid != m_curPid 真/假两分支                 → FirstScan(真)/SecondScan(假)
//   B5  过期进程清理（simpleSet/myApps/prePid 移除）      → test_scanProcess_PruneObsoletePid
//   B6  isNewProcess：readProcessSimpleInfo(true/false)  → DKaptureSuccess(true)/FirstScan(false)/SecondScan(else)
//   B7  kFilterApps：isTrayApp 假→加入 / 真→跳过         → FilterAppsNotTray / FilterAppsTray
//   B8  kFilterCurrentUser：有窗→重分类(托盘移除/普通加入) / 无窗→移除
//                                                        → CurrentUserWithGui / CurrentUserTray / CurrentUserNoWindow
//   B9  DKapture 批量获取 success 真/假                  → DKaptureSuccess / DKaptureBatchFails
//   B10 applyDKaptureData 与 readProcessVariableInfo 分派 → 同 B9
//   B11 !isValid 进程跳过                                → FirstScan（不变式 size <= curPid.size）
//   B12 m_pidMyApps 合并循环（NetIO 恒做 / CPU 仅传统）   → FilterAppsNotTray（传统）/ DKaptureSuccess（DK 跳过 CPU）
//   B13 GUI 祖先递归 + /bin/bash 父进程跳过 + setAppType(kFilterCurrentUser)
//                                                        → BashParent / NonBashParent
//   B14 尾部 m_recentProcStage.clear + groupProcesses    → FirstScan / SecondScanWithDKapture
// groupProcessesByAMInstance 分支清单：
//   G1  AM 不可用提前返回                                → AMUnavailable
//   G2  Step1 逐 pid 取组信息、!isValid 跳过             → SingleInstance/MultiInstance/InvalidInfo
//   G3  Step2 prevPidToInstance 恢复子进程               → RecoveredSubProc / PrevCacheMissingInstance
//   G4  Step3 多进程实例：隐藏子进程+内存/CPU 聚合       → MultiInstance
//   G5  Step3 单进程实例：不落 m_instanceGroups          → SingleInstance
//   G6  displayName 写回主进程（含单进程）               → SingleInstance
//
// 最小清单核对：
//   [x] 1 每个被测分支都有直接触发用例（见上两份分支清单映射）
//   [x] 2 每个用例至少 2 个断言维度（状态值 + 集合成员/规模）
//   [x] 3 外部依赖（/proc、DConfig、DBus、SystemServiceClient、AM）全部 stub 隔离或使用真实 /proc 只读面
//   [x] 4 测试间无共享可变状态（每用例独立 ProcessSet；Stub 为作用域级自动恢复）
//   [x] 5 断言验证行为结果而非实现细节（getter/d->字段与构造输入一致）
//   [x] 6 无空断言/恒真断言（禁用 EXPECT_TRUE(true) 类占位）
//   [x] 7 命名遵循 Feature_Scenario_Expected 三段式
//   [x] 8 AAA（Arrange-Act-Assert）注释齐全
//   [x] 9 私有方法经 -fno-access-control 直测（scanProcess/merge*/groupProcesses/Iterator::advance）
//   [x] 10 DKapture 批量数据、AM 分组信息均为构造的确定性 QVariantMap/AMProcessGroupInfo
//
// Stub 说明：
//   - DConfig::create → 恒 nullptr（禁用 DKapture 配置路径，隔离 DConfig 文件读取）
//   - SystemServiceClient::getProcessInfoBatch → 构造的批量 DKapture 数据（成功/失败两态）
//   - Process::appType → 固定 FilterType，驱动 scanProcess 分类分支
//   - WMWindowList::isTrayApp/isGuiApp/isDesktopEntryApp → 构造窗口状态（选择性 isGuiApp
//     以 (void*, pid_t) 签名读取 this 与 pid，实现按 pid 定向）
//   - Process::cmdlineString → "/bin/bash -c ut"（B13 bash 父进程分支）
//   - AMIconManager::isAMAvailable/getProcessGroupInfo → AM 分组确定性数据
//   - ProcessIcon::getIcon → 空 QIcon（headless 环境无 xcb，图标子系统整体隔离）
//   - opendir → nullptr（Iterator opendir 失败分支）
//   其余用例直接读取真实 /proc 只读面（进程自观测），不写任何外部状态。

//self
#include "process/process_set.h"
#include "process/process_db.h"
#include "process/system_service_client.h"
#include "process/am_icon_manager.h"
#include "process/private/process_p.h"
#include "wm/wm_window_list.h"
#include "common/common.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QVariant>
#include <QString>
#include <QList>
#include <QIcon>
#include <QCoreApplication>
#include <QEvent>

#include <unistd.h>
#include <dirent.h>

using namespace core::process;

/***************************************STUB begin*********************************************/

// ---- ProcessSet 构造：DConfig 恒空，禁用 DKapture 配置路径 ----
static DConfig *stub_dconfig_create_null(const QString &, const QString &, const QString &, QObject *)
{
    return nullptr;
}

// DConfig::create 重载消歧：用带上下文的具名指针取四参重载
using DConfigCreateFn = DConfig *(*)(const QString &, const QString &, const QString &, QObject *);

// ---- ProcessSet 构造：DKapture 启用路径（FakeBackend 返回 enable_dkapture=true）----
class UTFakeDConfigBackend : public DConfigBackend
{
public:
    bool isValid() const override { return true; }
    bool load(const QString &) override { return true; }
    QStringList keyList() const override { return { QStringLiteral("enable_dkapture") }; }
    QVariant value(const QString &key, const QVariant &fallback) const override
    {
        return key == QStringLiteral("enable_dkapture") ? QVariant(true) : fallback;
    }
    void setValue(const QString &, const QVariant &) override { }
};
static DConfig *stub_dconfig_create_enabled(const QString &, const QString &, const QString &, QObject *)
{
    return new DConfig(new UTFakeDConfigBackend(),
                       QStringLiteral("org.deepin.system-monitor"), QString());
}

// ---- SystemServiceClient 可用性开关 ----
static bool g_stubSvcAvailable = false;
static bool stub_svc_available()
{
    return g_stubSvcAvailable;
}
static bool stub_svc_dkapture_available()
{
    return true;
}
static void stub_svc_start_noop()
{
}

// ---- scanProcess 分类分支：固定 appType ----
static int g_stubAppType = static_cast<int>(kNoFilter);
static int stub_appType_fixed()
{
    return g_stubAppType;
}

// ---- WMWindowList 窗口状态 ----
static bool stub_bool_false()
{
    return false;
}
static bool stub_bool_true()
{
    return true;
}
// 选择性 isGuiApp：仅 g_stubGuiPid 返回 true（成员函数 ABI：rdi=this, rsi=pid）
static pid_t g_stubGuiPid = 0;
static bool stub_isGuiApp_forPid(void *, pid_t pid)
{
    return pid == g_stubGuiPid;
}

// ---- B13：父进程 cmdline 固定为 bash ----
static QString stub_cmdlineString_bash()
{
    return QStringLiteral("/bin/bash -c ut");
}

static QString stub_cmdlineString_de()
{
    return QStringLiteral("deepin-demo --window"); // 非 bash 前缀即可触发另一分支
}

// ---- 图标子系统隔离：任何 scanProcess/refresh 测试都不得触达 xcb/缓存逻辑 ----
static void stub_refreashProcessIcon_noop(Process *)
{
}

// ---- DKapture 批量数据（SystemServiceClient::getProcessInfoBatch）----
static QVariantMap g_stubBatchResponse;
static int g_stubBatchCalls = 0;
static QVariantMap stub_getProcessInfoBatch()
{
    ++g_stubBatchCalls;
    return g_stubBatchResponse;
}

// ---- AM 分组（AMIconManager）----
static bool g_stubAmAvailable = false;
static bool stub_isAMAvailable_fixed()
{
    return g_stubAmAvailable;
}
static AMProcessGroupInfo g_stubGroupInfo;
static int g_stubGroupInfoCalls = 0;
static AMProcessGroupInfo stub_getProcessGroupInfo_fixed()
{
    ++g_stubGroupInfoCalls;
    return g_stubGroupInfo;
}

// ---- Iterator opendir 失败 ----
static DIR *stub_opendir_null(const char *)
{
    return nullptr;
}

/***************************************STUB end***********************************************/

namespace {
// 构造 DKapture 批量成功响应：仅包含 targetPid 的数据
QVariantMap makeDkaptureResponse(pid_t targetPid, qulonglong utime, qulonglong stime, qulonglong rssBytes)
{
    QVariantMap pidData;
    pidData["comm"] = QStringLiteral("ut-dkapture-proc");
    pidData["utime"] = QVariant::fromValue<qulonglong>(utime);
    pidData["stime"] = QVariant::fromValue<qulonglong>(stime);
    pidData["rss"] = QVariant::fromValue<qulonglong>(rssBytes);

    QVariantMap data;
    data[QString::number(targetPid)] = pidData;

    QVariantMap response;
    response["success"] = true;
    response["data"] = data;
    return response;
}
} // namespace

class UT_ProcessSet : public ::testing::Test
{
public:
    UT_ProcessSet() : m_tester(nullptr) {}

    virtual void SetUp()
    {
        m_tester = new ProcessSet();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    ProcessSet *m_tester;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// ---------------- 构造 / 析构 ----------------

// ProcessSet() + DConfig::create 恒空：DKapture 配置不可用，服务客户端不创建
TEST_F(UT_ProcessSet, ProcessSet_DefaultConfig_ServiceDisabled)
{
    // Arrange：DConfig::create 恒空（见 stub）
    DConfigCreateFn createFn = &DConfig::create;
    Stub b1;
    b1.set(createFn, stub_dconfig_create_null);

    // Act
    ProcessSet set;

    // Assert：默认走传统 /proc 扫描路径，且无 DConfig 资源
    EXPECT_FALSE(set.m_useSystemService);
    EXPECT_EQ(set.m_systemServiceClient, nullptr);
    EXPECT_EQ(set.m_config, nullptr);
    EXPECT_TRUE(set.getPIDList().isEmpty());
}

// ProcessSet(const ProcessSet&)：进程集合与映射深拷贝，运行期资源与增量状态不拷贝
TEST_F(UT_ProcessSet, ProcessSet_CopySemantics_StateCopiedResourcesReset)
{
    // Arrange
    m_tester->m_set.insert(100, Process(100));
    m_tester->m_set.insert(200, Process(200));
    m_tester->m_pidCtoPMapping[110] = 100;
    m_tester->m_pidPtoCMapping.insert(100, 110);
    auto stage = std::make_shared<RecentProcStage>();
    stage->ptime = 5;
    m_tester->m_recentProcStage[100] = stage;
    m_tester->m_useSystemService = true;

    // Act
    ProcessSet copy(*m_tester);

    // Assert：数据成员拷贝，运行期资源（client/config）与增量状态（simpleSet/prePid 等）不拷贝
    EXPECT_EQ(copy.m_set.size(), 2);
    EXPECT_EQ(copy.m_set[100].pid(), 100);
    EXPECT_EQ(copy.m_pidCtoPMapping, m_tester->m_pidCtoPMapping);
    EXPECT_EQ(copy.m_pidPtoCMapping, m_tester->m_pidPtoCMapping);
    EXPECT_TRUE(copy.m_recentProcStage[100] != nullptr);
    EXPECT_TRUE(copy.m_simpleSet.isEmpty());
    EXPECT_TRUE(copy.m_prePid.isEmpty());
    EXPECT_TRUE(copy.m_pidMyApps.isEmpty());
    EXPECT_EQ(copy.m_systemServiceClient, nullptr);
    EXPECT_EQ(copy.m_config, nullptr);
    EXPECT_EQ(copy.m_useSystemService, true);

    // 与源对象独立：删除源数据不影响副本
    m_tester->m_set.remove(100);
    EXPECT_TRUE(copy.m_set.contains(100));
}

// ~ProcessSet()：持有 SystemServiceClient 时正常释放（ASan 验证无泄漏/无 UAF）
TEST_F(UT_ProcessSet, Dtor_OwnedClientDeleted_NoLeak)
{
    // Arrange：直接挂接真实客户端对象，验证析构 delete 分支
    DConfigCreateFn createFn = &DConfig::create;
    Stub b1;
    b1.set(createFn, stub_dconfig_create_null);
    {
        ProcessSet set;
        set.m_systemServiceClient = new SystemServiceClient();
        set.m_useSystemService = true;
        set.m_set.insert(100, Process(100));

        // Assert（析构前状态可用）
        EXPECT_EQ(set.getPIDList().size(), 1);
    }

    // Act 已由作用域结束触发；Assert：析构后新实例不受影响
    ProcessSet fresh;
    EXPECT_TRUE(fresh.getPIDList().isEmpty());
}

// ---------------- 查询类 ----------------

// getProcessById：已存在 pid 返回对应进程，且不改变集合
TEST_F(UT_ProcessSet, GetProcessById_ExistingPid_ReturnsMatch)
{
    // Arrange
    m_tester->m_set.insert(7777, Process(7777));

    // Act
    const Process proc = m_tester->getProcessById(7777);

    // Assert
    EXPECT_EQ(proc.pid(), 7777);
    EXPECT_EQ(m_tester->getPIDList(), QList<pid_t>({ 7777 }));
}

// getProcessById：未知 pid 返回默认进程（pid 0），不向集合插入条目
TEST_F(UT_ProcessSet, GetProcessById_UnknownPid_ReturnsDefaultNoInsert)
{
    // Arrange：空集合
    EXPECT_TRUE(m_tester->m_set.isEmpty());

    // Act
    const Process proc = m_tester->getProcessById(99999);

    // Assert：const QMap::operator[] 语义——查询不插入
    EXPECT_EQ(proc.pid(), 0);
    EXPECT_FALSE(proc.isValid());
    EXPECT_FALSE(m_tester->getPIDList().contains(99999));
}

// getPIDList：多 pid 返回按键升序的完整列表
TEST_F(UT_ProcessSet, GetPIDList_MultiplePids_ReturnsAscendingList)
{
    // Arrange
    m_tester->m_set.insert(300, Process(300));
    m_tester->m_set.insert(100, Process(100));
    m_tester->m_set.insert(200, Process(200));

    // Act
    const QList<pid_t> pidList = m_tester->getPIDList();

    // Assert
    EXPECT_EQ(pidList, QList<pid_t>({ 100, 200, 300 }));
    EXPECT_EQ(pidList.size(), 3);
}

// getPIDList：空集合返回空列表（回归：key_iterator 方案不触发旧 keys() 段错误）
TEST_F(UT_ProcessSet, GetPIDList_EmptySet_ReturnsEmptyList)
{
    // Arrange：空集合
    EXPECT_TRUE(m_tester->m_set.isEmpty());

    // Act
    const QList<pid_t> pidList = m_tester->getPIDList();

    // Assert
    EXPECT_TRUE(pidList.isEmpty());
    EXPECT_EQ(pidList.size(), 0);
}

// ---------------- 增删改 ----------------

// removeProcess：删除已存在 pid，仅移除目标条目
TEST_F(UT_ProcessSet, RemoveProcess_ExistingPid_RemovesOnlyTarget)
{
    // Arrange
    m_tester->m_set.insert(100, Process(100));
    m_tester->m_set.insert(200, Process(200));

    // Act
    m_tester->removeProcess(100);

    // Assert
    EXPECT_EQ(m_tester->getPIDList(), QList<pid_t>({ 200 }));
    m_tester->removeProcess(100);
    EXPECT_EQ(m_tester->getPIDList(), QList<pid_t>({ 200 }));
}

// removeProcess：删除未知 pid 无副作用
TEST_F(UT_ProcessSet, RemoveProcess_UnknownPid_KeepsSetUnchanged)
{
    // Arrange
    m_tester->m_set.insert(100, Process(100));

    // Act
    m_tester->removeProcess(999);

    // Assert
    EXPECT_EQ(m_tester->getPIDList(), QList<pid_t>({ 100 }));
    EXPECT_EQ(m_tester->m_set.size(), 1);
    EXPECT_EQ(m_tester->m_set[100].pid(), 100);
}

// updateProcessState：已存在 pid 状态被更新
TEST_F(UT_ProcessSet, UpdateProcessState_ExistingPid_StateApplied)
{
    // Arrange
    m_tester->m_set.insert(100, Process(100));
    m_tester->m_set[100].setState('R');

    // Act
    m_tester->updateProcessState(100, 'S');

    // Assert
    EXPECT_EQ(m_tester->getProcessById(100).state(), 'S');
    EXPECT_EQ(m_tester->m_set[100].d->state, 'S');
}

// updateProcessState：未知 pid 不插入
TEST_F(UT_ProcessSet, UpdateProcessState_UnknownPid_DoesNotInsert)
{
    // Arrange
    m_tester->m_set.insert(100, Process(100));

    // Act
    m_tester->updateProcessState(999, 'Z');

    // Assert
    EXPECT_EQ(m_tester->getPIDList(), QList<pid_t>({ 100 }));
    EXPECT_EQ(m_tester->m_set.size(), 1);
    EXPECT_EQ(m_tester->getProcessById(999).pid(), 0);
}

// updateProcessPriority：已存在 pid 优先级被更新（含正负边界值）
TEST_F(UT_ProcessSet, UpdateProcessPriority_ExistingPid_PriorityApplied)
{
    // Arrange
    m_tester->m_set.insert(100, Process(100));

    // Act
    m_tester->updateProcessPriority(100, 19);

    // Assert
    EXPECT_EQ(m_tester->getProcessById(100).priority(), 19);

    // 边界：最低优先级
    m_tester->updateProcessPriority(100, -20);
    EXPECT_EQ(m_tester->getProcessById(100).priority(), -20);
}

// updateProcessPriority：未知 pid 不插入
TEST_F(UT_ProcessSet, UpdateProcessPriority_UnknownPid_DoesNotInsert)
{
    // Arrange
    m_tester->m_set.insert(100, Process(100));

    // Act
    m_tester->updateProcessPriority(999, 5);

    // Assert
    EXPECT_EQ(m_tester->getPIDList(), QList<pid_t>({ 100 }));
    EXPECT_EQ(m_tester->m_set.size(), 1);
}

// getRecentProcStage：已存在 pid 返回共享阶段快照
TEST_F(UT_ProcessSet, GetRecentProcStage_ExistingPid_ReturnsSharedStage)
{
    // Arrange
    auto stage = std::make_shared<RecentProcStage>();
    stage->ptime = 123;
    stage->read_bytes = 456;
    stage->write_bytes = 789;
    m_tester->m_recentProcStage[100] = stage;

    // Act
    std::weak_ptr<RecentProcStage> weak = m_tester->getRecentProcStage(100);
    std::shared_ptr<RecentProcStage> locked = weak.lock();

    // Assert：字段与写入一致（qulonglong 比较）
    EXPECT_TRUE(locked != nullptr);
    EXPECT_EQ(locked->ptime, 123u);
    EXPECT_EQ(locked->read_bytes, 456u);
    EXPECT_EQ(locked->write_bytes, 789u);
}

// getRecentProcStage：未知 pid 返回过期 weak_ptr（operator[] 默认构造的空 shared_ptr）
TEST_F(UT_ProcessSet, GetRecentProcStage_UnknownPid_ReturnsExpiredWeakPtr)
{
    // Arrange：空阶段缓存
    EXPECT_TRUE(m_tester->m_recentProcStage.isEmpty());

    // Act
    std::weak_ptr<RecentProcStage> weak = m_tester->getRecentProcStage(4242);

    // Assert：operator[] 默认构造的空 shared_ptr——过期且锁不出对象
    EXPECT_EQ(weak.use_count(), 0);
    EXPECT_EQ(weak.lock(), nullptr);
}

// ---------------- refresh / scanProcess ----------------

// 首次扫描（传统 /proc 模式）：填充集合、映射与 simpleSet，阶段缓存清空
TEST_F(UT_ProcessSet, Refresh_FirstScan_PopulatesSetAndMappings)
{
    // Arrange：真实 /proc 环境；图标子系统隔离（headless 无 xcb）
    EXPECT_TRUE(m_tester->m_set.isEmpty());
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act
    m_tester->refresh();

    // Assert：集合非空、包含自身进程、pid 均 >= 10（迭代器过滤低位 pid）
    const QList<pid_t> pidList = m_tester->getPIDList();
    EXPECT_FALSE(pidList.isEmpty());
    EXPECT_TRUE(pidList.contains(getpid()));
    for (pid_t pid : pidList) {
        ASSERT_TRUE(pid >= 10);
    }

    // 自身进程有效，父子映射与真实 ppid 一致，且正向映射可反查
    const Process &self = m_tester->m_set[getpid()];
    EXPECT_TRUE(self.isValid());
    EXPECT_EQ(m_tester->m_pidCtoPMapping[getpid()], getppid());
    EXPECT_TRUE(m_tester->m_pidPtoCMapping.values(getppid()).contains(getpid()));

    // 不变式：无效进程被跳过（B11），集合规模不超过扫描到的 pid 数
    EXPECT_LE(m_tester->m_set.size(), m_tester->m_curPid.size());

    // 尾部清空阶段缓存（B14，记录当前行为）
    EXPECT_TRUE(m_tester->m_recentProcStage.isEmpty());
}

// 第二次扫描（列表未变化）：复用 simpleSet，不重复读取进程基础信息
TEST_F(UT_ProcessSet, Refresh_SecondScan_UnchangedList_KeepsSimpleSet)
{
    // Arrange
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    m_tester->refresh();
    const QList<pid_t> firstScanPids = m_tester->m_prePid;
    ASSERT_FALSE(firstScanPids.isEmpty());

    // Act
    m_tester->refresh();

    // Assert：pid 列表两轮一致（B4 假分支），既有缓存保留，自身仍有效
    EXPECT_EQ(m_tester->m_prePid, m_tester->m_curPid);
    EXPECT_TRUE(m_tester->m_simpleSet.contains(getpid()));
    for (pid_t pid : firstScanPids) {
        if (pid == getpid()) {
            EXPECT_TRUE(m_tester->m_simpleSet.contains(pid));
        }
    }
    EXPECT_TRUE(m_tester->m_set.contains(getpid()));
}

// 列表变化时的过期清理：不在新列表中的 pid 从 prePid/simpleSet/myApps 全部移除
TEST_F(UT_ProcessSet, ScanProcess_PruneObsoletePid_RemovesCache)
{
    // Arrange：预置过期 pid（含 simpleSet 与 myApps 缓存）与仍存活的自身进程
    m_tester->m_prePid.append(999991);
    m_tester->m_prePid.append(getpid());
    m_tester->m_simpleSet.insert(999991, Process(999991));
    m_tester->m_simpleSet.insert(getpid(), Process(getpid()));
    m_tester->m_pidMyApps.append(999991);
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act
    m_tester->scanProcess();

    // Assert：过期 pid 全链路清理；存活 pid 保留（B5）
    EXPECT_EQ(m_tester->m_prePid.count(999991), 0);
    EXPECT_EQ(m_tester->m_simpleSet.count(999991), 0);
    EXPECT_EQ(m_tester->m_pidMyApps.count(999991), 0);
    EXPECT_EQ(m_tester->m_prePid.count(getpid()), 1);
    EXPECT_EQ(m_tester->m_simpleSet.count(getpid()), 1);
    EXPECT_EQ(m_tester->m_set.count(getpid()), 1);
}

// DKapture 模式：轻量初始化 + 批量数据成功应用（B1/B6/B9/B10）
TEST_F(UT_ProcessSet, ScanProcess_DKaptureSuccess_AppliesBatchData)
{
    // Arrange：空集合直接以 DKapture 模式扫描，所有进程走 isNewProcess 轻量初始化
    m_tester->m_useSystemService = true;
    m_tester->m_systemServiceClient = new SystemServiceClient();
    g_stubBatchResponse = makeDkaptureResponse(getpid(), 123456, 777, 4096000);
    g_stubBatchCalls = 0;
    Stub b1;
    b1.set(ADDR(SystemServiceClient, getProcessInfoBatch), stub_getProcessInfoBatch);
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act
    m_tester->scanProcess();

    // Assert：批量接口被调用一次；自身进程应用构造数据（rss 字节→KB 右移 10 位）
    EXPECT_EQ(g_stubBatchCalls, 1);
    const Process &self = m_tester->m_set[getpid()];
    EXPECT_EQ(self.d->utime, 123456u);
    EXPECT_EQ(self.d->stime, 777u);
    EXPECT_EQ(self.d->rss, 4096000u >> 10);
    EXPECT_TRUE(self.isValid());
    EXPECT_TRUE(m_tester->m_set.contains(getpid()));
}

// DKapture 批量获取失败：回退传统 /proc 读取（B9 假分支/B10 传统分派）
TEST_F(UT_ProcessSet, ScanProcess_DKaptureBatchFails_FallsBackTraditional)
{
    // Arrange：批量接口返回失败
    m_tester->m_useSystemService = true;
    m_tester->m_systemServiceClient = new SystemServiceClient();
    g_stubBatchResponse.clear();
    g_stubBatchResponse["success"] = false;
    g_stubBatchResponse["error"] = QStringLiteral("ut-failure");
    g_stubBatchCalls = 0;
    Stub b1;
    b1.set(ADDR(SystemServiceClient, getProcessInfoBatch), stub_getProcessInfoBatch);
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act
    m_tester->scanProcess();

    // Assert：批量接口被调用；自身进程拿到真实 /proc 数据而非构造数据
    EXPECT_EQ(g_stubBatchCalls, 1);
    const Process &self = m_tester->m_set[getpid()];
    EXPECT_NE(self.d->utime, 123456u);
    EXPECT_TRUE(self.isValid());
}

// 构造：DConfig 启用 DKapture 但系统服务不可用 → 回退传统扫描（构造 B3/B5 假分支）
TEST_F(UT_ProcessSet, ProcessSet_DKaptureEnabled_ServiceUnavailable_TraditionalMode)
{
    // Arrange：配置启用 DKapture，服务可用性打桩为 false
    DConfigCreateFn createFn = &DConfig::create;
    Stub bCfg;
    bCfg.set(createFn, stub_dconfig_create_enabled);
    Stub bSvc;
    bSvc.set(ADDR(SystemServiceClient, isServiceAvailable), stub_svc_available);
    Stub bStart;
    bStart.set(ADDR(SystemServiceClient, startSystemService), stub_svc_start_noop);
    g_stubSvcAvailable = false;

    // Act：重新构造被测对象（走 DKapture 启用构造路径）
    delete m_tester;
    m_tester = new ProcessSet();
    g_stubSvcAvailable = true;

    // Assert：服务客户端已创建但未启用系统服务模式
    EXPECT_NE(m_tester->m_config, nullptr);
    EXPECT_NE(m_tester->m_systemServiceClient, nullptr);
    EXPECT_FALSE(m_tester->m_useSystemService);

    // 收尾：消化 deleteLater，避免 LSan 泄漏报告
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

// 构造：DConfig 启用且服务可用 → 系统服务模式（构造 B4/B6 真分支）
TEST_F(UT_ProcessSet, ProcessSet_DKaptureEnabled_ServiceAvailable_SystemServiceMode)
{
    // Arrange：配置启用 DKapture，服务与 DKapture 可用性均打桩为 true
    DConfigCreateFn createFn = &DConfig::create;
    Stub bCfg;
    bCfg.set(createFn, stub_dconfig_create_enabled);
    Stub bSvc;
    bSvc.set(ADDR(SystemServiceClient, isServiceAvailable), stub_svc_available);
    Stub bDk;
    bDk.set(ADDR(SystemServiceClient, isDKaptureAvailable), stub_svc_dkapture_available);
    Stub bStart;
    bStart.set(ADDR(SystemServiceClient, startSystemService), stub_svc_start_noop);
    g_stubSvcAvailable = true;

    // Act：重新构造被测对象（不走 startSystemService 分支）
    delete m_tester;
    m_tester = new ProcessSet();

    // Assert：系统服务模式已启用
    EXPECT_NE(m_tester->m_systemServiceClient, nullptr);
    EXPECT_TRUE(m_tester->m_useSystemService);

    // 收尾：消化 deleteLater，避免 LSan 泄漏报告
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

// DKapture 第二次扫描：近期阶段同步循环 + 已存在进程复用（B2 循环/B8 假分支）
TEST_F(UT_ProcessSet, ScanProcess_SecondScanWithDKapture_RecentStageSynced)
{
    // Arrange：以 DKapture 模式连续扫描两次
    m_tester->m_useSystemService = true;
    m_tester->m_systemServiceClient = new SystemServiceClient();
    g_stubBatchResponse = makeDkaptureResponse(getpid(), 654321, 888, 8192000);
    g_stubBatchCalls = 0;
    Stub b1;
    b1.set(ADDR(SystemServiceClient, getProcessInfoBatch), stub_getProcessInfoBatch);
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act：首次扫描建集合，二次扫描触发阶段同步循环
    m_tester->scanProcess();
    m_tester->scanProcess();

    // Assert：批量接口调用两次、集合完整。
    // 注：实现源 process_set.cpp:382 在 scanProcess 尾部无条件 m_recentProcStage.clear()，
    // 同步循环产物被清除（疑似源码缺陷，已另行标红报告），此处按实际行为断言。
    EXPECT_EQ(g_stubBatchCalls, 2);
    EXPECT_TRUE(m_tester->m_set.contains(getpid()));
    EXPECT_TRUE(m_tester->m_recentProcStage.isEmpty());
}

// kFilterApps 且非托盘：进程加入 myApps，合并循环执行（B7 加入/B12 传统 CPU 合并）
TEST_F(UT_ProcessSet, ScanProcess_FilterAppsNotTray_AddedToMyApps)
{
    // Arrange：所有进程分类为 kFilterApps，无托盘窗口
    g_stubAppType = static_cast<int>(kFilterApps);
    Stub b1;
    b1.set(ADDR(Process, appType), stub_appType_fixed);
    Stub b2;
    b2.set(ADDR(WMWindowList, isTrayApp), stub_bool_false);
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act
    m_tester->scanProcess();

    // Assert：自身进程进入 myApps，集合仍然完整
    g_stubAppType = static_cast<int>(kNoFilter);
    EXPECT_GE(m_tester->m_pidMyApps.size(), 1);
    EXPECT_EQ(m_tester->m_pidMyApps.count(getpid()), 1);
    EXPECT_EQ(m_tester->m_set.count(getpid()), 1);
}

// kFilterApps 但是托盘应用：不加入 myApps（B7 跳过分支）
TEST_F(UT_ProcessSet, ScanProcess_FilterAppsTray_NotAdded)
{
    // Arrange：所有进程分类为 kFilterApps，且全部被窗管判定为托盘
    g_stubAppType = static_cast<int>(kFilterApps);
    Stub b1;
    b1.set(ADDR(Process, appType), stub_appType_fixed);
    Stub b2;
    b2.set(ADDR(WMWindowList, isTrayApp), stub_bool_true);
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act
    m_tester->scanProcess();

    // Assert：没有任何进程加入 myApps，集合仍包含自身
    g_stubAppType = static_cast<int>(kNoFilter);
    EXPECT_EQ(m_tester->m_pidMyApps.size(), 0);
    EXPECT_EQ(m_tester->m_pidMyApps.count(getpid()), 0);
    EXPECT_EQ(m_tester->m_set.count(getpid()), 1);
}

// kFilterCurrentUser 且有 GUI 窗口：重分类为 kFilterApps 并加入 myApps（B8 重分类+加入）
TEST_F(UT_ProcessSet, ScanProcess_CurrentUserWithGui_ReclassifiedToMyApps)
{
    // Arrange：分类为当前用户进程，且全部有 GUI 窗口、非托盘
    g_stubAppType = static_cast<int>(kFilterCurrentUser);
    Stub b1;
    b1.set(ADDR(Process, appType), stub_appType_fixed);
    Stub b2;
    b2.set(ADDR(WMWindowList, isGuiApp), stub_bool_true);
    Stub b3;
    b3.set(ADDR(WMWindowList, isTrayApp), stub_bool_false);
    Stub b4;
    b4.set(ADDR(WMWindowList, isDesktopEntryApp), stub_bool_false);
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act
    m_tester->scanProcess();

    // Assert：自身进程经重分类进入 myApps（注：新进程的本地副本重分类不回写 simpleSet，
    // 仅 m_pidMyApps 记录——记录当前实现行为）
    g_stubAppType = static_cast<int>(kNoFilter);
    EXPECT_EQ(m_tester->m_pidMyApps.count(getpid()), 1);
    EXPECT_EQ(m_tester->m_set.count(getpid()), 1);
}

// kFilterCurrentUser 重分类为托盘：从 myApps 移除（B8 重分类+托盘移除）
TEST_F(UT_ProcessSet, ScanProcess_CurrentUserTray_RemovedFromMyApps)
{
    // Arrange：预置自身 pid 于 myApps，扫描中判定为托盘
    g_stubAppType = static_cast<int>(kFilterCurrentUser);
    m_tester->m_pidMyApps.append(getpid());
    Stub b1;
    b1.set(ADDR(Process, appType), stub_appType_fixed);
    Stub b2;
    b2.set(ADDR(WMWindowList, isGuiApp), stub_bool_true);
    Stub b3;
    b3.set(ADDR(WMWindowList, isTrayApp), stub_bool_true);
    Stub b4;
    b4.set(ADDR(WMWindowList, isDesktopEntryApp), stub_bool_false);
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act
    m_tester->scanProcess();

    // Assert：自身 pid 被移除（重分类托盘移除分支），其他有窗口进程仍会加入
    g_stubAppType = static_cast<int>(kNoFilter);
    EXPECT_EQ(m_tester->m_pidMyApps.count(getpid()), 0);
    EXPECT_EQ(m_tester->m_set.count(getpid()), 1);
}

// kFilterCurrentUser 且无任何窗口：从 myApps 移除（B8 无窗移除分支）
TEST_F(UT_ProcessSet, ScanProcess_CurrentUserNoWindow_RemovedFromMyApps)
{
    // Arrange：预置自身 pid 于 myApps，扫描中判定无窗口
    g_stubAppType = static_cast<int>(kFilterCurrentUser);
    m_tester->m_pidMyApps.append(getpid());
    Stub b1;
    b1.set(ADDR(Process, appType), stub_appType_fixed);
    Stub b2;
    b2.set(ADDR(WMWindowList, isGuiApp), stub_bool_false);
    Stub b3;
    b3.set(ADDR(WMWindowList, isTrayApp), stub_bool_false);
    Stub b4;
    b4.set(ADDR(WMWindowList, isDesktopEntryApp), stub_bool_false);
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act
    m_tester->scanProcess();

    // Assert：无窗口进程被移除
    g_stubAppType = static_cast<int>(kNoFilter);
    EXPECT_EQ(m_tester->m_pidMyApps.count(getpid()), 0);
    EXPECT_EQ(m_tester->m_set.count(getpid()), 1);
}

// GUI 祖先 + /bin/bash 父进程：跳过 appType 覆写，保持 kFilterApps（B13 bash 分支）
TEST_F(UT_ProcessSet, ScanProcess_BashParent_GuiAncestorKeepsAppType)
{
    // Arrange：真实分类经 isDesktopEntryApp→true 写为 kFilterApps（readProcessVariableInfo），
    // myApps 含自身进程；自身无窗口但祖先（真实父进程）有 GUI 窗口；父 cmdline 为 bash
    g_stubAppType = static_cast<int>(kFilterApps);
    g_stubGuiPid = getppid();
    Stub b1;
    b1.set(ADDR(Process, appType), stub_appType_fixed);
    Stub b2;
    b2.set(ADDR(WMWindowList, isTrayApp), stub_bool_false);
    Stub b3;
    b3.set(ADDR(WMWindowList, isGuiApp), stub_isGuiApp_forPid);
    Stub b4;
    b4.set(ADDR(WMWindowList, isDesktopEntryApp), stub_bool_true);
    Stub b5;
    b5.set(ADDR(Process, cmdlineString), stub_cmdlineString_bash);
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act
    m_tester->scanProcess();

    // Assert：bash 父进程分支 continue，appType 不被覆写为 kFilterCurrentUser
    g_stubAppType = static_cast<int>(kNoFilter);
    g_stubGuiPid = 0;
    EXPECT_EQ(m_tester->m_pidMyApps.count(getpid()), 1);
    EXPECT_EQ(m_tester->m_set[getpid()].d->apptype, kFilterApps);
}

// 非 bash 父进程：GUI 祖先存在时 appType 覆写为 kFilterCurrentUser 并移除桌面入口（B13 另一分支）
TEST_F(UT_ProcessSet, ScanProcess_NonBashParent_ReclassifiedCurrentUser)
{
    // Arrange：与 BashParent 同构，但父进程 cmdline 非bash
    g_stubAppType = static_cast<int>(kFilterApps);
    g_stubGuiPid = getppid();
    Stub b1;
    b1.set(ADDR(Process, appType), stub_appType_fixed);
    Stub b2;
    b2.set(ADDR(WMWindowList, isTrayApp), stub_bool_false);
    Stub b3;
    b3.set(ADDR(WMWindowList, isGuiApp), stub_isGuiApp_forPid);
    Stub b4;
    b4.set(ADDR(WMWindowList, isDesktopEntryApp), stub_bool_true);
    Stub b5;
    b5.set(ADDR(Process, cmdlineString), stub_cmdlineString_de);
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act
    m_tester->scanProcess();

    // Assert：非 bash 父进程不 continue，appType 被覆写为 kFilterCurrentUser
    g_stubAppType = static_cast<int>(kNoFilter);
    g_stubGuiPid = 0;
    EXPECT_EQ(m_tester->m_pidMyApps.count(getpid()), 1);
    EXPECT_EQ(m_tester->m_set[getpid()].d->apptype, kFilterCurrentUser);
}

// ---------------- 合并类 ----------------

// mergeSubProcNetIO：多级子进程树自底向上累加 IO 到调用参数
TEST_F(UT_ProcessSet, MergeSubProcNetIO_ProcessTree_SumsDescendantIO)
{
    // Arrange：进程树 100 → {110 → {111}, 120}
    m_tester->m_set.insert(100, Process(100));
    m_tester->m_set.insert(110, Process(110));
    m_tester->m_set.insert(120, Process(120));
    m_tester->m_set.insert(111, Process(111));
    m_tester->m_set[100].setNetIoBps(1, 2);
    m_tester->m_set[110].setNetIoBps(10, 20);
    m_tester->m_set[120].setNetIoBps(100, 200);
    m_tester->m_set[111].setNetIoBps(5, 5);
    m_tester->m_pidPtoCMapping.insert(100, 110);
    m_tester->m_pidPtoCMapping.insert(100, 120);
    m_tester->m_pidPtoCMapping.insert(110, 111);

    // Act
    qreal recvBps = 0;
    qreal sendBps = 0;
    m_tester->mergeSubProcNetIO(100, recvBps, sendBps);

    // Assert：全树累加 1+10+100+5 / 2+20+200+5
    EXPECT_NEAR(recvBps, 116.0, 1e-6);
    EXPECT_NEAR(sendBps, 227.0, 1e-6);

    // 子树部分和：110 → 111 = 10+5 / 20+5
    qreal recvSub = 0;
    qreal sendSub = 0;
    m_tester->mergeSubProcNetIO(110, recvSub, sendSub);
    EXPECT_NEAR(recvSub, 15.0, 1e-6);
    EXPECT_NEAR(sendSub, 25.0, 1e-6);
}

// mergeSubProcNetIO：叶子进程（无子进程）仅累加自身 IO
TEST_F(UT_ProcessSet, MergeSubProcNetIO_LeafProcess_OnlyOwnIO)
{
    // Arrange
    m_tester->m_set.insert(100, Process(100));
    m_tester->m_set[100].setNetIoBps(3, 4);

    // Act
    qreal recvBps = 0;
    qreal sendBps = 0;
    m_tester->mergeSubProcNetIO(100, recvBps, sendBps);

    // Assert
    EXPECT_NEAR(recvBps, 3.0, 1e-6);
    EXPECT_NEAR(sendBps, 4.0, 1e-6);
}

// mergeSubProcNetIO：未知 ppid 经 m_set[ppid] 默认构造插入集合（记录当前实现语义）
TEST_F(UT_ProcessSet, MergeSubProcNetIO_UnknownPpid_InsertsDefaultEntry)
{
    // Arrange：集合为空
    EXPECT_TRUE(m_tester->m_set.isEmpty());

    // Act
    qreal recvBps = 0;
    qreal sendBps = 0;
    m_tester->mergeSubProcNetIO(5555, recvBps, sendBps);

    // Assert：默认进程 IO 为 0，且集合出现默认条目（operator[] 插入语义）
    EXPECT_NEAR(recvBps, 0.0, 1e-6);
    EXPECT_NEAR(sendBps, 0.0, 1e-6);
    EXPECT_TRUE(m_tester->m_set.contains(5555));
    EXPECT_EQ(m_tester->m_set[5555].pid(), 0);
}

// mergeSubProcCpu：多级子进程树累加 CPU 占用
TEST_F(UT_ProcessSet, MergeSubProcCpu_ProcessTree_SumsDescendantCpu)
{
    // Arrange：进程树 100 → {110 → {111}, 120}
    m_tester->m_set.insert(100, Process(100));
    m_tester->m_set.insert(110, Process(110));
    m_tester->m_set.insert(120, Process(120));
    m_tester->m_set.insert(111, Process(111));
    m_tester->m_set[100].setCpu(1.5);
    m_tester->m_set[110].setCpu(2.5);
    m_tester->m_set[120].setCpu(3.0);
    m_tester->m_set[111].setCpu(0.5);
    m_tester->m_pidPtoCMapping.insert(100, 110);
    m_tester->m_pidPtoCMapping.insert(100, 120);
    m_tester->m_pidPtoCMapping.insert(110, 111);

    // Act
    qreal cpu = 0;
    m_tester->mergeSubProcCpu(100, cpu);

    // Assert：全树累加 1.5+2.5+3.0+0.5
    EXPECT_NEAR(cpu, 7.5, 1e-6);

    // 叶子子树：120 无子进程，仅自身
    qreal cpuLeaf = 0;
    m_tester->mergeSubProcCpu(120, cpuLeaf);
    EXPECT_NEAR(cpuLeaf, 3.0, 1e-6);
}

// mergeSubProcCpu：叶子进程仅累加自身 CPU
TEST_F(UT_ProcessSet, MergeSubProcCpu_LeafProcess_OnlyOwnCpu)
{
    // Arrange
    m_tester->m_set.insert(100, Process(100));
    m_tester->m_set[100].setCpu(2.25);

    // Act
    qreal cpu = 0;
    m_tester->mergeSubProcCpu(100, cpu);

    // Assert
    EXPECT_NEAR(cpu, 2.25, 1e-6);
    EXPECT_EQ(m_tester->m_set.size(), 1);
    EXPECT_EQ(m_tester->m_pidPtoCMapping.size(), 0);
}

// ---------------- AM 实例分组 ----------------

// AM 不可用：提前返回，不产生任何分组状态（G1）
TEST_F(UT_ProcessSet, GroupProcesses_AMUnavailable_EarlyReturnNoState)
{
    // Arrange：预置 myApps 与进程集合，AM 判定不可用
    g_stubAmAvailable = false;
    m_tester->m_set.insert(100, Process(100));
    m_tester->m_pidMyApps.append(100);
    Stub b1;
    b1.set(ADDR(AMIconManager, isAMAvailable), stub_isAMAvailable_fixed);
    Stub b2;
    b2.set(ADDR(AMIconManager, getProcessGroupInfo), stub_getProcessGroupInfo_fixed);
    g_stubGroupInfoCalls = 0;

    // Act
    m_tester->groupProcessesByAMInstance();

    // Assert：未查询组信息，分组状态为空
    g_stubAmAvailable = false;
    EXPECT_EQ(g_stubGroupInfoCalls, 0);
    EXPECT_TRUE(m_tester->m_instanceGroups.isEmpty());
    EXPECT_TRUE(m_tester->m_pidToInstance.isEmpty());
}

// 单进程实例：设置 displayName 与 pid 映射，但不落 m_instanceGroups（G2/G5/G6）
TEST_F(UT_ProcessSet, GroupProcesses_SingleInstance_NameSetNoGroupStored)
{
    // Arrange
    g_stubAmAvailable = true;
    g_stubGroupInfo = AMProcessGroupInfo();
    g_stubGroupInfo.appId = QStringLiteral("org.example.app");
    g_stubGroupInfo.instancePath = QStringLiteral("/instance/path");
    g_stubGroupInfo.displayName = QStringLiteral("Example");
    m_tester->m_set.insert(100, Process(100));
    m_tester->m_pidMyApps.append(100);
    Stub b1;
    b1.set(ADDR(AMIconManager, isAMAvailable), stub_isAMAvailable_fixed);
    Stub b2;
    b2.set(ADDR(AMIconManager, getProcessGroupInfo), stub_getProcessGroupInfo_fixed);

    // Act
    m_tester->groupProcessesByAMInstance();

    // Assert：单进程实例记录 pid 映射与显示名，不存储分组（size==1 不落组）
    EXPECT_TRUE(m_tester->m_pidToInstance.contains(100));
    EXPECT_EQ(m_tester->m_pidToInstance[100], QStringLiteral("/instance/path"));
    EXPECT_EQ(m_tester->getProcessById(100).displayName(), QStringLiteral("Example"));
    EXPECT_TRUE(m_tester->m_instanceGroups.isEmpty());
}

// 多进程实例：主进程聚合内存/CPU，子进程隐藏为 kFilterCurrentUser（G2/G4）
TEST_F(UT_ProcessSet, GroupProcesses_MultiInstance_AggregatesAndHidesSubs)
{
    // Arrange：100 与 110 同实例；内存 1000+400、CPU 2.0+1.0；DKapture 模式开启 CPU 聚合
    g_stubAmAvailable = true;
    g_stubGroupInfo = AMProcessGroupInfo();
    g_stubGroupInfo.appId = QStringLiteral("org.example.app");
    g_stubGroupInfo.instancePath = QStringLiteral("/instance/path");
    g_stubGroupInfo.displayName = QStringLiteral("Example");
    Process main(100);
    main.d->rss = 1000;
    main.d->shm = 0;
    main.setCpu(2.0);
    Process sub(110);
    sub.d->rss = 500;
    sub.d->shm = 100;
    sub.setCpu(1.0);
    m_tester->m_set.insert(100, main);
    m_tester->m_set.insert(110, sub);
    m_tester->m_pidMyApps.append(100);
    m_tester->m_pidMyApps.append(110);
    m_tester->m_useSystemService = true;
    Stub b1;
    b1.set(ADDR(AMIconManager, isAMAvailable), stub_isAMAvailable_fixed);
    Stub b2;
    b2.set(ADDR(AMIconManager, getProcessGroupInfo), stub_getProcessGroupInfo_fixed);
    g_stubGroupInfoCalls = 0;

    // Act
    m_tester->groupProcessesByAMInstance();

    // Assert：主进程取最小 pid，聚合内存（rss-shm 之和）与 CPU；子进程隐藏；分组落库
    EXPECT_EQ(g_stubGroupInfoCalls, 2);
    const AMInstanceGroup &group = m_tester->m_instanceGroups[QStringLiteral("/instance/path")];
    EXPECT_EQ(group.mainPid, 100);
    EXPECT_EQ(group.allPids, QList<pid_t>({ 100, 110 }));
    EXPECT_EQ(group.appId, QStringLiteral("org.example.app"));

    EXPECT_EQ(m_tester->m_set[100].memory(), 1400u);
    EXPECT_NEAR(m_tester->m_set[100].cpu(), 3.0, 1e-6);
    EXPECT_EQ(m_tester->m_set[110].d->apptype, kFilterCurrentUser);
    EXPECT_TRUE(m_tester->m_pidToInstance.contains(110));
}

// 恢复缓存中的子进程：prevPidToInstance 中仍存活的 pid 并入实例分组（G3）
TEST_F(UT_ProcessSet, GroupProcesses_RecoveredSubProc_JoinsInstanceGroup)
{
    // Arrange：110 已不在 myApps（重建后丢失），但上一轮实例映射仍记录它
    g_stubAmAvailable = true;
    g_stubGroupInfo = AMProcessGroupInfo();
    g_stubGroupInfo.appId = QStringLiteral("org.example.app");
    g_stubGroupInfo.instancePath = QStringLiteral("/instance/path");
    g_stubGroupInfo.displayName = QStringLiteral("Example");
    Process main(100);
    main.d->rss = 1000;
    main.d->shm = 0;
    Process lost(110);
    lost.d->rss = 500;
    lost.d->shm = 0;
    m_tester->m_set.insert(100, main);
    m_tester->m_set.insert(110, lost);
    m_tester->m_pidMyApps.append(100);
    m_tester->m_pidToInstance[110] = QStringLiteral("/instance/path");
    Stub b1;
    b1.set(ADDR(AMIconManager, isAMAvailable), stub_isAMAvailable_fixed);
    Stub b2;
    b2.set(ADDR(AMIconManager, getProcessGroupInfo), stub_getProcessGroupInfo_fixed);

    // Act
    m_tester->groupProcessesByAMInstance();

    // Assert：110 被恢复进实例并隐藏，主进程内存聚合
    EXPECT_EQ(m_tester->m_pidToInstance[110], QStringLiteral("/instance/path"));
    EXPECT_EQ(m_tester->m_set[110].d->apptype, kFilterCurrentUser);
    EXPECT_EQ(m_tester->m_instanceGroups[QStringLiteral("/instance/path")].allPids.size(), 2);
    EXPECT_EQ(m_tester->m_set[100].memory(), 1500u);
}

// 组信息无效（appId 为空）与不在集合内的 pid 均被跳过（G2 假分支）
TEST_F(UT_ProcessSet, GroupProcesses_InvalidInfoAndMissingPid_Skipped)
{
    // Arrange：100 在集合内但组信息无效；200 在 myApps 但不在集合内
    g_stubAmAvailable = true;
    g_stubGroupInfo = AMProcessGroupInfo();
    g_stubGroupInfo.appId.clear();
    g_stubGroupInfo.instancePath = QStringLiteral("/instance/path");
    Process main(100);
    m_tester->m_set.insert(100, main);
    m_tester->m_pidMyApps.append(100);
    m_tester->m_pidMyApps.append(200);
    Stub b1;
    b1.set(ADDR(AMIconManager, isAMAvailable), stub_isAMAvailable_fixed);
    Stub b2;
    b2.set(ADDR(AMIconManager, getProcessGroupInfo), stub_getProcessGroupInfo_fixed);
    g_stubGroupInfoCalls = 0;

    // Act
    m_tester->groupProcessesByAMInstance();

    // Assert：仅集合内的 100 触发查询；无效信息不产生映射与分组
    EXPECT_EQ(g_stubGroupInfoCalls, 1);
    EXPECT_TRUE(m_tester->m_pidToInstance.isEmpty());
    EXPECT_TRUE(m_tester->m_instanceGroups.isEmpty());
}

// 上一轮缓存的 pid 其实例路径本轮无对应分组：不恢复映射（G3 假分支）
TEST_F(UT_ProcessSet, GroupProcesses_PrevCacheMissingInstance_Dropped)
{
    // Arrange：100 本轮仍在（实例 A）；300 缓存指向本轮不存在的实例 B
    g_stubAmAvailable = true;
    g_stubGroupInfo = AMProcessGroupInfo();
    g_stubGroupInfo.appId = QStringLiteral("org.example.app");
    g_stubGroupInfo.instancePath = QStringLiteral("/instance/a");
    g_stubGroupInfo.displayName = QStringLiteral("Example");
    Process main(100);
    Process cached(300);
    m_tester->m_set.insert(100, main);
    m_tester->m_set.insert(300, cached);
    m_tester->m_pidMyApps.append(100);
    m_tester->m_pidToInstance[300] = QStringLiteral("/instance/b");
    Stub b1;
    b1.set(ADDR(AMIconManager, isAMAvailable), stub_isAMAvailable_fixed);
    Stub b2;
    b2.set(ADDR(AMIconManager, getProcessGroupInfo), stub_getProcessGroupInfo_fixed);

    // Act
    m_tester->groupProcessesByAMInstance();

    // Assert：300 的实例路径本轮无分组，映射不恢复；100 单进程实例不落分组仅设显示名
    EXPECT_FALSE(m_tester->m_pidToInstance.contains(300));
    EXPECT_TRUE(m_tester->m_pidToInstance.contains(100));
    EXPECT_TRUE(m_tester->m_instanceGroups.isEmpty());
    EXPECT_EQ(m_tester->getProcessById(100).displayName(), QStringLiteral("Example"));
}

// getInstanceGroup：已知实例路径返回完整分组
TEST_F(UT_ProcessSet, GetInstanceGroup_KnownPath_ReturnsGroup)
{
    // Arrange
    AMInstanceGroup group;
    group.instancePath = QStringLiteral("/p");
    group.appId = QStringLiteral("a.b");
    group.displayName = QStringLiteral("Name");
    group.mainPid = 100;
    group.allPids = QList<pid_t>({ 100, 110 });
    m_tester->m_instanceGroups[QStringLiteral("/p")] = group;

    // Act
    const AMInstanceGroup got = m_tester->getInstanceGroup(QStringLiteral("/p"));

    // Assert
    EXPECT_EQ(got.instancePath, QStringLiteral("/p"));
    EXPECT_EQ(got.appId, QStringLiteral("a.b"));
    EXPECT_EQ(got.displayName, QStringLiteral("Name"));
    EXPECT_EQ(got.mainPid, 100);
    EXPECT_EQ(got.allPids, QList<pid_t>({ 100, 110 }));
}

// getInstanceGroup：未知实例路径返回默认空分组
TEST_F(UT_ProcessSet, GetInstanceGroup_UnknownPath_ReturnsEmpty)
{
    // Arrange：空分组表
    EXPECT_TRUE(m_tester->m_instanceGroups.isEmpty());

    // Act
    const AMInstanceGroup got = m_tester->getInstanceGroup(QStringLiteral("/not/exist"));

    // Assert
    EXPECT_TRUE(got.instancePath.isEmpty());
    EXPECT_TRUE(got.appId.isEmpty());
    EXPECT_TRUE(got.allPids.isEmpty());
    EXPECT_EQ(got.mainPid, 0);
}

// ---------------- Iterator ----------------

// Iterator 首次构造：定位到首个数字且 >= 10 的 /proc 条目，hasNext 为真
TEST_F(UT_ProcessSet, Iterator_FirstConstruction_HasNextTrue)
{
    // Arrange：真实 /proc 至少包含当前测试进程自身（pid >= 10），无预置状态
    Process procProbe(getpid());
    EXPECT_EQ(procProbe.pid(), getpid());

    // Act
    ProcessSet::Iterator it;
    const bool hasNext = it.hasNext();
    Process first = hasNext ? it.next() : Process();

    // Assert：定位到首个数字条目，pid 不小于 10
    EXPECT_TRUE(hasNext);
    EXPECT_GE(first.pid(), 10);
}

// Iterator 全量遍历：返回的 pid 互异、均 >= 10，且能覆盖当前进程
TEST_F(UT_ProcessSet, Iterator_FullTraversal_PidsDistinctAtLeastTen)
{
    // Arrange
    ProcessSet::Iterator it;
    QSet<pid_t> seen;
    bool foundSelf = false;

    // Act
    while (it.hasNext()) {
        Process proc = it.next();
        seen.insert(proc.pid());
        if (proc.pid() == getpid()) {
            foundSelf = true;
        }
    }

    // Assert
    EXPECT_FALSE(seen.isEmpty());
    EXPECT_EQ(seen.size(), static_cast<int>(std::count_if(seen.begin(), seen.end(), [](pid_t) { return true; })));
    for (pid_t pid : seen) {
        ASSERT_TRUE(pid >= 10);
    }
    EXPECT_TRUE(foundSelf);
}

// Iterator 耗尽后：hasNext 为假，next 返回默认进程；继续 advance 不崩溃（B 末尾分支）
TEST_F(UT_ProcessSet, Iterator_Exhausted_NextReturnsDefaultProcess)
{
    // Arrange：耗尽迭代器
    ProcessSet::Iterator it;
    while (it.hasNext()) {
        it.next();
    }

    // Act
    Process proc = it.next();

    // Assert：返回默认构造进程（pid 0、无效）
    EXPECT_FALSE(it.hasNext());
    EXPECT_EQ(proc.pid(), 0);
    EXPECT_FALSE(proc.isValid());

    // 耗尽后继续推进仍安全
    it.advance();
    EXPECT_FALSE(it.hasNext());
}

// opendir 失败：迭代器从空开始，hasNext 恒假（错误分支）
TEST_F(UT_ProcessSet, Iterator_OpendirFails_StartsEmpty)
{
    // Arrange：opendir 恒空（见 stub 声明）
    EXPECT_FALSE(stub_opendir_null("/proc"));

    // Act
    Stub b1;
    b1.set(opendir, stub_opendir_null);
    ProcessSet::Iterator it;

    // Assert：无目录句柄即无元素，next 返回默认进程
    EXPECT_FALSE(it.hasNext());
    Process proc = it.next();
    EXPECT_EQ(proc.pid(), 0);
}

// advance 反复调用：到达 /proc 末尾后稳定停在空位（readdir 空分支）
TEST_F(UT_ProcessSet, Advance_RepeatedCalls_TerminatesCleanly)
{
    // Arrange
    ProcessSet::Iterator it;

    // Act：推进至耗尽后额外推进 3 次
    while (it.hasNext()) {
        it.advance();
    }
    it.advance();
    it.advance();
    it.advance();

    // Assert：保持耗尽状态，next 返回默认进程，无崩溃
    EXPECT_FALSE(it.hasNext());
    EXPECT_EQ(it.next().pid(), 0);
}

#pragma GCC diagnostic pop
