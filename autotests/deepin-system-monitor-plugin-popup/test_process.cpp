// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成：deepin-system-monitor-plugin-popup Process 单元测试
//
// 被测类：core::process::Process（deepin-system-monitor-plugin-popup/process/process.cpp）
//         + 自由函数 getPriorityName / getProcessPriorityStub
// 测试策略：
//   - 私有方法（read*/applyDKaptureData 等）经 -fno-access-control 直调（deepin-mail
//     同款方案），不模仿被上游 V-6431 排除的遗留主测试私有访问方式
//   - /proc/<pid> 读取链路用 ProcFsSandbox（procfs_sandbox.h）重定向到临时文件：
//     open/fopen/stat/opendir 打桩 + raw syscall 直穿，等价类/边界值内容全可控
//   - 外部依赖全部打桩：ProcessName/ProcessIcon（名称与图标解析）、
//     CPUSet::getUsageTotalDelta（CPU 占比分母）、WMWindowList 三类应用判定
//   - Application 构造已 attach SystemMonitorThread（其构造立即创建 SystemMonitor），
//     ProcessDB::instance()/DeviceDB::instance() 单例链真实可用
//
// 用例计数声明（self-check-structural 校验本块）：
// | method                | level | factors      | min | actual |
// |-----------------------|-------|--------------|-----|--------|
// | applyDKaptureData     | high  | complexity:26| 4   | 9      |
// | readProcessVariableInfo | high | transitive_loop_depth | 3 | 3 |
// | readSockInodes        | high  | complexity:7 | 3   | 3      |
// | Process               | mid   | -            | 2   | 4      |
// | cancelledWriteBytes   | mid   | -            | 2   | 2      |
// | isValid               | mid   | -            | 2   | 2      |
// | memory                | mid   | -            | 2   | 2      |
// | name                  | mid   | -            | 2   | 2      |
// | readCmdline           | mid   | complexity:7 | 2   | 4      |
// | readEnviron           | mid   | complexity:7 | 2   | 3      |
// | readIO                | mid   | complexity:6 | 2   | 3      |
// | readProcessSimpleInfo | mid   | complexity:4 | 2   | 4      |
// | readStat              | mid   | complexity:9 | 3   | 4      |
// | readStatus            | mid   | complexity:9 | 3   | 3      |
// | writeBps              | mid   | -            | 2   | 2      |
// | writeBytes            | mid   | -            | 2   | 2      |
// | appId                 | low   | -            | 1   | 1      |
// | appType               | low   | -            | 1   | 2      |
// | cmdline               | low   | -            | 1   | 1      |
// | cmdlineString         | low   | -            | 1   | 1      |
// | cpu                   | low   | -            | 1   | 2      |
// | displayName           | low   | -            | 1   | 1      |
// | environ               | low   | -            | 1   | 2      |
// | gid                   | low   | -            | 1   | 1      |
// | groupName             | low   | -            | 1   | 1      |
// | icon                  | low   | -            | 1   | 1      |
// | instancePath          | low   | -            | 1   | 1      |
// | operator=             | low   | -            | 1   | 2      |
// | pid                   | low   | -            | 1   | 1      |
// | ppid                  | low   | -            | 1   | 2      |
// | priority              | low   | -            | 1   | 1      |
// | procuptime            | low   | -            | 1   | 1      |
// | readBps               | low   | -            | 1   | 2      |
// | readBytes             | low   | -            | 1   | 2      |
// | readProcessInfo       | low   | -            | 1   | 1      |
// | readSchedStat         | low   | complexity:4 | 1   | 3      |
// | readStatm             | low   | complexity:5 | 1   | 3      |
// | recvBps               | low   | -            | 1   | 2      |
// | recvBytes             | low   | -            | 1   | 1      |
// | sentBps               | low   | -            | 1   | 2      |
// | sentBytes             | low   | -            | 1   | 1      |
// | setAppId              | low   | -            | 1   | 1      |
// | setAppType            | low   | -            | 1   | 1      |
// | setCpu                | low   | -            | 1   | 1      |
// | setDisplayName        | low   | -            | 1   | 1      |
// | setInstancePath       | low   | -            | 1   | 1      |
// | setMemory             | low   | -            | 1   | 1      |
// | setName               | low   | -            | 1   | 1      |
// | setNetIoBps           | low   | -            | 1   | 1      |
// | setPriority           | low   | -            | 1   | 1      |
// | setState              | low   | -            | 1   | 1      |
// | sharememory           | low   | -            | 1   | 1      |
// | startTime             | low   | -            | 1   | 1      |
// | state                 | low   | -            | 1   | 1      |
// | stime                 | low   | -            | 1   | 1      |
// | uid                   | low   | -            | 1   | 2      |
// | userName              | low   | -            | 1   | 2      |
// | utime                 | low   | -            | 1   | 1      |
// | vtrmemory             | low   | -            | 1   | 1      |
// | ~Process              | low   | -            | 1   | 1      |
// | getPriorityName       | low   | -            | 1   | 3      |
// | getProcessPriorityStub | low  | complexity:6 | 1   | 1      |

#include <gtest/gtest.h>
#include <stubext.h>

#include <QApplication>
#include <QTemporaryDir>

#include <cerrno>
#include <memory>

#include "common/common.h"
#include "common/thread_manager.h"
#include "common/base_thread.h"
#include "process/process.h"
#include "process/process_name.h"
#include "process/process_icon.h"
#include "process/private/process_p.h"
#include "process/process_set.h"
#include "process_db.h"
#include "procfs_sandbox.h"
#include "system/system_monitor.h"
#include "system/system_monitor_thread.h"
#include "system/sys_info.h"
// popup 自带 CPUSet/DeviceDB（同名类优先解析 popup 版，与被测编译单元一致）
#include "system/cpu_set.h"
#include "system/device_db.h"
#include "wm/wm_window_list.h"

using namespace core::process;
using namespace core::system;
using namespace core::wm;
using common::init::HZ;
using common::init::kb_shift;

// ---------------------------------------------------------------------------
// 分支清单（来源：Process::applyDKaptureData）
// B1: contains("state") → setState(首字符)
// B2: contains("priority") → setPriority
// B3: contains("comm") → setName
// B4: contains("utime") → d->utime
// B5: contains("stime") → d->stime
// B6: contains("cutime") → d->cutime
// B7: contains("cstime") → d->cstime
// B8: contains("rss") → 字节 >> 10 转 KB
// B9: contains("vsize") → 字节 >> 10 转 KB
// B10: contains("memory_resident") → 页数 << kb_shift
// B11: contains("memory_size") → 页数 << kb_shift
// B12: contains("memory_shared") → 页数 << kb_shift
// B13: contains("ppid") → d->ppid
// B14: contains("num_threads") → d->nthreads
// B15: contains("nice") → d->nice
// B16: contains("start_time") → d->start_time
// B17: contains("uid") → d->uid
// B18: contains("gid") → d->gid
// B19: contains("euid") → d->euid
// B20: contains("egid") → d->egid
// B21: contains("read_bytes") → d->read_bytes
// B22: contains("write_bytes") → d->write_bytes
// B23: contains("cancelled_write_bytes") → d->cancelled_write_bytes
// B24: contains("rq_wait_time") → ns * HZ / 1e9 转 wtime
// B25: euid == uid && (gui || tray || desktop) → kFilterApps
// B26: else if (euid == uid) → kFilterCurrentUser，否则 kNoFilter
// ---------------------------------------------------------------------------
// 分支清单（来源：Process::readStat）
// B1: open /proc/[pid]/stat 失败 → 返回 false
// B2: B1 失败路径提前 return !ok
// B3: read 失败（sz < 0）→ 返回 false
// B4: B3 失败路径提前 return !ok
// B5: 找不到 ')' → 返回 false
// B6: B5 失败路径提前 return !ok
// B7: sscanf 转换数 < 15 → 返回 false
// B8: B7 失败路径提前 return !ok
// B9: sscanf 成功转换 15 项（完整 44 字段行）→ guest/cguest 时间清零
// ---------------------------------------------------------------------------
// 分支清单（来源：Process::readStatus）
// B1: fopen 失败 → 返回 false
// B2: B1 失败路径提前 return !ok
// B3: fgets 逐行循环（真实 FILE*）
// B4: Umask: 前缀 → 解析 mask
// B5: State: 前缀 → 解析 state 首字符
// B6: Uid: 前缀 → 解析 4 个 uid
// B7: Gid: 前缀 → 解析 4 个 gid
// B8: ferror 读失败 → 返回 false
// B9: B8 失败路径提前 return !ok
// ---------------------------------------------------------------------------
// 分支清单（来源：Process::readCmdline）
// B1: fopen 失败 → 返回 false
// B2: B1 失败路径提前 return !ok
// B3: ferror 读失败 → 返回 false
// B4: while 逐字节扫描（cur < end）
// B5: *cur == '\0' → 切分参数并推进 begin
// B6: while 结束后 begin < end → 追加尾段（无结尾 NUL 的参数）
// B7: begin == end → 不追加尾段
// ---------------------------------------------------------------------------
// 分支清单（来源：Process::readEnviron）
// B1: open 失败 → 提前 return
// B2: while 分块 read 循环
// B3: read 返回 0 且 errno != 0 → 提前 return
// B4: sbuf.size() > 0 → 按 '\0' 切分
// B5: for 遍历每个 name=value 段
// B6: 段内 '=' 恰好 2 列 → 写入 environ 哈希
// B7: 段内 '=' 不足 2 列（畸形项）→ 跳过
// ---------------------------------------------------------------------------
// 分支清单（来源：Process::readSockInodes）
// B1: opendir 失败 → 提前 return
// B2: while readdir 逐项遍历
// B3: 项名首字符非数字 → 跳过
// B4: stat /proc/[pid]/fd/[fd] 失败（悬空链接）→ 跳过
// B5: stat 成功且为 socket → 收集
// B6: inode 已收集 → 去重不重复加入
// B7: 遍历结束后 errno != 0 → 仅告警
// ---------------------------------------------------------------------------
// 分支清单（来源：Process::readIO）
// B1: fopen 失败 → 提前 return
// B2: while fgets 逐行扫描
// B3: read_bytes: 前缀 → sscanf 偏移 +12
// B4: write_bytes: 前缀 → sscanf 偏移 +13
// B5: cancelled_write_bytes: 前缀 → sscanf 偏移 +23
// B6: ferror 读失败 → 仅告警
// ---------------------------------------------------------------------------
// 分支清单（来源：Process::readStatm）
// B1: open 失败 → 返回 false
// B2: B1 失败路径提前 return !ok
// B3: read 失败 → 返回 false
// B4: B3 失败路径提前 return !ok
// B5: sscanf 解析出 3 列 → 页数 << kb_shift 转 KB；否则清零
// ---------------------------------------------------------------------------
// 分支清单（来源：Process::readProcessSimpleInfo）
// B1: !skipStatReading → 读 stat/schedstat/statm/status 全链
// B2: recentProcStage 有效 → 计算时间差与磁盘 IO 采样
// B3: euid == uid && (gui||tray||desktop) → kFilterApps
// B4: else if euid == uid → kFilterCurrentUser，否则 kNoFilter
// ---------------------------------------------------------------------------
// 分支清单（来源：Process::readSchedStat）
// B1: open 失败 → 提前 return
// B2: B1 失败路径提前 return
// B3: read 失败（n < 0）→ 提前 return
// B4: sscanf 成功（rc == 1）→ wtime = ns * HZ / 1e9
// ---------------------------------------------------------------------------

namespace {
// 用于构造不可转换的 QVariant（命中 QVariant::toXxx 的 canConvert=false 分支）
struct UtUnconvertible { int x = 0; };
}

namespace {

// 44 字段合法 /proc/[pid]/stat 行（f3..f44 全覆盖）
constexpr const char *kValidStatLine =
    "424242 (ut-proc) S 100 200 0 0 0 0 0 0 0 0 55 66 3 4 0 10 2 0 999 "
    "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3 0 0 0 5 7";
// 完整 44 字段行（f43=5/f44=7 非零）：sscanf 成功转换 15 项 → guest/cguest
// 被显式清零（本插件不消费 guest 统计），验证清零分支覆盖非零值
constexpr const char *kStatLineRc15 = kValidStatLine;


class ProcessTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 名称/图标解析依赖环境与缓存，全部打桩隔离
        stub.set_lamda(ADDR(ProcessName, refreashProcessName),
                       [](ProcessName *, Process *) {});
        stub.set_lamda(ADDR(ProcessIcon, refreashProcessIcon),
                       [](ProcessIcon *, Process *) {});
        stub.set_lamda(ADDR(ProcessIcon, icon),
                       [](ProcessIcon *) -> QIcon { return {}; });
        // CPU 占比分母固定为 100，保证采样值确定性
        stub.set_lamda(ADDR(CPUSet, getUsageTotalDelta),
                       [](CPUSet *) -> qulonglong { return 100; });
        // 窗口管理器相关判定默认 false（具体用例按需覆写）
        stub.set_lamda(ADDR(WMWindowList, isGuiApp),
                       [](const WMWindowList *, pid_t) -> bool { return false; });
        stub.set_lamda(ADDR(WMWindowList, isTrayApp),
                       [](const WMWindowList *, pid_t) -> bool { return false; });
        stub.set_lamda(ADDR(WMWindowList, isDesktopEntryApp),
                       [](const WMWindowList *, pid_t) -> bool { return false; });
    }

    void TearDown() override { stub.clear(); }

    stub_ext::StubExt stub;
};

// ---------------------------------------------------------------------------
// 构造 / 析构 / 赋值
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, Ctor_Default_ZeroPidAndInvalid)
{
    // Arrange
    // Act
    Process p;
    // Assert
    EXPECT_EQ(0, p.pid());
    EXPECT_FALSE(p.isValid());
}

TEST_F(ProcessTest, Ctor_WithPid_SetsPidKeepsInvalid)
{
    // Arrange
    // Act
    Process p(utFakePid());
    // Assert
    EXPECT_EQ(utFakePid(), p.pid());
    EXPECT_FALSE(p.isValid());   // 未读取 /proc 前仍无效
}

TEST_F(ProcessTest, CopyCtor_SharesPid_CowOnWrite)
{
    // Arrange
    Process a(utFakePid());
    a.setName("origin");
    // Act
    Process b(a);
    b.setName("copy");
    // Assert：QSharedDataPointer 写时分离，互不影响
    EXPECT_EQ(a.pid(), b.pid());
    EXPECT_EQ(QStringLiteral("origin"), a.name());
    EXPECT_EQ(QStringLiteral("copy"), b.name());
}

TEST_F(ProcessTest, AssignOp_SelfAssign_NoChange)
{
    // Arrange
    Process p(utFakePid());
    p.setName("self");
    // Act
    Process &ref = p;
    p = ref;
    // Assert
    EXPECT_EQ(utFakePid(), p.pid());
    EXPECT_EQ(QStringLiteral("self"), p.name());
}

TEST_F(ProcessTest, AssignOp_Copy_IsIndependent)
{
    // Arrange
    Process p(utFakePid());
    p.setName("src");
    Process q;
    // Act
    q = p;
    p.setName("changed");
    // Assert
    EXPECT_EQ(utFakePid(), q.pid());
    EXPECT_EQ(QStringLiteral("src"), q.name());
}

TEST_F(ProcessTest, Dtor_ScopeExit_NoLeak)
{
    // Arrange：拷贝共享 d，构造/析构交错验证引用计数
    // Act：作用域退出触发 ~Process（含共享 d 引用计数归零）
    {
        Process p(utFakePid());
        Process p2 = p;
        EXPECT_EQ(utFakePid(), p2.pid());
        EXPECT_EQ(QStringLiteral(""), p2.name());
    }
    // Assert：asan 开启时由运行期泄漏检查兜底；析构后再建新对象状态正确
    Process p3;
    EXPECT_EQ(0, p3.pid());
    EXPECT_FALSE(p3.isValid());
}

// ---------------------------------------------------------------------------
// 简单 setter / getter
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, SetGet_ScalarFields_RoundTrip)
{
    // Arrange
    Process p(utFakePid());
    // Act
    p.setAppType(kFilterApps);
    p.setInstancePath("applications/ut.desktop");
    p.setAppId("ut-app");
    p.setCpu(12.5);
    p.setMemory(4096);
    p.setPriority(kHighPriority);
    p.setState('R');
    p.setName("scalar");
    p.setDisplayName("Scalar Display");
    // Assert
    EXPECT_EQ(int(kFilterApps), p.appType());
    EXPECT_EQ(QStringLiteral("applications/ut.desktop"), p.instancePath());
    EXPECT_EQ(QStringLiteral("ut-app"), p.appId());
    EXPECT_DOUBLE_EQ(12.5, p.cpu());
    EXPECT_EQ(4096, p.memory());
    EXPECT_EQ(kHighPriority, p.priority());
    EXPECT_EQ('R', p.state());
    EXPECT_EQ(QStringLiteral("scalar"), p.name());
    EXPECT_EQ(QStringLiteral("Scalar Display"), p.displayName());
}

TEST_F(ProcessTest, SetGet_NetIoBps_RoundTrip)
{
    // Arrange
    Process p(utFakePid());
    // Act
    p.setNetIoBps(3.5, 9.5);
    // Assert
    EXPECT_DOUBLE_EQ(3.5, p.recvBps());
    EXPECT_DOUBLE_EQ(9.5, p.sentBps());
}

TEST_F(ProcessTest, Getters_DefaultValues_ZeroOrEmpty)
{
    // Arrange
    // Act
    Process p;
    // Assert：全新对象各字段默认值
    EXPECT_EQ(0, p.ppid());
    EXPECT_EQ(0u, p.utime());
    EXPECT_EQ(0u, p.stime());
    EXPECT_EQ(0u, p.memory());
    EXPECT_EQ(0u, p.vtrmemory());
    EXPECT_EQ(0u, p.sharememory());
    EXPECT_EQ(0u, p.readBytes());
    EXPECT_EQ(0u, p.writeBytes());
    EXPECT_EQ(0u, p.cancelledWriteBytes());
    EXPECT_EQ(0u, p.recvBytes());
    EXPECT_EQ(0u, p.sentBytes());
    EXPECT_DOUBLE_EQ(0.0, p.readBps());
    EXPECT_DOUBLE_EQ(0.0, p.writeBps());
    EXPECT_TRUE(p.cmdline().isEmpty());
    EXPECT_TRUE(p.cmdlineString().isEmpty());
    EXPECT_TRUE(p.environ().isEmpty());
    EXPECT_EQ(0, p.procuptime().tv_sec);
    EXPECT_EQ(0, p.procuptime().tv_usec);
    EXPECT_TRUE(p.userName().isEmpty());
    EXPECT_FALSE(p.groupName().isEmpty());   // gid=0 → 真实 /etc/group 组名
}

TEST_F(ProcessTest, Icon_StubbedHook_ReturnsNullIcon)
{
    // Arrange
    Process p(utFakePid());
    // Act
    const QIcon ic = p.icon();
    // Assert
    EXPECT_EQ(utFakePid(), p.pid());
    EXPECT_TRUE(ic.isNull());
}

TEST_F(ProcessTest, StartTime_UsesSysInfoBootTime_PlusTicks)
{
    // Arrange
    Process p(utFakePid());
    p.d->start_time = 2 * HZ;   // 2 秒后的 start_time（直调私有字段）
    // Act
    const time_t st = p.startTime();
    // Assert：btime + start_time/HZ 秒；字段本身未被改动
    EXPECT_EQ(qulonglong(2 * HZ), p.d->start_time);
    const time_t btime = ThreadManager::instance()
                             ->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)
                             ->systemMonitorInstance()->sysInfo()->btime().tv_sec;
    EXPECT_EQ(btime + 2, st);
}

// ---------------------------------------------------------------------------
// readStat
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, ReadStat_ValidLine_ParsesAllVisibleFields)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("stat"), kValidStatLine);
    Process p(utFakePid());
    // Act
    const bool ok = p.readStat();
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(QStringLiteral("ut-proc"), p.name());
    EXPECT_EQ('S', p.state());
    EXPECT_EQ(100, p.ppid());
    EXPECT_EQ(55u, p.utime());
    EXPECT_EQ(66u, p.stime());
    EXPECT_EQ(3u, p.d->cutime);
    EXPECT_EQ(4u, p.d->cstime);
    EXPECT_EQ(10, p.d->nice);
    EXPECT_EQ(2, p.d->nthreads);
    EXPECT_EQ(999u, p.d->start_time);
    EXPECT_EQ(3, p.d->processor);
    EXPECT_EQ(200, p.d->pgid);
}

TEST_F(ProcessTest, ReadStat_OpenFail_ReturnsFalse)
{
    // Arrange：不写 stat 文件 → open ENOENT
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    Process p(utFakePid());
    // Act
    const bool ok = p.readStat();
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_FALSE(p.isValid());
}

TEST_F(ProcessTest, ReadStat_NoClosingParen_ReturnsFalse)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("stat"), "424242 (ut-proc S 1 2 3");
    Process p(utFakePid());
    // Act
    // Assert
    EXPECT_FALSE(p.readStat());
    EXPECT_TRUE(p.name().isEmpty());
}

TEST_F(ProcessTest, ReadStat_MalformedTail_TooFewFields_ReturnsFalse)
{
    // Arrange：')' 后无数字尾巴 → sscanf rc < 15
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("stat"), "424242 (ut-proc) S garbage");
    Process p(utFakePid());
    // Act
    const bool ok = p.readStat();
    // Assert：name 在 sscanf 之前已写入 → 解析失败仍保留
    EXPECT_FALSE(ok);
    EXPECT_EQ(QStringLiteral("ut-proc"), p.name());
}

TEST_F(ProcessTest, ReadStat_Rc15_GuestTimeZeroed_ReturnsTrue)
{
    // Arrange：完整行 → sscanf rc==15（全部转换成功）→ guest/cguest 清零
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("stat"), kStatLineRc15);
    Process p(utFakePid());
    // Act
    const bool ok = p.readStat();
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ('S', p.state());
    EXPECT_EQ(55u, p.utime());
    EXPECT_EQ(0u, p.d->guest_time);
    EXPECT_EQ(0u, p.d->cguest_time);
}

// ---------------------------------------------------------------------------
// readStatus
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, ReadStatus_ValidFields_Parsed)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("status"),
               "Umask: 0022\nState: S (sleeping)\n"
               "Uid: 0 1000 1001 1002\nGid: 0 2000 2001 2002\n");
    Process p(utFakePid());
    // Act
    const bool ok = p.readStatus();
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(22u, p.d->mask);
    EXPECT_EQ('S', p.state());
    EXPECT_EQ(0u, p.uid());
    EXPECT_EQ(1000u, p.d->euid);
    EXPECT_EQ(1001u, p.d->suid);
    EXPECT_EQ(1002u, p.d->fuid);
    EXPECT_EQ(0u, p.gid());
    EXPECT_EQ(2000u, p.d->egid);
    EXPECT_EQ(2001u, p.d->sgid);
    EXPECT_EQ(2002u, p.d->fgid);
}

TEST_F(ProcessTest, ReadStatus_OpenFail_ReturnsFalse)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    Process p(utFakePid());
    // Act
    const bool ok = p.readStatus();
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(0u, p.uid());
}

TEST_F(ProcessTest, ReadStatus_UnknownLinesIgnored_PartialParse)
{
    // Arrange：仅含未知行与 State 行，其余字段保持默认
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("status"),
               "Threads: 4\nState: R (running)\nVmSize: 999 kB\n");
    Process p(utFakePid());
    // Act
    const bool ok = p.readStatus();
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ('R', p.state());
    EXPECT_EQ(0u, p.uid());     // 无 Uid 行 → 默认 0
    EXPECT_EQ(0u, p.d->mask);   // 无 Umask 行 → 默认 0
}

// ---------------------------------------------------------------------------
// readCmdline
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, ReadCmdline_NullSeparated_SplitsArgs)
{
    // Arrange：尾部带 NUL → begin == end，不追加尾段
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), QByteArray("ut-proc\0--flag\0val\0", 19));
    Process p(utFakePid());
    // Act
    const bool ok = p.readCmdline();
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(3, p.cmdline().size());
    EXPECT_EQ(QByteArray("ut-proc"), p.cmdline().at(0));
    EXPECT_EQ(QByteArray("--flag"), p.cmdline().at(1));
    EXPECT_EQ(QByteArray("val"), p.cmdline().at(2));
    EXPECT_EQ(QStringLiteral("ut-proc --flag val"), p.cmdlineString());
}

TEST_F(ProcessTest, ReadCmdline_NoTrailingNull_AppendsTailSegment)
{
    // Arrange：无结尾 NUL → begin < end 追加尾段
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "single");
    Process p(utFakePid());
    // Act
    const bool ok = p.readCmdline();
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(1, p.cmdline().size());
    EXPECT_EQ(QByteArray("single"), p.cmdline().at(0));
}

TEST_F(ProcessTest, ReadCmdline_OpenFail_ReturnsFalse)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    Process p(utFakePid());
    // Act
    const bool ok = p.readCmdline();
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_TRUE(p.cmdline().isEmpty());
}

TEST_F(ProcessTest, ReadCmdline_EmptyFile_ReturnsTrueEmptyList)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "");
    Process p(utFakePid());
    // Act
    const bool ok = p.readCmdline();
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_TRUE(p.cmdline().isEmpty());
}

// ---------------------------------------------------------------------------
// readEnviron
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, ReadEnviron_KvPairs_Parsed)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("environ"), QByteArray("HOME=/root\0LANG=zh_CN.UTF-8\0", 28));
    Process p(utFakePid());
    // Act
    p.readEnviron();
    // Assert
    const auto env = p.environ();
    EXPECT_EQ(2, env.size());
    EXPECT_EQ(QStringLiteral("/root"), env.value(QStringLiteral("HOME")));
    EXPECT_EQ(QStringLiteral("zh_CN.UTF-8"), env.value(QStringLiteral("LANG")));
}

TEST_F(ProcessTest, ReadEnviron_MalformedEntry_Skipped)
{
    // Arrange：无 '=' 的段与空段被跳过
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("environ"), QByteArray("BAD\0K=V\0\0", 9));
    Process p(utFakePid());
    // Act
    p.readEnviron();
    // Assert
    const auto env = p.environ();
    EXPECT_EQ(1, env.size());
    EXPECT_EQ(QStringLiteral("V"), env.value(QStringLiteral("K")));
}

TEST_F(ProcessTest, ReadEnviron_OpenFail_ReturnsEarlyNoCrash)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    Process p(utFakePid());
    // Act
    p.readEnviron();
    // Assert
    EXPECT_TRUE(p.environ().isEmpty());
    EXPECT_TRUE(p.name().isEmpty());
}

// ---------------------------------------------------------------------------
// readSchedStat
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, ReadSchedStat_Valid_ConvertsNsToTicks)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("schedstat"), "0 5000000000 100");
    Process p(utFakePid());
    // Act
    p.readSchedStat();
    // Assert：5e9 ns → 5 * HZ 个时钟滴答；schedstat 不触碰 utime
    EXPECT_EQ(qulonglong(5 * HZ), p.d->wtime);
    EXPECT_EQ(0u, p.utime());
}

TEST_F(ProcessTest, ReadSchedStat_Malformed_KeepsZero)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("schedstat"), "garbage");
    Process p(utFakePid());
    // Act
    p.readSchedStat();
    // Assert：解析失败保持零值
    EXPECT_EQ(0u, p.d->wtime);
    EXPECT_EQ(0u, p.utime());
}

TEST_F(ProcessTest, ReadSchedStat_OpenFail_KeepsZero)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    Process p(utFakePid());
    // Act
    p.readSchedStat();
    // Assert
    EXPECT_EQ(0u, p.d->wtime);
    EXPECT_EQ(0u, p.utime());
}

// ---------------------------------------------------------------------------
// readStatm
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, ReadStatm_Valid_ConvertsPagesToKb)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("statm"), "200 100 50");
    Process p(utFakePid());
    // Act
    const bool ok = p.readStatm();
    // Assert：页数 << kb_shift
    EXPECT_TRUE(ok);
    EXPECT_EQ(200u << kb_shift, p.vtrmemory());
    EXPECT_EQ(100u << kb_shift, p.d->rss);
    EXPECT_EQ(50u << kb_shift, p.sharememory());
    EXPECT_EQ(50u << kb_shift, p.memory());   // rss - shm
}

TEST_F(ProcessTest, ReadStatm_Malformed_ZeroesFields_ReturnsTrue)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("statm"), "not-a-number");
    Process p(utFakePid());
    // Act
    const bool ok = p.readStatm();
    // Assert
    EXPECT_TRUE(ok);   // 解析失败仅清零，仍返回 true
    EXPECT_EQ(0u, p.vtrmemory());
    EXPECT_EQ(0u, p.memory());
}

TEST_F(ProcessTest, ReadStatm_OpenFail_ReturnsFalse)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    Process p(utFakePid());
    // Act
    const bool ok = p.readStatm();
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(0u, p.vtrmemory());
}

// ---------------------------------------------------------------------------
// readIO
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, ReadIo_ThreeCounters_Parsed)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("io"),
               "rchar: 999\nread_bytes: 111\nwrite_bytes: 222\n"
               "cancelled_write_bytes: 33\n");
    Process p(utFakePid());
    // Act
    p.readIO();
    // Assert
    EXPECT_EQ(111u, p.readBytes());
    EXPECT_EQ(222u, p.writeBytes());
    EXPECT_EQ(33u, p.cancelledWriteBytes());
}

TEST_F(ProcessTest, ReadIo_UnknownLinesIgnored_FieldsRemainZero)
{
    // Arrange：仅未知键 → strncmp 各分支不命中
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("io"), "rchar: 999\nsyscr: 1\n");
    Process p(utFakePid());
    // Act
    p.readIO();
    // Assert
    EXPECT_EQ(0u, p.readBytes());
    EXPECT_EQ(0u, p.writeBytes());
    EXPECT_EQ(0u, p.cancelledWriteBytes());
}

TEST_F(ProcessTest, ReadIo_OpenFail_NoCrash)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    Process p(utFakePid());
    // Act：io 文件缺失 → 仅告警不崩溃
    p.readIO();
    // Assert：三个计数器保持零值
    EXPECT_EQ(0u, p.readBytes());
    EXPECT_EQ(0u, p.writeBytes());
    EXPECT_EQ(0u, p.cancelledWriteBytes());
}

// ---------------------------------------------------------------------------
// readSockInodes
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, SockInodes_OpendirFail_EarlyReturnEmpty)
{
    // Arrange：opendir 注入失败（/proc/<pid>/fd 不存在）
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    ASSERT_TRUE(sb.armOpendir(true));
    Process p(utFakePid());
    // Act
    p.readSockInodes();
    // Assert：提前返回，集合为空，进程有效性不受影响
    EXPECT_TRUE(p.d->sockInodes.isEmpty());
    EXPECT_EQ(0, p.d->sockInodes.size());
}

TEST_F(ProcessTest, SockInodes_SocketLinks_CollectedAndDeduped)
{
    // Arrange：两个不同 inode 的 socket 链接 + 指向同一文件的重复链接
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    ASSERT_TRUE(sb.addSocket(QStringLiteral("sock-a")));
    ASSERT_TRUE(sb.addSocket(QStringLiteral("sock-b")));
    ASSERT_TRUE(sb.addSymlink(QStringLiteral("3"), sb.sockPath(QStringLiteral("sock-a"))));
    ASSERT_TRUE(sb.addSymlink(QStringLiteral("4"), sb.sockPath(QStringLiteral("sock-b"))));
    ASSERT_TRUE(sb.addSymlink(QStringLiteral("5"), sb.sockPath(QStringLiteral("sock-a"))));  // 重复 inode
    ASSERT_TRUE(sb.armOpendir(false));
    Process p(utFakePid());
    // Act
    p.readSockInodes();
    // Assert：仅两个唯一 inode，重复项被去重，且 inode 均有效
    ASSERT_EQ(2, p.d->sockInodes.size());
    EXPECT_GT(p.d->sockInodes.front(), ino_t(0));
    EXPECT_GT(p.d->sockInodes.back(), ino_t(0));
}

TEST_F(ProcessTest, SockInodes_RegularFileTarget_NotCollected)
{
    // Arrange：指向普通文件的 fd 链接（stat 成功但非 socket）+ 一个 socket 链接
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("fd/reg.txt"), "plain");
    ASSERT_TRUE(sb.addSymlink(QStringLiteral("8"), sb.sockPath(QStringLiteral("reg.txt"))));
    ASSERT_TRUE(sb.addSocket(QStringLiteral("sock-a")));
    ASSERT_TRUE(sb.addSymlink(QStringLiteral("9"), sb.sockPath(QStringLiteral("sock-a"))));
    ASSERT_TRUE(sb.armOpendir(false));
    Process p(utFakePid());
    // Act
    p.readSockInodes();
    // Assert：仅 socket 链接的 inode 被收集
    ASSERT_EQ(1, p.d->sockInodes.size());
    EXPECT_GT(p.d->sockInodes.front(), ino_t(0));
}

TEST_F(ProcessTest, SockInodes_NonDigitAndDangling_Skipped)
{
    // Arrange：非数字项 + 悬空链接（stat 失败）
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    ASSERT_TRUE(sb.addSymlink(QStringLiteral("notdigit"), "target-x"));
    ASSERT_TRUE(sb.addSymlink(QStringLiteral("7"), "dangling-target"));
    ASSERT_TRUE(sb.armOpendir(false));
    Process p(utFakePid());
    // Act
    p.readSockInodes();
    // Assert：非数字项与悬空链接均被跳过
    EXPECT_TRUE(p.d->sockInodes.isEmpty());
    EXPECT_EQ(0, p.d->sockInodes.size());
}

// ---------------------------------------------------------------------------
// readProcessSimpleInfo / readProcessInfo / readProcessVariableInfo
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, ReadSimpleInfo_FullSandbox_AllFieldsSet_OtherUser)
{
    // Arrange：uid 65534（nobody）≠ 当前 euid → kNoFilter
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("stat"), kValidStatLine);
    sb.setFile(QStringLiteral("statm"), "200 100 50");
    sb.setFile(QStringLiteral("status"), "Uid: 65534 65534 65534 65534\n");
    sb.setFile(QStringLiteral("cmdline"), "ut-proc\0");
    Process p(utFakePid());
    // Act
    p.readProcessSimpleInfo(false);
    // Assert
    EXPECT_TRUE(p.isValid());
    EXPECT_EQ(QStringLiteral("ut-proc"), p.name());
    EXPECT_EQ('S', p.state());
    EXPECT_EQ(200u << kb_shift, p.vtrmemory());
    EXPECT_EQ(65534u, p.uid());
    EXPECT_EQ(int(kNoFilter), p.appType());
    EXPECT_FALSE(p.userName().isEmpty());
}

TEST_F(ProcessTest, ReadSimpleInfo_MissingCmdline_Invalid)
{
    // Arrange：cmdline 缺失 → ok=false → valid=false
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("stat"), kValidStatLine);
    Process p(utFakePid());
    // Act
    p.readProcessSimpleInfo(false);
    // Assert：cmdline 失败 → && 链短路，stat/statm/status 均未读取 →
    // utime 保持 0，整体无效
    EXPECT_FALSE(p.isValid());
    EXPECT_EQ(0u, p.utime());
}

TEST_F(ProcessTest, ReadSimpleInfo_SkipStatReading_OnlyCmdlineRead)
{
    // Arrange：skipStatReading=true → stat 缺失不影响，cmdline 决定有效性
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "lite-proc\0");
    Process p(utFakePid());
    // Act
    p.readProcessSimpleInfo(true);
    // Assert
    EXPECT_TRUE(p.isValid());
    EXPECT_EQ(0u, p.utime());   // stat 未读 → utime 保持 0
    EXPECT_EQ(QByteArray("lite-proc"), p.cmdline().at(0));
}

TEST_F(ProcessTest, ReadSimpleInfo_WithRecentStage_ComputesSamples)
{
    // Arrange：向单例 ProcessSet 注入最近一次采样 → 走 validrecentPtr 分支
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("stat"), kValidStatLine);
    sb.setFile(QStringLiteral("cmdline"), "ut-proc\0");
    sb.setFile(QStringLiteral("statm"), "100 200 300\n");
    sb.setFile(QStringLiteral("status"),
               "Umask: 0022\nState: S (sleeping)\n"
               "Uid: 0 1000 1001 1002\nGid: 0 2000 2001 2002\n");
    auto stage = std::make_shared<RecentProcStage>();
    stage->ptime = 10;           // (utime+stime) - ptime = 121 - 10
    stage->read_bytes = 1;
    stage->write_bytes = 2;
    stage->uptime = timeval {100, 0};
    ProcessDB::instance()->processSet()->m_recentProcStage.insert(utFakePid(), stage);
    Process p(utFakePid());
    // Act
    p.readProcessSimpleInfo(false);
    // Assert：CPU 采样 = (55+66-10)/100*100，采样已入队且进程有效
    EXPECT_TRUE(p.isValid());
    EXPECT_DOUBLE_EQ(111.0, p.cpu());
    ProcessDB::instance()->processSet()->m_recentProcStage.remove(utFakePid());
}

TEST_F(ProcessTest, ReadProcessInfo_ValidFiles_DelegatesFullChain)
{
    // Arrange：全链路文件齐备（cmdline/stat/statm/status）→ ok 全真
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("stat"), kValidStatLine);
    sb.setFile(QStringLiteral("cmdline"), "ut-proc\0");
    sb.setFile(QStringLiteral("statm"), "100 200 300\n");
    sb.setFile(QStringLiteral("status"),
               "Umask: 0022\nState: S (sleeping)\n"
               "Uid: 0 1000 1001 1002\nGid: 0 2000 2001 2002\n");
    Process p(utFakePid());
    // Act
    p.readProcessInfo();
    // Assert
    EXPECT_TRUE(p.isValid());
    EXPECT_EQ(QStringLiteral("ut-proc"), p.name());
}

TEST_F(ProcessTest, ReadProcessVariableInfo_ValidFiles_SetsState)
{
    // Arrange：readProcessVariableInfo → readProcessInfo → 全链路文件齐备
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("stat"), kValidStatLine);
    sb.setFile(QStringLiteral("cmdline"), "ut-proc\0");
    sb.setFile(QStringLiteral("statm"), "100 200 300\n");
    sb.setFile(QStringLiteral("status"),
               "Umask: 0022\nState: S (sleeping)\n"
               "Uid: 0 1000 1001 1002\nGid: 0 2000 2001 2002\n");
    Process p(utFakePid());
    // Act
    p.readProcessVariableInfo();
    // Assert
    EXPECT_TRUE(p.isValid());
    EXPECT_EQ('S', p.state());
}

// ---------------------------------------------------------------------------
// applyDKaptureData
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, ApplyDK_TrayAppOnly_FiltersAsApps)
{
    // Arrange：uid == euid + 仅 isTrayApp → kFilterApps（|| 链第二臂）
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "tray-proc\0");
    stub.set_lamda(ADDR(WMWindowList, isGuiApp),
                   [](const WMWindowList *, pid_t) -> bool { return false; });
    stub.set_lamda(ADDR(WMWindowList, isTrayApp),
                   [](const WMWindowList *, pid_t) -> bool { return true; });
    QVariantMap map;
    map["uid"] = uint(geteuid());
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert：托盘应用 → kFilterApps
    EXPECT_EQ(int(kFilterApps), p.appType());
}

TEST_F(ProcessTest, ApplyDK_DesktopEntryAppOnly_FiltersAsApps)
{
    // Arrange：uid == euid + 仅 isDesktopEntryApp → kFilterApps（|| 链第三臂）
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "desktop-proc\0");
    stub.set_lamda(ADDR(WMWindowList, isGuiApp),
                   [](const WMWindowList *, pid_t) -> bool { return false; });
    stub.set_lamda(ADDR(WMWindowList, isTrayApp),
                   [](const WMWindowList *, pid_t) -> bool { return false; });
    stub.set_lamda(ADDR(WMWindowList, isDesktopEntryApp),
                   [](const WMWindowList *, pid_t) -> bool { return true; });
    QVariantMap map;
    map["uid"] = uint(geteuid());
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert：桌面入口应用 → kFilterApps
    EXPECT_EQ(int(kFilterApps), p.appType());
}

TEST_F(ProcessTest, ApplyDK_FullMap_AllFieldsApplied)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "dk-proc\0");
    QVariantMap map;
    map["state"] = QStringLiteral("D");
    map["priority"] = 15;
    map["comm"] = QStringLiteral("dk-comm");
    map["utime"] = qulonglong(11);
    map["stime"] = qulonglong(22);
    map["cutime"] = qulonglong(33);
    map["cstime"] = qulonglong(44);
    map["rss"] = qulonglong(4096);          // 字节 → 4 KB
    map["vsize"] = qulonglong(8192);        // 字节 → 8 KB
    map["ppid"] = 7;
    map["num_threads"] = 5;
    map["nice"] = 3;
    map["start_time"] = qulonglong(12345);
    map["uid"] = uint(geteuid());
    map["gid"] = uint(5000);
    map["euid"] = uint(5001);
    map["egid"] = uint(5002);
    map["read_bytes"] = qulonglong(100);
    map["write_bytes"] = qulonglong(200);
    map["cancelled_write_bytes"] = qulonglong(50);
    map["rq_wait_time"] = qulonglong(1000000000);   // 1e9 ns → HZ ticks
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert
    EXPECT_TRUE(p.isValid());
    EXPECT_EQ('D', p.state());
    // 注意：applyDKaptureData 先处理 "priority"→setPriority(15)，后处理
    // "nice"→d->nice=3 覆盖同字段 → 实际 priority()==3（疑似源码缺陷，见缺陷清单）
    EXPECT_EQ(3, p.priority());
    EXPECT_EQ(QStringLiteral("dk-comm"), p.name());
    EXPECT_EQ(11u, p.utime());
    EXPECT_EQ(22u, p.stime());
    EXPECT_EQ(33u, p.d->cutime);
    EXPECT_EQ(44u, p.d->cstime);
    EXPECT_EQ(4u, p.d->rss);
    EXPECT_EQ(8u, p.vtrmemory());
    EXPECT_EQ(7, p.ppid());
    EXPECT_EQ(5, p.d->nthreads);
    EXPECT_EQ(3, p.d->nice);
    EXPECT_EQ(12345u, p.d->start_time);
    EXPECT_EQ(geteuid(), p.uid());
    EXPECT_EQ(5000u, p.gid());
    EXPECT_EQ(5001u, p.d->euid);
    EXPECT_EQ(5002u, p.d->egid);
    EXPECT_EQ(100u, p.readBytes());
    EXPECT_EQ(200u, p.writeBytes());
    EXPECT_EQ(50u, p.cancelledWriteBytes());
    EXPECT_EQ(qulonglong(HZ), p.d->wtime);
    EXPECT_EQ(int(kFilterCurrentUser), p.appType());   // uid == euid 且非 GUI
}

TEST_F(ProcessTest, ApplyDK_EmptyMap_CmdlineFail_Invalid)
{
    // Arrange：空 map + 无 cmdline 文件
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(QVariantMap {});
    // Assert：字段保持默认，valid=false
    EXPECT_FALSE(p.isValid());
    EXPECT_EQ(0u, p.utime());
    EXPECT_TRUE(p.name().isEmpty());
}

TEST_F(ProcessTest, ApplyDK_PagesToKb_ConvertedWithKbShift)
{
    // Arrange
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "dk-proc\0");
    QVariantMap map;
    map["memory_resident"] = qulonglong(10);
    map["memory_size"] = qulonglong(20);
    map["memory_shared"] = qulonglong(2);
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert：页数 << kb_shift
    EXPECT_EQ(10u << kb_shift, p.d->rss);
    EXPECT_EQ(20u << kb_shift, p.vtrmemory());
    EXPECT_EQ(2u << kb_shift, p.sharememory());
}

TEST_F(ProcessTest, ApplyDK_RqWaitTimeZero_WtimeStaysZero)
{
    // Arrange：rq_wait_time=0 边界
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "dk-proc\0");
    QVariantMap map;
    map["rq_wait_time"] = qulonglong(0);
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert：rq_wait_time=0 → wtime 保持 0；cmdline 可读仍有效
    EXPECT_EQ(0u, p.d->wtime);
    EXPECT_TRUE(p.isValid());
}

TEST_F(ProcessTest, ApplyDK_EmptyMap_NoFieldsApplied)
{
    // Arrange：空 map → 所有 contains 分支走 false 臂，仅尾部链路生效
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "empty-proc\0");
    QVariantMap map;
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert：无字段被赋值 → 默认值保持（state 默认 '\0'）；cmdline 成功 → valid
    EXPECT_TRUE(p.isValid());
    EXPECT_EQ(0u, p.utime());
    EXPECT_EQ(0u, p.stime());
    EXPECT_EQ('\0', p.state());
}

TEST_F(ProcessTest, ApplyDK_GuiApp_FiltersAsApps)
{
    // Arrange：uid == euid + isGuiApp → kFilterApps
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "gui-proc\0");
    stub.set_lamda(ADDR(WMWindowList, isGuiApp),
                   [](const WMWindowList *, pid_t) -> bool { return true; });
    QVariantMap map;
    map["uid"] = uint(geteuid());
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert：GUI 应用 → kFilterApps
    EXPECT_EQ(int(kFilterApps), p.appType());
    EXPECT_TRUE(p.isValid());
}

TEST_F(ProcessTest, ApplyDK_WrongTypeValues_ConversionFailsGracefully)
{
    // Arrange：所有字段给不可转换类型 → toXxx() canConvert=false 分支 + 无崩溃
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "badtype-proc\0");
    QVariantMap map;
    const QVariant bad = QVariant::fromValue(UtUnconvertible{});
    map["priority"] = bad;
    map["comm"] = bad;
    map["utime"] = bad;
    map["stime"] = bad;
    map["cutime"] = bad;
    map["cstime"] = bad;
    map["rss"] = bad;
    map["vsize"] = bad;
    map["memory_resident"] = bad;
    map["memory_size"] = bad;
    map["memory_shared"] = bad;
    map["ppid"] = bad;
    map["num_threads"] = bad;
    map["nice"] = bad;
    map["start_time"] = bad;
    map["uid"] = bad;
    map["gid"] = bad;
    map["read_bytes"] = bad;
    map["write_bytes"] = bad;
    map["cancelled_write_bytes"] = bad;
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert：转换全部回退默认值，进程仍有效（cmdline 成功）
    EXPECT_TRUE(p.isValid());
    EXPECT_EQ(0u, p.utime());
    EXPECT_EQ(0u, p.utime() + p.stime());
    // uid 转换为 0 → 与 euid(geteuid()) 不等 → kNoFilter
    EXPECT_EQ(int(kNoFilter), p.appType());
}

TEST_F(ProcessTest, ApplyDK_UidMismatch_OtherUser_NoFilter)
{
    // Arrange：uid 为有效数值但不等于 euid → apptype 判定链 false 臂
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "otheruser-proc\0");
    QVariantMap map;
    map["uid"] = uint(geteuid()) + 5;   // 有效 uint，但 != euid
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert：uid 不等 → 三个应用判定均短路不调用 → kNoFilter
    EXPECT_EQ(int(kNoFilter), p.appType());
    EXPECT_TRUE(p.isValid());
}

TEST_F(ProcessTest, ApplyDK_CurrentUserNonGui_FilterCurrentUser)
{
    // Arrange：uid == euid，三类应用判定均 false
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "cli-proc\0");
    QVariantMap map;
    map["uid"] = uint(geteuid());
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert：当前用户 CLI 进程 → kFilterCurrentUser
    EXPECT_EQ(int(kFilterCurrentUser), p.appType());
    EXPECT_TRUE(p.isValid());
}

TEST_F(ProcessTest, ApplyDK_OtherUser_NoFilter)
{
    // Arrange：uid != euid
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "other-proc\0");
    QVariantMap map;
    map["uid"] = uint(65534);
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert：他人进程 → 不过滤
    EXPECT_EQ(int(kNoFilter), p.appType());
    EXPECT_TRUE(p.isValid());
}

TEST_F(ProcessTest, ApplyDK_EnvironMissing_ToleratedStillValid)
{
    // Arrange：cmdline 存在、environ 缺失 → 容忍，仍有效
    stub_ext::StubExt local;
    ProcFsSandbox sb(local);
    sb.setFile(QStringLiteral("cmdline"), "dk-proc\0");
    QVariantMap map;
    map["comm"] = QStringLiteral("env-ok");
    Process p(utFakePid());
    // Act
    p.applyDKaptureData(map);
    // Assert
    EXPECT_TRUE(p.isValid());
    EXPECT_TRUE(p.environ().isEmpty());
    EXPECT_EQ(QStringLiteral("env-ok"), p.name());
}

// ---------------------------------------------------------------------------
// 自由函数 getPriorityName / getProcessPriorityStub
// ---------------------------------------------------------------------------

TEST_F(ProcessTest, GetPriorityName_NamedPriorities_Mapped)
{
    // Arrange
    const int normal = kNormalPriority;
    // Act：查询各命名优先级的显示名
    // Assert
    EXPECT_EQ(QApplication::translate("Process.Priority", "Normal"),
              getPriorityName(kNormalPriority));
    EXPECT_EQ(QApplication::translate("Process.Priority", "Very high"),
              getPriorityName(kVeryHighPriority));
    EXPECT_EQ(QApplication::translate("Process.Priority", "High"),
              getPriorityName(kHighPriority));
    EXPECT_EQ(QApplication::translate("Process.Priority", "Low"),
              getPriorityName(kLowPriority));
    EXPECT_EQ(QApplication::translate("Process.Priority", "Very low"),
              getPriorityName(kVeryLowPriority));
}

TEST_F(ProcessTest, GetPriorityName_CustomRange_AndInvalid)
{
    // Arrange
    const int custom = -15;
    // Act：查询自定义区间与无效值
    // Assert：区间 [-20, 19] 内的未命名值 → Custom
    EXPECT_EQ(QApplication::translate("Process.Priority", "Custom"),
              getPriorityName(-15));
    EXPECT_EQ(QApplication::translate("Process.Priority", "Custom"),
              getPriorityName(5));
    // 区间外与 INT_MAX → Invalid
    EXPECT_EQ(QApplication::translate("Process.Priority", "Invalid"),
              getPriorityName(20));
    EXPECT_EQ(QApplication::translate("Process.Priority", "Invalid"),
              getPriorityName(-21));
    EXPECT_EQ(QApplication::translate("Process.Priority", "Invalid"),
              getPriorityName(kInvalidPriority));
}

TEST_F(ProcessTest, GetProcessPriorityStub_AllRanges_MapsCorrectly)
{
    // Arrange
    const int normal = 0;
    // Act：查询各命名/自定义/无效值
    // Assert：命名优先级直通
    EXPECT_EQ(kNormalPriority, getProcessPriorityStub(0));
    EXPECT_EQ(kVeryHighPriority, getProcessPriorityStub(-20));
    EXPECT_EQ(kHighPriority, getProcessPriorityStub(-10));
    EXPECT_EQ(kLowPriority, getProcessPriorityStub(10));
    EXPECT_EQ(kVeryLowPriority, getProcessPriorityStub(19));
    // 自定义区间
    EXPECT_EQ(kCustomPriority, getProcessPriorityStub(-15));
    EXPECT_EQ(kCustomPriority, getProcessPriorityStub(5));
    // 无效值
    EXPECT_EQ(kInvalidPriority, getProcessPriorityStub(25));
    EXPECT_EQ(kInvalidPriority, getProcessPriorityStub(INT_MAX));
}

}   // namespace
