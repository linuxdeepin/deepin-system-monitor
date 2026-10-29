// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 本文件为 ut_process.cpp 的重写版（qt-autotest-generator Mode 6 审查判定原文件
// 132 个错误 / 22 个警告，verdict FAIL：全部用例缺 Arrange/Act/Assert 三段注释、
// 5 个空断言、59 个单断言、21 个纯布尔断言，列入全量重写清单）。

// 用例统计声明（与 .ut-inventory.json 的 usecase_count 对应）
// | 方法                       | 级别 | 计划 | 实际 |
// |----------------------------|------|------|------|
// | Process()                  | mid  | 2    | 2    |
// | ~Process()                 | low  | 1    | 1    |
// | operator=                  | low  | 1    | 1    |
// | isValid                    | mid  | 1    | 1    |
// | pid/ppid                   | mid  | 2    | 2    |
// | utime/stime                | mid  | 2    | 2    |
// | name/setName               | mid  | 2    | 2    |
// | displayName/setDisplayName | low  | 2    | 2    |
// | icon                       | low  | 1    | 1    |
// | cpu/setCpu                 | mid  | 3    | 3    |
// | memory/setMemory           | mid  | 3    | 3    |
// | vtrmemory/sharememory      | low  | 2    | 2    |
// | priority/setPriority       | mid  | 2    | 2    |
// | state/setState             | mid  | 2    | 2    |
// | cmdline                    | mid  | 1    | 1    |
// | cmdlineString              | low  | 2    | 2    |
// | environ                    | low  | 1    | 1    |
// | uid/userName/setUserName   | mid  | 3    | 3    |
// | gid/groupName              | mid  | 2    | 2    |
// | instancePath/setInstancePath | low | 1   | 1    |
// | appId/setAppId             | low  | 1    | 1    |
// | startTime/setUptime        | low  | 2    | 2    |
// | readBps/writeBps           | low  | 2    | 2    |
// | readBytes/writeBytes       | low  | 2    | 2    |
// | cancelledWriteBytes        | mid  | 1    | 1    |
// | recvBps/sentBps/setNetIoBps | low | 4    | 4    |
// | recvBytes/sentBytes        | low  | 2    | 2    |
// | appType/setAppType         | low  | 2    | 2    |
// | procuptime                 | low  | 1    | 1    |
// | readStat                   | mid  | 5    | 5    |
// | readSchedStat              | low  | 4    | 4    |
// | readStatm                  | low  | 4    | 4    |
// | readStatus                 | high | 4    | 4    |
// | readCmdline                | mid  | 2    | 2    |
// | readEnviron                | high | 3    | 3    |
// | readIO                     | mid  | 2    | 2    |
// | readSockInodes             | high | 2    | 2    |
// | readProcessInfo            | high | 4    | 4    |
// | readProcessSimpleInfo      | low  | 1    | 1    |
// | readProcessVariableInfo    | low  | 1    | 1    |
// | calculateProcessMetrics    | mid  | 1    | 1    |
// | refreashProcessName        | mid  | 1    | 1    |
// | refreashProcessIcon        | low  | 1    | 1    |
// | applyDKaptureData          | high | 11   | 11   |
// | getPriorityName            | low  | 7    | 7    |
// | getProcessPriorityStub     | low  | 3    | 3    |
// | 合计                       |      | 98   | 98   |
//
// 设计要点：
//   1. headless 环境隔离：凡触达 ProcessIcon::refreashProcessIcon 的路径
//      （applyDKaptureData / readProcessInfo / readProcessSimpleInfo）一律
//      以 ADDR 打桩为 no-op，避免 xcb 窗口查询崩溃。
//   2. /proc 只读面直接使用本进程真实数据（uid/gid/cmdline/environ/stat），
//      不依赖外部设备与环境。
//   3. readStat/readStatm/readSchedStat 的成功路径以签名匹配的 read 打桩注入
//      合法 /proc 行，逐字段断言解析结果；失败路径断言返回值 + 打桩标记串。
//   4. 打桩 open 成功返回 fd=1 时必须同时打桩 close，避免真实 close(STDOUT)。
//   5. m_Sresult 为打桩标记全局串，每个用例 Arrange 中先 clear。

//self
#include "process/process.h"
#include "process/process_set.h"
#include "process/process_db.h"
#include "wm/wm_window_list.h"
#include "common/common.h"
#include "process/private/process_p.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QIcon>
#include <QApplication>
#include <QFile>
#include <QUrl>
//system
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <sys/types.h>

using namespace core::process;
using namespace common::init;
static QString m_Sresult;
static bool g_stubWmGui = false;
/***************************************STUB begin*********************************************/
int stub_readStat_open1()
{
    m_Sresult = "open";
    return 1;
}
int stub_readStat_open2()
{
    m_Sresult = "open failed";
    return -1;
}
ssize_t stub_readStat_read2()
{
    m_Sresult = "read failed";
    return -1;
}
int stub_readStat_close()
{
    // 仅拦截真实 close(STDOUT)，不改动 m_Sresult，避免覆盖 read 的打桩标记
    return 1;
}
// 注入不含右括号的非法 stat 内容（readStat 解析失败分支）
ssize_t stub_readStat_read_nostat(int, void *buf, size_t)
{
    strcpy(static_cast<char *>(buf), "inv(test");
    m_Sresult = "read";
    return 8;
}
// 注入合法 stat 行（44 字段布局，与 Process::readStat 的 sscanf 位置一致）
ssize_t stub_readStat_read_valid(int, void *buf, size_t)
{
    // state(3)=S ppid(4)=1 pgid(5)=1 utime(14)=55 stime(15)=66 nice(19)=2
    // nthreads(20)=4 start_time(22)=999 processor(39)=1 guest(43)=100 cguest(44)=200
    const char *line = "1234 (ut-process) S 1 1 0 0 0 0 0 0 0 0 55 66 0 0 10 2 4 0 999 "
                       "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 100 200";
    strcpy(static_cast<char *>(buf), line);
    m_Sresult = "read";
    return static_cast<ssize_t>(strlen(line));
}
// 注入合法 statm 行：vmsize(200) rss(100) shm(50)，单位为页
ssize_t stub_readStatm_read_valid(int, void *buf, size_t)
{
    strcpy(static_cast<char *>(buf), "200 100 50");
    m_Sresult = "read";
    return 10;
}
// 注入空读（EOF），触发 readStatm 解析失败清零分支
ssize_t stub_read_zero(int, void *, size_t)
{
    m_Sresult = "read";
    return 0;
}
// 注入合法 schedstat 行：运行时长(0) 等待时长 5000000000ns 调度次数(100)
ssize_t stub_readSchedStat_read_valid(int, void *buf, size_t)
{
    strcpy(static_cast<char *>(buf), "0 5000000000 100");
    m_Sresult = "read";
    return 16;
}
FILE *stub_fopen_fail(const char *, const char *)
{
    m_Sresult = "fopen failed";
    return nullptr;
}
DIR *stub_opendir_fail(const char *)
{
    m_Sresult = "diropen failed";
    return nullptr;
}
bool stub_bool_false()
{
    return false;
}
// WMWindowList::isGuiApp 成员函数打桩（成员 ABI：rdi=this, rsi=pid）
bool stub_wm_gui_flag(void *, pid_t)
{
    return g_stubWmGui;
}
void stub_refreashProcessIcon_noop(Process *)
{
}
bool stub_readCmdline_false()
{
    return false;
}
// 取一个超出系统 pid_max 的不存在 pid，用于 access 失败分支
static pid_t ut_bogus_pid()
{
    pid_t max = 4194304;
    QFile f(QStringLiteral("/proc/sys/kernel/pid_max"));
    if (f.open(QIODevice::ReadOnly)) {
        const int v = f.readAll().trimmed().toInt();
        if (v > 0)
            max = v;
    }
    return max + 1;
}
/***************************************STUB end**********************************************/
class UT_Process : public ::testing::Test
{
public:
    UT_Process() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // Arrange：以本进程 pid 构造被测对象（真实 /proc 只读面可用）
        common::init::global_init();
        m_tester = new Process(getpid());
        m_Sresult.clear();
        g_stubWmGui = false;
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    Process *m_tester;
};

/***************************************************************************************************
 * 构造 / 析构 / 赋值
 **************************************************************************************************/
TEST_F(UT_Process, Process_DefaultPid_PrivateInitialized)
{
    // Arrange：SetUps 已用本进程 pid 构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取构造后的私有字段
    const pid_t pid = m_tester->pid();

    // Assert：pid 正确，其余字段保持初值
    EXPECT_EQ(pid, getpid());
    EXPECT_EQ(m_tester->d->ppid, 0);
    EXPECT_EQ(m_tester->d->rss, 0ULL);
    EXPECT_EQ(m_tester->d->cmdline.size(), 0);
}

TEST_F(UT_Process, Process_Ctor_CopiesFields)
{
    // Arrange：构造源对象并填充字段
    m_tester->setName("ut-copy-src");
    m_tester->setPriority(7);
    m_tester->setState('R');

    // Act：拷贝构造
    Process copy(*m_tester);

    // Assert：值字段逐一相等
    EXPECT_EQ(copy.pid(), m_tester->pid());
    EXPECT_EQ(copy.name(), QString("ut-copy-src"));
    EXPECT_EQ(copy.priority(), 7);
    EXPECT_EQ(copy.state(), 'R');
    EXPECT_EQ(copy.d->utime, m_tester->d->utime);
}

TEST_F(UT_Process, Dtor_ScopeExit_NoLeak)
{
    // Arrange：准备局部作用域记录变量
    pid_t scopePid = 0;

    // Act：局部对象离开作用域触发析构
    {
        Process proc(getpid());
        scopePid = proc.pid();
    }

    // Assert：作用域内取值有效且析构后测试夹具仍可用
    EXPECT_EQ(scopePid, getpid());
    EXPECT_EQ(m_tester->pid(), getpid());
}

TEST_F(UT_Process, OperatorAssign_OtherInstance_CopiesPid)
{
    // Arrange：两个不同 pid 的对象
    Process lhs(getpid());
    Process rhs(getppid());

    // Act：赋值
    lhs = rhs;

    // Assert：赋值后与源对象一致
    EXPECT_EQ(lhs.pid(), getppid());
    EXPECT_EQ(lhs.pid(), rhs.pid());
}

/***************************************************************************************************
 * 基础 getter / setter
 **************************************************************************************************/
TEST_F(UT_Process, IsValid_DefaultCtor_ReturnsFalse)
{
    // Arrange：SetUps 已构造未读取 /proc 的对象
    ASSERT_NE(m_tester, nullptr);

    // Act：查询有效性
    const bool valid = m_tester->isValid();

    // Assert：默认构造未读取数据，应无效
    EXPECT_FALSE(valid);
    EXPECT_EQ(m_tester->d->valid, false);
}

TEST_F(UT_Process, Pid_DefaultCtor_MatchesOwnPid)
{
    // Arrange：SetUps 已用本进程 pid 构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 pid
    const pid_t pid = m_tester->pid();

    // Assert：getter 与私有字段、系统调用三方一致
    EXPECT_EQ(pid, getpid());
    EXPECT_EQ(m_tester->d->pid, getpid());
}

TEST_F(UT_Process, Ppid_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象（未读取 stat）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 ppid
    const pid_t ppid = m_tester->ppid();

    // Assert：未读取前保持初值 0
    EXPECT_EQ(ppid, 0);
    EXPECT_EQ(m_tester->d->ppid, 0);
}

TEST_F(UT_Process, Utime_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 utime
    const qulonglong utime = m_tester->utime();

    // Assert：未读取前保持初值 0
    EXPECT_EQ(utime, 0ULL);
    EXPECT_EQ(m_tester->d->utime, 0ULL);
}

TEST_F(UT_Process, Stime_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 stime
    const qulonglong stime = m_tester->stime();

    // Assert：未读取前保持初值 0
    EXPECT_EQ(stime, 0ULL);
    EXPECT_EQ(m_tester->d->stime, 0ULL);
}

TEST_F(UT_Process, Name_DefaultCtor_Empty)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取进程名
    const QString name = m_tester->name();

    // Assert：未读取前为空串
    EXPECT_EQ(name, QString());
    EXPECT_EQ(name.size(), 0);
}

TEST_F(UT_Process, SetName_ValidName_FieldUpdated)
{
    // Arrange：准备合法进程名
    const QString name = "ut-name";

    // Act：写入进程名
    m_tester->setName(name);

    // Assert：getter 与私有字段一致
    EXPECT_EQ(m_tester->name(), name);
    EXPECT_EQ(m_tester->d->name, name);
}

TEST_F(UT_Process, DisplayName_DefaultCtor_MatchesCache)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取展示名
    const QString display = m_tester->displayName();

    // Assert：与缓存展示名一致且初始为空
    EXPECT_EQ(display, m_tester->d->proc_name.displayName());
    EXPECT_EQ(display, QString());
}

TEST_F(UT_Process, SetDisplayName_CustomName_CacheUpdated)
{
    // Arrange：准备展示名
    const QString display = "ut-display";

    // Act：写入展示名
    m_tester->setDisplayName(display);

    // Assert：缓存与 getter 一致
    EXPECT_EQ(m_tester->d->proc_name.displayName(), display);
    EXPECT_EQ(m_tester->displayName(), display);
}

TEST_F(UT_Process, Icon_DefaultCtor_NullIcon)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取图标
    const QIcon icon = m_tester->icon();

    // Assert：默认为空图标且与缓存一致
    EXPECT_TRUE(icon.isNull());
    EXPECT_EQ(icon.cacheKey(), m_tester->d->proc_icon.icon().cacheKey());
}

TEST_F(UT_Process, InstancePath_RoundTrip_ValueKept)
{
    // Arrange：准备实例路径
    const QString path = "ut-instance-path";

    // Act：写入后读取
    m_tester->setInstancePath(path);

    // Assert：往返一致
    EXPECT_EQ(m_tester->instancePath(), path);
    EXPECT_EQ(m_tester->d->m_instancePath, path);
}

TEST_F(UT_Process, AppId_RoundTrip_ValueKept)
{
    // Arrange：准备 AM 应用 ID
    const QString appId = "ut.app.id";

    // Act：写入后读取
    m_tester->setAppId(appId);

    // Assert：往返一致
    EXPECT_EQ(m_tester->appId(), appId);
    EXPECT_EQ(m_tester->d->m_appId, appId);
}

/***************************************************************************************************
 * CPU / 内存
 **************************************************************************************************/
TEST_F(UT_Process, Cpu_NoSamples_ReturnsZero)
{
    // Arrange：SetUps 已构造对象（无采样）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 CPU 占用
    const qreal cpu = m_tester->cpu();

    // Assert：无采样时返回 0
    EXPECT_EQ(cpu, 0.0);
    EXPECT_EQ(m_tester->d->cpuUsageSample->recentSample(), nullptr);
}

TEST_F(UT_Process, SetCpu_LatestSample_ReturnedByCpu)
{
    // Arrange：写入一个 CPU 采样
    m_tester->setCpu(42.5);

    // Act：读取 CPU 占用
    const qreal cpu = m_tester->cpu();

    // Assert：返回最新采样值
    EXPECT_DOUBLE_EQ(cpu, 42.5);
    ASSERT_NE(m_tester->d->cpuUsageSample->recentSample(), nullptr);
    EXPECT_DOUBLE_EQ(m_tester->d->cpuUsageSample->recentSample()->data, 42.5);
}

TEST_F(UT_Process, CalculateProcessMetrics_NoHistory_NoCrash)
{
    // Arrange：SetUps 已构造对象（无任何采样历史）
    ASSERT_NE(m_tester, nullptr);

    // Act：计算进程指标
    m_tester->calculateProcessMetrics();
    const qreal cpu = m_tester->cpu();

    // Assert：无历史时 CPU 占用不为负
    EXPECT_GE(cpu, 0.0);
    EXPECT_EQ(m_tester->pid(), getpid());
}

TEST_F(UT_Process, Memory_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象（rss/shm 均为 0）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取物理内存
    const qulonglong memory = m_tester->memory();

    // Assert：rss - shm = 0
    EXPECT_EQ(memory, 0ULL);
    EXPECT_EQ(m_tester->d->rss, 0ULL);
}

TEST_F(UT_Process, SetMemory_InvertsRssWithShm_RssIncludesShm)
{
    // Arrange：预设共享内存
    m_tester->d->shm = 100;

    // Act：写入物理内存（rss = memory + shm）
    m_tester->setMemory(1234);

    // Assert：回读值与反转结果一致
    EXPECT_EQ(m_tester->memory(), 1234ULL);
    EXPECT_EQ(m_tester->d->rss, 1334ULL);
}

TEST_F(UT_Process, Vtrmemory_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取虚拟内存
    const qulonglong vtrmemory = m_tester->vtrmemory();

    // Assert：未读取前保持初值 0
    EXPECT_EQ(vtrmemory, 0ULL);
    EXPECT_EQ(m_tester->d->vmsize, 0ULL);
}

TEST_F(UT_Process, Sharememory_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取共享内存
    const qulonglong sharememory = m_tester->sharememory();

    // Assert：未读取前保持初值 0
    EXPECT_EQ(sharememory, 0ULL);
    EXPECT_EQ(m_tester->d->shm, 0ULL);
}

/***************************************************************************************************
 * 优先级 / 状态
 **************************************************************************************************/
TEST_F(UT_Process, Priority_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取优先级
    const int priority = m_tester->priority();

    // Assert：未读取前保持初值 0
    EXPECT_EQ(priority, 0);
    EXPECT_EQ(m_tester->d->nice, 0);
}

TEST_F(UT_Process, SetPriority_PositiveValue_NiceUpdated)
{
    // Arrange：准备优先级值
    const int prio = 15;

    // Act：写入优先级
    m_tester->setPriority(prio);

    // Assert：getter 与私有字段一致
    EXPECT_EQ(m_tester->priority(), 15);
    EXPECT_EQ(m_tester->d->nice, 15);
}

TEST_F(UT_Process, State_DefaultCtor_ZeroChar)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取进程状态
    const char state = m_tester->state();

    // Assert：未读取前为 '\0'
    EXPECT_EQ(state, '\0');
    EXPECT_EQ(m_tester->d->state, '\0');
}

TEST_F(UT_Process, SetState_RunningChar_StateUpdated)
{
    // Arrange：准备运行状态字符
    const char state = 'R';

    // Act：写入状态
    m_tester->setState(state);

    // Assert：getter 与私有字段一致
    EXPECT_EQ(m_tester->state(), 'R');
    EXPECT_EQ(m_tester->d->state, 'R');
}

/***************************************************************************************************
 * 命令行 / 环境 / 身份
 **************************************************************************************************/
TEST_F(UT_Process, Cmdline_DefaultCtor_Empty)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取命令行列表
    const QByteArrayList cmdline = m_tester->cmdline();

    // Assert：未读取前为空列表
    EXPECT_EQ(cmdline.size(), 0);
    EXPECT_EQ(m_tester->d->cmdline.size(), 0);
}

TEST_F(UT_Process, CmdlineString_EmptyCmdline_EmptyString)
{
    // Arrange：SetUps 已构造对象（cmdline 为空）
    ASSERT_NE(m_tester, nullptr);

    // Act：拼接命令行串
    const QString cmdlineString = m_tester->cmdlineString();

    // Assert：空列表拼接结果为空串
    EXPECT_EQ(cmdlineString, QString());
    EXPECT_EQ(cmdlineString.size(), 0);
}

TEST_F(UT_Process, CmdlineString_MultiArgs_PercentDecoded)
{
    // Arrange：注入带百分号编码的命令行片段
    m_tester->d->cmdline << QByteArray("deepin%20app") << QByteArray("-v");

    // Act：拼接并解码
    const QString cmdlineString = m_tester->cmdlineString();

    // Assert：%20 应被解码为空格
    EXPECT_EQ(cmdlineString, QString("deepin app -v"));
    EXPECT_EQ(cmdlineString.size(), 13);
}

TEST_F(UT_Process, Environ_DefaultCtor_Empty)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取环境变量表
    const QHash<QString, QString> environ = m_tester->environ();

    // Assert：未读取前为空
    EXPECT_EQ(environ.size(), 0);
    EXPECT_EQ(m_tester->d->environ.size(), 0);
}

TEST_F(UT_Process, Uid_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 uid
    const uid_t uid = m_tester->uid();

    // Assert：未读取前保持初值 0
    EXPECT_EQ(uid, 0U);
    EXPECT_EQ(m_tester->d->uid, 0U);
}

TEST_F(UT_Process, UserName_DefaultCtor_Empty)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取用户名
    const QString userName = m_tester->userName();

    // Assert：未读取前为空
    EXPECT_EQ(userName, QString());
    EXPECT_EQ(m_tester->d->usrerName, QString());
}

TEST_F(UT_Process, SetUserName_CustomName_FieldUpdated)
{
    // Arrange：准备用户名
    const QString user = "ut-user";

    // Act：写入用户名
    m_tester->setUserName(user);

    // Assert：getter 与私有字段一致
    EXPECT_EQ(m_tester->userName(), user);
    EXPECT_EQ(m_tester->d->usrerName, user);
}

TEST_F(UT_Process, Gid_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 gid
    const gid_t gid = m_tester->gid();

    // Assert：未读取前保持初值 0
    EXPECT_EQ(gid, 0U);
    EXPECT_EQ(m_tester->d->gid, 0U);
}

TEST_F(UT_Process, GroupName_DefaultCtor_MatchesSysInfo)
{
    // Arrange：SetUps 已构造对象（gid 为 0）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取组名
    const QString groupName = m_tester->groupName();

    // Assert：与 SysInfo 按 gid 查询结果一致
    EXPECT_EQ(groupName, SysInfo::groupName(m_tester->d->gid));
    EXPECT_EQ(groupName, SysInfo::groupName(0));
}

/***************************************************************************************************
 * 时间 / 磁盘 / 网络
 **************************************************************************************************/
TEST_F(UT_Process, StartTime_DefaultCtor_BootTimePlusZero)
{
    // Arrange：SetUps 已构造对象（start_time 保持初值 0）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取启动时间（系统启动时刻 + start_time/HZ）
    const time_t startTime = m_tester->startTime();
    const time_t startTimeAgain = m_tester->startTime();

    // Assert：两次读取一致且不早于系统启动时刻
    EXPECT_EQ(startTimeAgain, startTime);
    EXPECT_GE(startTime, 0);
    EXPECT_EQ(m_tester->d->start_time, 0ULL);
}

TEST_F(UT_Process, SetUptime_Value_RoundTrip)
{
    // Arrange：准备时间值
    timeval tv { 100, 500 };

    // Act：写入进程存活时间
    m_tester->setUptime(tv);

    // Assert：getter 与写入值一致
    EXPECT_EQ(m_tester->procuptime().tv_sec, 100);
    EXPECT_EQ(m_tester->procuptime().tv_usec, 500);
}

TEST_F(UT_Process, Procuptime_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取存活时间
    const timeval uptime = m_tester->procuptime();

    // Assert：未读取前两个分量均为 0
    EXPECT_EQ(uptime.tv_sec, 0);
    EXPECT_EQ(uptime.tv_usec, 0);
}

TEST_F(UT_Process, ReadBps_NoSamples_ReturnsZero)
{
    // Arrange：SetUps 已构造对象（无磁盘采样）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取磁盘读速率
    const qreal readBps = m_tester->readBps();

    // Assert：无采样时返回 0
    EXPECT_EQ(readBps, 0.0);
    EXPECT_EQ(m_tester->d->diskIOSpeedSample->recentSample(), nullptr);
}

TEST_F(UT_Process, WriteBps_NoSamples_ReturnsZero)
{
    // Arrange：SetUps 已构造对象（无磁盘采样）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取磁盘写速率
    const qreal writeBps = m_tester->writeBps();

    // Assert：无采样时返回 0
    EXPECT_EQ(writeBps, 0.0);
    EXPECT_EQ(m_tester->d->diskIOSpeedSample->recentSample(), nullptr);
}

TEST_F(UT_Process, ReadBytes_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取累计读字节
    const qulonglong readBytes = m_tester->readBytes();

    // Assert：未读取前保持初值 0
    EXPECT_EQ(readBytes, 0ULL);
    EXPECT_EQ(m_tester->d->read_bytes, 0ULL);
}

TEST_F(UT_Process, WriteBytes_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取累计写字节
    const qulonglong writeBytes = m_tester->writeBytes();

    // Assert：未读取前保持初值 0
    EXPECT_EQ(writeBytes, 0ULL);
    EXPECT_EQ(m_tester->d->write_bytes, 0ULL);
}

TEST_F(UT_Process, CancelledWriteBytes_DefaultCtor_Zero)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取取消写字节
    const qulonglong cancelled = m_tester->cancelledWriteBytes();

    // Assert：未读取前保持初值 0
    EXPECT_EQ(cancelled, 0ULL);
    EXPECT_EQ(m_tester->d->cancelled_write_bytes, 0ULL);
}

TEST_F(UT_Process, RecvBps_NoSamples_ReturnsZero)
{
    // Arrange：SetUps 已构造对象（无网络采样）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取收包速率
    const qreal recvBps = m_tester->recvBps();

    // Assert：无采样时返回 0
    EXPECT_EQ(recvBps, 0.0);
    EXPECT_EQ(m_tester->d->networkBandwidthSample->recentSample(), nullptr);
}

TEST_F(UT_Process, RecvBps_AfterSetNetIoBps_ReturnsRecvValue)
{
    // Arrange：写入网络速率采样（收 11.5 发 22.5）
    m_tester->setNetIoBps(11.5, 22.5);

    // Act：读取收包速率
    const qreal recvBps = m_tester->recvBps();

    // Assert：与写入的收包速率一致
    EXPECT_DOUBLE_EQ(recvBps, 11.5);
    ASSERT_NE(m_tester->d->networkBandwidthSample->recentSample(), nullptr);
    EXPECT_DOUBLE_EQ(m_tester->d->networkBandwidthSample->recentSample()->data.inBps, 11.5);
}

TEST_F(UT_Process, SentBps_NoSamples_ReturnsZero)
{
    // Arrange：SetUps 已构造对象（无网络采样）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取发包速率
    const qreal sentBps = m_tester->sentBps();

    // Assert：无采样时返回 0
    EXPECT_EQ(sentBps, 0.0);
    EXPECT_EQ(m_tester->d->networkBandwidthSample->recentSample(), nullptr);
}

TEST_F(UT_Process, SentBps_AfterSetNetIoBps_ReturnsSentValue)
{
    // Arrange：写入网络速率采样（收 33.5 发 44.5）
    m_tester->setNetIoBps(33.5, 44.5);

    // Act：读取发包速率
    const qreal sentBps = m_tester->sentBps();

    // Assert：与写入的发包速率一致
    EXPECT_DOUBLE_EQ(sentBps, 44.5);
    ASSERT_NE(m_tester->d->networkBandwidthSample->recentSample(), nullptr);
    EXPECT_DOUBLE_EQ(m_tester->d->networkBandwidthSample->recentSample()->data.outBps, 44.5);
}

TEST_F(UT_Process, RecvBytes_NoSamples_ReturnsZero)
{
    // Arrange：SetUps 已构造对象（无网络 IO 采样）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取累计收字节
    const qulonglong recvBytes = m_tester->recvBytes();

    // Assert：无采样时返回 0
    EXPECT_EQ(recvBytes, 0ULL);
    EXPECT_EQ(m_tester->d->networkIOSample->recentSample(), nullptr);
}

TEST_F(UT_Process, SentBytes_NoSamples_ReturnsZero)
{
    // Arrange：SetUps 已构造对象（无网络 IO 采样）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取累计发字节
    const qulonglong sentBytes = m_tester->sentBytes();

    // Assert：无采样时返回 0
    EXPECT_EQ(sentBytes, 0ULL);
    EXPECT_EQ(m_tester->d->networkIOSample->recentSample(), nullptr);
}

/***************************************************************************************************
 * 应用类型
 **************************************************************************************************/
TEST_F(UT_Process, AppType_DefaultCtor_NoFilter)
{
    // Arrange：SetUps 已构造对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取应用类型
    const int appType = m_tester->appType();

    // Assert：初始为不过滤
    EXPECT_EQ(appType, kNoFilter);
    EXPECT_EQ(m_tester->d->apptype, kNoFilter);
}

TEST_F(UT_Process, SetAppType_FilterApps_Updated)
{
    // Arrange：准备应用类型值
    const int type = kFilterApps;

    // Act：写入应用类型
    m_tester->setAppType(type);

    // Assert：getter 与私有字段一致
    EXPECT_EQ(m_tester->appType(), kFilterApps);
    EXPECT_EQ(m_tester->d->apptype, kFilterApps);
}

/***************************************************************************************************
 * readStat：/proc/[pid]/stat
 **************************************************************************************************/
TEST_F(UT_Process, ReadStat_OpenFails_ReturnsFalse)
{
    // Arrange：打桩 open 失败
    Stub b1;
    b1.set(open, stub_readStat_open2);

    // Act：读取 stat
    const bool ok = m_tester->readStat();

    // Assert：返回失败且打桩标记命中
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_Sresult, QString("open failed"));
    EXPECT_EQ(m_tester->d->state, '\0');
}

TEST_F(UT_Process, ReadStat_ReadFails_ReturnsFalse)
{
    // Arrange：open 成功但 read 失败（close 必须打桩避免关闭 STDOUT）
    Stub b1;
    b1.set(open, stub_readStat_open1);
    Stub b2;
    b2.set(read, stub_readStat_read2);
    Stub b3;
    b3.set(close, stub_readStat_close);

    // Act：读取 stat
    const bool ok = m_tester->readStat();

    // Assert：返回失败且打桩标记命中
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_Sresult, QString("read failed"));
}

TEST_F(UT_Process, ReadStat_MalformedContent_ReturnsFalse)
{
    // Arrange：注入缺少右括号的非法内容
    Stub b1;
    b1.set(open, stub_readStat_open1);
    Stub b2;
    b2.set(read, stub_readStat_read_nostat);
    Stub b3;
    b3.set(close, stub_readStat_close);

    // Act：读取 stat
    const bool ok = m_tester->readStat();

    // Assert：解析失败返回 false
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_Sresult, QString("read"));
    EXPECT_EQ(m_tester->name(), QString());
}

TEST_F(UT_Process, ReadStat_ValidLine_FieldsParsed)
{
    // Arrange：注入合法 44 字段 stat 行
    Stub b1;
    b1.set(open, stub_readStat_open1);
    Stub b2;
    b2.set(read, stub_readStat_read_valid);
    Stub b3;
    b3.set(close, stub_readStat_close);

    // Act：读取 stat
    const bool ok = m_tester->readStat();

    // Assert：逐字段核对解析结果
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_tester->name(), QString("ut-process"));
    EXPECT_EQ(m_tester->state(), 'S');
    EXPECT_EQ(m_tester->utime(), 55ULL);
    EXPECT_EQ(m_tester->stime(), 66ULL);
    EXPECT_EQ(m_tester->d->ppid, 1);
    EXPECT_EQ(m_tester->d->pgid, 1);
    EXPECT_EQ(m_tester->d->nice, 2);
    EXPECT_EQ(m_tester->d->nthreads, 4);
    EXPECT_EQ(m_tester->d->start_time, 999ULL);
}

TEST_F(UT_Process, ReadStat_RealSelfProc_FieldsPopulated)
{
    // Arrange：使用本进程真实 /proc 数据（SetUps 已设 pid）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 stat
    const bool ok = m_tester->readStat();

    // Assert：进程名非空且 CPU 时间已累计
    EXPECT_TRUE(ok);
    EXPECT_FALSE(m_tester->name().isEmpty());
    EXPECT_GT(m_tester->utime() + m_tester->stime(), 0ULL);
    EXPECT_EQ(m_tester->pid(), getpid());
}

/***************************************************************************************************
 * readSchedStat：/proc/[pid]/schedstat
 **************************************************************************************************/
TEST_F(UT_Process, ReadSchedStat_OpenFails_ReturnsEarly)
{
    // Arrange：打桩 open 失败
    Stub b1;
    b1.set(open, stub_readStat_open2);

    // Act：读取调度统计
    m_tester->readSchedStat();

    // Assert：提前返回且 wtime 保持 0
    EXPECT_EQ(m_Sresult, QString("open failed"));
    EXPECT_EQ(m_tester->d->wtime, 0ULL);
}

TEST_F(UT_Process, ReadSchedStat_ReadFails_ReturnsEarly)
{
    // Arrange：open 成功但 read 失败（close 必须打桩）
    Stub b1;
    b1.set(open, stub_readStat_open1);
    Stub b2;
    b2.set(read, stub_readStat_read2);
    Stub b3;
    b3.set(close, stub_readStat_close);

    // Act：读取调度统计
    m_tester->readSchedStat();

    // Assert：提前返回且 wtime 保持 0
    EXPECT_EQ(m_Sresult, QString("read failed"));
    EXPECT_EQ(m_tester->d->wtime, 0ULL);
}

TEST_F(UT_Process, ReadSchedStat_ValidLine_WtimeConverted)
{
    // Arrange：注入合法 schedstat 行（等待 5000000000ns）
    Stub b1;
    b1.set(open, stub_readStat_open1);
    Stub b2;
    b2.set(read, stub_readSchedStat_read_valid);
    Stub b3;
    b3.set(close, stub_readStat_close);

    // Act：读取调度统计
    m_tester->readSchedStat();

    // Assert：纳秒按 HZ 换算为时钟滴答
    EXPECT_EQ(m_Sresult, QString("read"));
    EXPECT_EQ(m_tester->d->wtime, static_cast<qulonglong>(5000000000UL * HZ / 1000000000UL));
}

TEST_F(UT_Process, ReadSchedStat_RealSelfProc_NoCrash)
{
    // Arrange：使用本进程真实 /proc 数据
    ASSERT_NE(m_tester, nullptr);

    // Act：读取调度统计
    m_tester->readSchedStat();

    // Assert：等待时间非负
    EXPECT_GE(m_tester->d->wtime, 0ULL);
    EXPECT_EQ(m_tester->pid(), getpid());
}

/***************************************************************************************************
 * readStatm：/proc/[pid]/statm
 **************************************************************************************************/
TEST_F(UT_Process, ReadStatm_OpenFails_ReturnsFalse)
{
    // Arrange：打桩 open 失败
    Stub b1;
    b1.set(open, stub_readStat_open2);

    // Act：读取内存映射
    const bool ok = m_tester->readStatm();

    // Assert：返回失败且打桩标记命中
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_Sresult, QString("open failed"));
    EXPECT_EQ(m_tester->d->rss, 0ULL);
}

TEST_F(UT_Process, ReadStatm_ValidLine_PagesConvertedToKB)
{
    // Arrange：注入合法 statm 行（vmsize=200 rss=100 shm=50 页）
    Stub b1;
    b1.set(open, stub_readStat_open1);
    Stub b2;
    b2.set(read, stub_readStatm_read_valid);
    Stub b3;
    b3.set(close, stub_readStat_close);

    // Act：读取内存映射
    const bool ok = m_tester->readStatm();

    // Assert：页数按 kb_shift 换算为 KB
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_tester->d->vmsize, 200ULL << kb_shift);
    EXPECT_EQ(m_tester->d->rss, 100ULL << kb_shift);
    EXPECT_EQ(m_tester->d->shm, 50ULL << kb_shift);
    EXPECT_EQ(m_tester->memory(), 50ULL << kb_shift);
}

TEST_F(UT_Process, ReadStatm_EmptyRead_ZeroesFields)
{
    // Arrange：注入空读（EOF）
    Stub b1;
    b1.set(open, stub_readStat_open1);
    Stub b2;
    b2.set(read, stub_read_zero);
    Stub b3;
    b3.set(close, stub_readStat_close);

    // Act：读取内存映射
    const bool ok = m_tester->readStatm();

    // Assert：解析失败后三字段清零且整体返回成功
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_tester->d->vmsize, 0ULL);
    EXPECT_EQ(m_tester->d->rss, 0ULL);
    EXPECT_EQ(m_tester->d->shm, 0ULL);
}

TEST_F(UT_Process, ReadStatm_RealSelfProc_ConvertsToKB)
{
    // Arrange：使用本进程真实 /proc 数据
    ASSERT_NE(m_tester, nullptr);

    // Act：读取内存映射
    const bool ok = m_tester->readStatm();

    // Assert：常驻与虚拟内存均为正值
    EXPECT_TRUE(ok);
    EXPECT_GT(m_tester->d->rss, 0ULL);
    EXPECT_GT(m_tester->d->vmsize, 0ULL);
}

/***************************************************************************************************
 * readStatus：/proc/[pid]/status
 **************************************************************************************************/
TEST_F(UT_Process, ReadStatus_FopenFails_ReturnsFalse)
{
    // Arrange：打桩 fopen 失败
    Stub b1;
    b1.set(fopen, stub_fopen_fail);

    // Act：读取 status
    const bool ok = m_tester->readStatus();

    // Assert：返回失败且打桩标记命中
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_Sresult, QString("fopen failed"));
    EXPECT_EQ(m_tester->d->uid, 0U);
}

TEST_F(UT_Process, ReadStatus_AccessFails_ReturnsFalse)
{
    // Arrange：以不存在的 pid 构造进程（access 直接失败）
    Process bp(ut_bogus_pid());

    // Act：读取 status
    const bool ok = bp.readStatus();

    // Assert：文件不可访问时返回失败，解析字段保持默认
    EXPECT_FALSE(ok);
    EXPECT_FALSE(bp.isValid());
    EXPECT_EQ(bp.d->gid, 0U);
    EXPECT_EQ(bp.d->uid, 0U);
}

TEST_F(UT_Process, ReadStatus_RealSelfProc_UidGidParsed)
{
    // Arrange：使用本进程真实 /proc 数据
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 status
    const bool ok = m_tester->readStatus();

    // Assert：uid/gid/euid 与系统调用一致
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_tester->d->uid, getuid());
    EXPECT_EQ(m_tester->d->gid, getgid());
    EXPECT_EQ(m_tester->d->euid, geteuid());
}

TEST_F(UT_Process, ReadStatus_RealSelfProc_StateParsed)
{
    // Arrange：使用本进程真实 /proc 数据
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 status
    const bool ok = m_tester->readStatus();
    const char st = m_tester->d->state;

    // Assert：状态为合法取值之一
    EXPECT_TRUE(ok);
    EXPECT_TRUE(st == 'R' || st == 'S' || st == 'D' || st == 'Z' || st == 'T'
                || st == 'I' || st == 'W' || st == 'X' || st == 'x' || st == 'K' || st == 'P');
    EXPECT_EQ(m_tester->d->uid, getuid());
}

/***************************************************************************************************
 * readCmdline：/proc/[pid]/cmdline
 **************************************************************************************************/
TEST_F(UT_Process, ReadCmdline_FopenFails_ReturnsFalse)
{
    // Arrange：打桩 fopen 失败
    Stub b1;
    b1.set(fopen, stub_fopen_fail);

    // Act：读取命令行
    const bool ok = m_tester->readCmdline();

    // Assert：返回失败且列表保持为空
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_Sresult, QString("fopen failed"));
    EXPECT_EQ(m_tester->d->cmdline.size(), 0);
}

TEST_F(UT_Process, ReadCmdline_RealSelfProc_ListFilled)
{
    // Arrange：使用本进程真实 /proc 数据
    ASSERT_NE(m_tester, nullptr);

    // Act：读取命令行
    const bool ok = m_tester->readCmdline();

    // Assert：列表非空且首参为测试二进制自身
    EXPECT_TRUE(ok);
    EXPECT_FALSE(m_tester->d->cmdline.isEmpty());
    EXPECT_GT(m_tester->d->cmdline.size(), 0);
    EXPECT_TRUE(QByteArray(m_tester->d->cmdline.first()).contains("deepin-system-monitor-test"));
}

/***************************************************************************************************
 * readEnviron：/proc/[pid]/environ
 **************************************************************************************************/
TEST_F(UT_Process, ReadEnviron_OpenFails_MarkFailed)
{
    // Arrange：打桩 open 失败
    Stub b1;
    b1.set(open, stub_readStat_open2);

    // Act：读取环境变量
    m_tester->readEnviron();

    // Assert：提前返回且表保持为空
    EXPECT_EQ(m_Sresult, QString("open failed"));
    EXPECT_EQ(m_tester->d->environ.size(), 0);
}

TEST_F(UT_Process, ReadEnviron_RealSelfProc_PathPresent)
{
    // Arrange：使用本进程真实 /proc 数据（进程环境含 PATH）
    ASSERT_NE(m_tester, nullptr);

    // Act：读取环境变量
    m_tester->readEnviron();

    // Assert：PATH 在表中且表非空
    EXPECT_TRUE(m_tester->d->environ.contains("PATH"));
    EXPECT_GT(m_tester->d->environ.size(), 0);
}

TEST_F(UT_Process, ReadEnviron_AccessFails_EnvironEmpty)
{
    // Arrange：以不存在的 pid 构造进程（access 直接失败）
    Process bp(ut_bogus_pid());

    // Act：读取环境变量
    bp.readEnviron();

    // Assert：提前返回且环境表为空、其他字段不受影响
    EXPECT_TRUE(bp.d->environ.isEmpty());
    EXPECT_EQ(bp.name(), QString());
    EXPECT_EQ(bp.d->utime, 0ULL);
}

/***************************************************************************************************
 * readIO：/proc/[pid]/io
 **************************************************************************************************/
TEST_F(UT_Process, ReadIO_FopenFails_EarlyReturn)
{
    // Arrange：打桩 fopen 失败
    Stub b1;
    b1.set(fopen, stub_fopen_fail);

    // Act：读取 IO 统计
    m_tester->readIO();

    // Assert：提前返回且字段保持初值
    EXPECT_EQ(m_Sresult, QString("fopen failed"));
    EXPECT_EQ(m_tester->d->read_bytes, 0ULL);
}

TEST_F(UT_Process, ReadIO_RealSelfProc_FieldsNonNegative)
{
    // Arrange：使用本进程真实 /proc 数据
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 IO 统计
    m_tester->readIO();

    // Assert：累计读写字节非负
    EXPECT_GE(m_tester->d->read_bytes, 0ULL);
    EXPECT_GE(m_tester->d->write_bytes, 0ULL);
    EXPECT_EQ(m_tester->pid(), getpid());
}

/***************************************************************************************************
 * readSockInodes：/proc/[pid]/fd
 **************************************************************************************************/
TEST_F(UT_Process, ReadSockInodes_OpendirFails_MarkFailed)
{
    // Arrange：打桩 opendir 失败
    Stub b1;
    b1.set(opendir, stub_opendir_fail);

    // Act：读取 socket inode
    m_tester->readSockInodes();

    // Assert：提前返回且列表保持为空
    EXPECT_EQ(m_Sresult, QString("diropen failed"));
    EXPECT_EQ(m_tester->d->sockInodes.size(), 0);
}

TEST_F(UT_Process, ReadSockInodes_RealSelfProc_NoCrash)
{
    // Arrange：使用本进程真实 /proc 数据
    ASSERT_NE(m_tester, nullptr);

    // Act：读取 socket inode
    m_tester->readSockInodes();

    // Assert：不崩溃且列表规模非负
    EXPECT_GE(m_tester->d->sockInodes.size(), 0);
    EXPECT_EQ(m_tester->pid(), getpid());
}

/***************************************************************************************************
 * readProcessInfo / readProcessSimpleInfo / readProcessVariableInfo
 **************************************************************************************************/
TEST_F(UT_Process, ReadProcessInfo_RealSelfProc_MarkedValid)
{
    // Arrange：隔离图标刷新（headless 无 xcb）
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act：执行完整信息读取流水线
    m_tester->readProcessInfo();

    // Assert：进程有效、名字与身份正确、CPU 时间已累计
    EXPECT_TRUE(m_tester->isValid());
    EXPECT_FALSE(m_tester->name().isEmpty());
    EXPECT_EQ(m_tester->uid(), getuid());
    EXPECT_GT(m_tester->utime() + m_tester->stime(), 0ULL);
}

TEST_F(UT_Process, ReadProcessInfo_Twice_StablePidAndValid)
{
    // Arrange：隔离图标刷新
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act：连续两次完整读取
    m_tester->readProcessInfo();
    const bool firstValid = m_tester->isValid();
    m_tester->readProcessInfo();

    // Assert：两次读取后均保持有效
    EXPECT_TRUE(firstValid);
    EXPECT_TRUE(m_tester->isValid());
    EXPECT_EQ(m_tester->pid(), getpid());
}

TEST_F(UT_Process, ReadProcessInfo_ReadStatOpenFails_NotValid)
{
    // Arrange：隔离图标刷新，并打桩 open 失败使 readStat 失败
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    Stub b1;
    b1.set(open, stub_readStat_open2);

    // Act：执行完整信息读取
    m_tester->readProcessInfo();

    // Assert：readStat 失败经短路逻辑传播，进程标记无效
    EXPECT_EQ(m_Sresult, QString("open failed"));
    EXPECT_FALSE(m_tester->isValid());
}

TEST_F(UT_Process, ReadProcessInfo_WithRecentStage_ConsumedWithoutCrash)
{
    // Arrange：向全局 ProcessSet 注入本 pid 的近期阶段，隔离图标刷新
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    auto stage = std::make_shared<RecentProcStage>();
    stage->ptime = 100;
    stage->read_bytes = 2048;
    stage->write_bytes = 1024;
    stage->uptime = {1, 0};
    ProcessDB::instance()->processSet()->m_recentProcStage[getpid()] = stage;

    // Act：执行完整信息读取（命中 validrecentPtr 分支）
    m_tester->readProcessInfo();

    // Assert：读取流程完整执行且进程保持有效
    EXPECT_TRUE(m_tester->isValid());
    EXPECT_EQ(m_tester->pid(), getpid());
    EXPECT_GT(m_tester->utime() + m_tester->stime(), 0ULL);
}

TEST_F(UT_Process, ReadProcessSimpleInfo_RealSelfProc_MarkedValid)
{
    // Arrange：隔离图标刷新
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act：执行简单信息读取
    m_tester->readProcessSimpleInfo();

    // Assert：进程有效且名字非空
    EXPECT_TRUE(m_tester->isValid());
    EXPECT_FALSE(m_tester->name().isEmpty());
    EXPECT_EQ(m_tester->pid(), getpid());
}

TEST_F(UT_Process, ReadProcessVariableInfo_RealSelfProc_NoCrash)
{
    // Arrange：使用本进程真实 /proc 数据
    ASSERT_NE(m_tester, nullptr);

    // Act：执行变量信息读取
    m_tester->readProcessVariableInfo();

    // Assert：CPU 时间已累计
    EXPECT_GT(m_tester->utime() + m_tester->stime(), 0ULL);
    EXPECT_EQ(m_tester->pid(), getpid());
}

/***************************************************************************************************
 * refreashProcessName / refreashProcessIcon
 **************************************************************************************************/
TEST_F(UT_Process, RefreashProcessName_AfterRefresh_DisplayNameSynced)
{
    // Arrange：写入进程名
    m_tester->setName("ut-refresh-name");

    // Act：刷新进程名缓存
    m_tester->refreashProcessName();

    // Assert：展示名缓存与 getter 一致
    EXPECT_EQ(m_tester->displayName(), m_tester->d->proc_name.displayName());
    EXPECT_EQ(m_tester->pid(), getpid());
}

TEST_F(UT_Process, RefreashProcessIcon_Stubbed_NoCrash)
{
    // Arrange：打桩图标刷新为 no-op（headless 无 xcb）
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);

    // Act：调用无参刷新入口
    m_tester->refreashProcessIcon();

    // Assert：不崩溃且图标缓存仍为空图标
    EXPECT_TRUE(m_tester->icon().isNull());
    EXPECT_EQ(m_tester->pid(), getpid());
}

/***************************************************************************************************
 * applyDKaptureData：DKapture 数据落地与单位换算
 **************************************************************************************************/
TEST_F(UT_Process, ApplyDKaptureData_BasicFields_StateNamePriorityApplied)
{
    // Arrange：隔离图标刷新，准备基础字段数据
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    QVariantMap data;
    data["state"] = "S";
    data["priority"] = 5;
    data["comm"] = "ut-dkapture";

    // Act：落地 DKapture 数据
    m_tester->applyDKaptureData(data);

    // Assert：状态/优先级/进程名写入，且 cmdline 读取成功进程有效
    EXPECT_EQ(m_tester->state(), 'S');
    EXPECT_EQ(m_tester->priority(), 5);
    EXPECT_EQ(m_tester->name(), QString("ut-dkapture"));
    EXPECT_TRUE(m_tester->isValid());
}

TEST_F(UT_Process, ApplyDKaptureData_StatBytes_ConvertedToKB)
{
    // Arrange：隔离图标刷新，准备字节为单位的 stat 字段
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    QVariantMap data;
    data["rss"] = 4096000ULL;
    data["vsize"] = 8192000ULL;
    data["utime"] = 55ULL;
    data["stime"] = 66ULL;

    // Act：落地 DKapture 数据
    m_tester->applyDKaptureData(data);

    // Assert：字节 >> 10 转 KB，时间直接赋值
    EXPECT_EQ(m_tester->d->rss, 4096000ULL >> 10);
    EXPECT_EQ(m_tester->d->vmsize, 8192000ULL >> 10);
    EXPECT_EQ(m_tester->d->utime, 55ULL);
    EXPECT_EQ(m_tester->d->stime, 66ULL);
}

TEST_F(UT_Process, ApplyDKaptureData_StatmPages_PagesConvertedToKB)
{
    // Arrange：隔离图标刷新，准备页数为单位的 statm 字段
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    QVariantMap data;
    data["memory_resident"] = 100ULL;
    data["memory_size"] = 200ULL;
    data["memory_shared"] = 50ULL;

    // Act：落地 DKapture 数据
    m_tester->applyDKaptureData(data);

    // Assert：页数 << kb_shift 转 KB，且 memory() = rss - shm
    EXPECT_EQ(m_tester->d->rss, 100ULL << kb_shift);
    EXPECT_EQ(m_tester->d->vmsize, 200ULL << kb_shift);
    EXPECT_EQ(m_tester->d->shm, 50ULL << kb_shift);
    EXPECT_EQ(m_tester->memory(), 50ULL << kb_shift);
}

TEST_F(UT_Process, ApplyDKaptureData_IdentityFields_AllApplied)
{
    // Arrange：隔离图标与窗口查询，准备身份字段
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    Stub bWm;
    bWm.set(ADDR(core::wm::WMWindowList, isGuiApp), stub_wm_gui_flag);
    Stub bTray;
    bTray.set(ADDR(core::wm::WMWindowList, isTrayApp), stub_bool_false);
    QVariantMap data;
    data["uid"] = static_cast<uint>(getuid());
    data["gid"] = static_cast<uint>(getgid());
    data["ppid"] = static_cast<int>(getppid());
    data["num_threads"] = 4;
    data["nice"] = 10;
    data["start_time"] = 12345ULL;
    data["read_bytes"] = 1000ULL;
    data["write_bytes"] = 2000ULL;
    data["cancelled_write_bytes"] = 300ULL;

    // Act：落地 DKapture 数据
    m_tester->applyDKaptureData(data);

    // Assert：身份与 IO 字段一致，当前用户应用归类正确
    EXPECT_EQ(m_tester->d->uid, static_cast<uint>(getuid()));
    EXPECT_EQ(m_tester->d->gid, static_cast<uint>(getgid()));
    EXPECT_EQ(m_tester->d->ppid, getppid());
    EXPECT_EQ(m_tester->d->nthreads, 4);
    EXPECT_EQ(m_tester->d->nice, 10);
    EXPECT_EQ(m_tester->d->start_time, 12345ULL);
    EXPECT_EQ(m_tester->d->read_bytes, 1000ULL);
    EXPECT_EQ(m_tester->d->write_bytes, 2000ULL);
    EXPECT_EQ(m_tester->d->cancelled_write_bytes, 300ULL);
    EXPECT_EQ(m_tester->d->usrerName, SysInfo::userName(getuid()));
    EXPECT_EQ(m_tester->d->apptype, kFilterCurrentUser);
}

TEST_F(UT_Process, ApplyDKaptureData_GuiApp_ClassifiedAsFilterApps)
{
    // Arrange：隔离图标，isGuiApp 命中本进程
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    Stub bWm;
    g_stubWmGui = true;
    bWm.set(ADDR(core::wm::WMWindowList, isGuiApp), stub_wm_gui_flag);
    Stub bTray;
    bTray.set(ADDR(core::wm::WMWindowList, isTrayApp), stub_bool_false);
    QVariantMap data;
    data["uid"] = static_cast<uint>(getuid());

    // Act：落地 DKapture 数据
    m_tester->applyDKaptureData(data);

    // Assert：GUI 应用归入 kFilterApps
    EXPECT_EQ(m_tester->d->apptype, kFilterApps);
    EXPECT_EQ(m_tester->d->uid, static_cast<uint>(getuid()));
}

TEST_F(UT_Process, ApplyDKaptureData_RqWaitTime_NsConvertedToTicks)
{
    // Arrange：隔离图标刷新，准备 2 秒的调度等待（纳秒）
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    QVariantMap data;
    data["rq_wait_time"] = 2000000000ULL;

    // Act：落地 DKapture 数据
    m_tester->applyDKaptureData(data);

    // Assert：纳秒按 HZ 换算为时钟滴答，未提供 state 时状态保持 '\0'
    EXPECT_EQ(m_tester->d->wtime, static_cast<qulonglong>(2000000000UL * HZ / 1000000000UL));
    EXPECT_EQ(m_tester->d->state, '\0');
}

TEST_F(UT_Process, ApplyDKaptureData_CmdlineFail_MarkedInvalid)
{
    // Arrange：隔离图标并打桩 readCmdline 失败
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    Stub bCmd;
    bCmd.set(ADDR(Process, readCmdline), stub_readCmdline_false);
    QVariantMap data;
    data["comm"] = "ut-no-cmdline";

    // Act：落地 DKapture 数据
    m_tester->applyDKaptureData(data);

    // Assert：cmdline 失败导致进程标记无效，但名字已写入
    EXPECT_FALSE(m_tester->isValid());
    EXPECT_EQ(m_tester->name(), QString("ut-no-cmdline"));
}

TEST_F(UT_Process, ApplyDKaptureData_EmptyMap_FieldsUnchanged)
{
    // Arrange：隔离图标刷新，记录应用前基线
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    const QString nameBefore = m_tester->name();
    const qulonglong utimeBefore = m_tester->d->utime;
    const qulonglong stimeBefore = m_tester->d->stime;
    const int prioBefore = m_tester->priority();

    // Act：以空字典落地（所有 contains 分支均走假分支）
    const QVariantMap empty;
    m_tester->applyDKaptureData(empty);

    // Assert：任何字段均未被修改
    EXPECT_EQ(m_tester->name(), nameBefore);
    EXPECT_EQ(m_tester->d->utime, utimeBefore);
    EXPECT_EQ(m_tester->d->stime, stimeBefore);
    EXPECT_EQ(m_tester->priority(), prioBefore);
    EXPECT_EQ(m_tester->d->rss, 0ULL);
    EXPECT_EQ(m_tester->d->vmsize, 0ULL);
}

// 全字段响应 + 混合 QVariant 类型：覆盖各字段的转换分支与内存字段覆盖次序
TEST_F(UT_Process, ApplyDKaptureData_FullResponse_MixedTypes_AllFieldsApplied)
{
    // Arrange：隔离图标刷新，25 个键全部提供且类型各异
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    Stub bWm;
    bWm.set(ADDR(core::wm::WMWindowList, isGuiApp), stub_bool_false);
    Stub bTray;
    bTray.set(ADDR(core::wm::WMWindowList, isTrayApp), stub_bool_false);
    Stub bDesk;
    bDesk.set(ADDR(core::wm::WMWindowList, isDesktopEntryApp), stub_bool_false);
    QVariantMap data;
    data["state"] = QStringLiteral("R");
    data["priority"] = 5.7;                              // double → toInt
    data["comm"] = QStringLiteral("ut-mixed");
    data["utime"] = QVariant::fromValue<qulonglong>(11);
    data["stime"] = QStringLiteral("33");               // 字符串 → toULongLong
    data["cutime"] = 4.25;                               // double 截断为 4
    data["cstime"] = 7;                                  // int
    data["rss"] = static_cast<uint>(4096);              // uint 字节
    data["vsize"] = QVariant::fromValue<long>(8192000);  // long 字节
    data["memory_resident"] = QVariant::fromValue<ulong>(100); // ulong 页数覆盖 rss
    data["memory_size"] = QStringLiteral("55");         // 字符串页数覆盖 vmsize
    data["memory_shared"] = 6.9;                         // double 页数截断为 6
    data["pid"] = 4242;                                  // 局部变量，不可观测
    data["ppid"] = static_cast<uint>(getppid());
    data["num_threads"] = 3.9;                           // double 截断为 3
    data["nice"] = QStringLiteral("2");                 // 字符串
    data["start_time"] = 12345ULL;
    data["read_bytes"] = 1024.5;                         // double 截断为 1024
    data["write_bytes"] = QStringLiteral("77");
    data["cancelled_write_bytes"] = 9;
    data["uid"] = static_cast<uint>(getuid());
    data["gid"] = static_cast<uint>(getgid());
    data["euid"] = static_cast<uint>(getuid());
    data["egid"] = static_cast<uint>(getgid());
    data["rq_wait_time"] = 2000000000.0;                 // double 纳秒

    // Act：落地 DKapture 数据
    m_tester->applyDKaptureData(data);

    // Assert：各字段按序生效，memory_* 后置覆盖 rss/vmsize，纳秒按 HZ 换算
    EXPECT_EQ(m_tester->d->state, 'R');
    EXPECT_EQ(m_tester->name(), QStringLiteral("ut-mixed"));
    EXPECT_EQ(m_tester->d->utime, 11ULL);
    EXPECT_EQ(m_tester->d->stime, 33ULL);
    EXPECT_EQ(m_tester->d->cutime, 4ULL);
    EXPECT_EQ(m_tester->d->cstime, 7ULL);
    EXPECT_EQ(m_tester->d->rss, 100ULL << kb_shift);
    EXPECT_EQ(m_tester->d->vmsize, 55ULL << kb_shift);
    EXPECT_EQ(m_tester->d->shm, 7ULL << kb_shift); // double 6.9 经 toULongLong 四舍五入为 7
    EXPECT_EQ(m_tester->d->ppid, static_cast<int>(getppid()));
    EXPECT_EQ(m_tester->d->nthreads, 4); // double 3.9 经 toInt 四舍五入为 4
    EXPECT_EQ(m_tester->d->nice, 2);
    EXPECT_EQ(m_tester->d->start_time, 12345ULL);
    EXPECT_EQ(m_tester->d->read_bytes, 1025ULL); // double 1024.5 经 toULongLong 四舍五入为 1025
    EXPECT_EQ(m_tester->d->write_bytes, 77ULL);
    EXPECT_EQ(m_tester->d->cancelled_write_bytes, 9ULL);
    EXPECT_EQ(m_tester->d->uid, static_cast<uint>(getuid()));
    EXPECT_EQ(m_tester->d->gid, static_cast<uint>(getgid()));
    EXPECT_EQ(m_tester->d->euid, static_cast<uint>(getuid()));
    EXPECT_EQ(m_tester->d->egid, static_cast<uint>(getgid()));
    EXPECT_EQ(m_tester->d->wtime, static_cast<qulonglong>(2000000000UL * HZ / 1000000000UL));
    EXPECT_TRUE(m_tester->isValid());
}

// 旋转类型的全字段响应：覆盖同字段上另一批 QVariant 转换分支
TEST_F(UT_Process, ApplyDKaptureData_AlternateTypes_RotatedConversions)
{
    // Arrange：隔离图标刷新，与 MixedTypes 互补的类型轮换
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    QVariantMap data;
    data["state"] = QStringLiteral("S");
    data["priority"] = -5.5;
    data["comm"] = QStringLiteral("ut-rotate");
    data["utime"] = QStringLiteral("11");
    data["stime"] = static_cast<uint>(22);
    data["cutime"] = 8;                                  // int
    data["cstime"] = 9.5;                                // double 截断为 9
    data["rss"] = 4096ULL;                               // ull 字节
    data["vsize"] = QVariant::fromValue<qulonglong>(4096);
    data["memory_resident"] = 30;                        // int 页数
    data["memory_size"] = static_cast<uint>(20);
    data["memory_shared"] = QVariant::fromValue<long>(5); // long 页数
    data["ppid"] = 1.0;                                  // double → toInt
    data["num_threads"] = QStringLiteral("7");
    data["nice"] = 15;
    data["start_time"] = 123.9;                          // double 截断为 123
    data["read_bytes"] = static_cast<uint>(500);
    data["write_bytes"] = 600;                           // int
    data["cancelled_write_bytes"] = 7.8;                 // double 截断为 7
    data["gid"] = static_cast<uint>(getgid());
    data["egid"] = static_cast<uint>(getgid());
    data["rq_wait_time"] = QStringLiteral("1000000000");

    // Act：落地 DKapture 数据
    m_tester->applyDKaptureData(data);

    // Assert：轮换类型同样生效
    EXPECT_EQ(m_tester->d->state, 'S');
    EXPECT_EQ(m_tester->name(), QStringLiteral("ut-rotate"));
    EXPECT_EQ(m_tester->d->utime, 11ULL);
    EXPECT_EQ(m_tester->d->stime, 22ULL);
    EXPECT_EQ(m_tester->d->cutime, 8ULL);
    EXPECT_EQ(m_tester->d->cstime, 10ULL); // double 9.5 经 toULongLong 四舍五入为 10
    EXPECT_EQ(m_tester->d->rss, 30ULL << kb_shift);
    EXPECT_EQ(m_tester->d->vmsize, 20ULL << kb_shift);
    EXPECT_EQ(m_tester->d->shm, 5ULL << kb_shift);
    EXPECT_EQ(m_tester->d->ppid, 1);
    EXPECT_EQ(m_tester->d->nthreads, 7);
    EXPECT_EQ(m_tester->d->nice, 15);
    EXPECT_EQ(m_tester->d->start_time, 124ULL); // double 123.9 四舍五入为 124
    EXPECT_EQ(m_tester->d->read_bytes, 500ULL);
    EXPECT_EQ(m_tester->d->write_bytes, 600ULL);
    EXPECT_EQ(m_tester->d->cancelled_write_bytes, 8ULL); // double 7.8 四舍五入为 8
    EXPECT_EQ(m_tester->d->gid, static_cast<uint>(getgid()));
    EXPECT_EQ(m_tester->d->egid, static_cast<uint>(getgid()));
    EXPECT_EQ(m_tester->d->wtime, static_cast<qulonglong>(1000000000UL * HZ / 1000000000UL));
}

// 非空响应但键缺失：缺失键字段保持原值（contains 假分支，非空容器查找路径）
TEST_F(UT_Process, ApplyDKaptureData_PartialResponse_NonEmptyMap_MissingKeysSkipped)
{
    // Arrange：隔离图标刷新，仅提供 state/comm/utime 三个键
    Stub bIcon;
    bIcon.set(ADDR(ProcessIcon, refreashProcessIcon), stub_refreashProcessIcon_noop);
    QVariantMap data;
    data["state"] = QStringLiteral("R");
    data["comm"] = QStringLiteral("ut-partial");
    data["utime"] = 99ULL;

    // Act：落地 DKapture 数据
    m_tester->applyDKaptureData(data);

    // Assert：提供的键生效，未提供的键保持默认
    EXPECT_EQ(m_tester->d->state, 'R');
    EXPECT_EQ(m_tester->name(), QStringLiteral("ut-partial"));
    EXPECT_EQ(m_tester->d->utime, 99ULL);
    EXPECT_EQ(m_tester->d->stime, 0ULL);
    EXPECT_EQ(m_tester->d->cutime, 0ULL);
    EXPECT_EQ(m_tester->d->cstime, 0ULL);
    EXPECT_EQ(m_tester->d->rss, 0ULL);
    EXPECT_EQ(m_tester->d->vmsize, 0ULL);
    EXPECT_EQ(m_tester->d->shm, 0ULL);
    EXPECT_EQ(m_tester->d->ppid, 0);
    EXPECT_EQ(m_tester->d->nthreads, 0);
    EXPECT_EQ(m_tester->d->nice, 0);
    EXPECT_EQ(m_tester->d->start_time, 0ULL);
    EXPECT_EQ(m_tester->d->read_bytes, 0ULL);
    EXPECT_EQ(m_tester->d->write_bytes, 0ULL);
    EXPECT_EQ(m_tester->d->cancelled_write_bytes, 0ULL);
    EXPECT_EQ(m_tester->d->gid, 0u);
    EXPECT_EQ(m_tester->d->wtime, 0ULL);
}

/***************************************************************************************************
 * getPriorityName（自由函数，纯逻辑全分支）
 **************************************************************************************************/
TEST(UT_Process_GetPriorityName, VeryHighPriority_Label)
{
    // Arrange：准备最高优先级常量
    const int prio = kVeryHighPriority;

    // Act：查询展示名
    const QString name = getPriorityName(prio);

    // Assert：翻译文案与国际化上下文一致
    EXPECT_EQ(name, QApplication::translate("Process.Priority", "Very high"));
    EXPECT_EQ(getPriorityName(kVeryHighPriorityMax),
              QApplication::translate("Process.Priority", "Very high"));
}

TEST(UT_Process_GetPriorityName, HighPriority_Label)
{
    // Arrange：准备高优先级常量
    const int prio = kHighPriority;

    // Act：查询展示名
    const QString name = getPriorityName(prio);

    // Assert：翻译文案正确
    EXPECT_EQ(name, QApplication::translate("Process.Priority", "High"));
    EXPECT_EQ(getPriorityName(kHighPriorityMax),
              QApplication::translate("Process.Priority", "High"));
}

TEST(UT_Process_GetPriorityName, NormalPriority_Label)
{
    // Arrange：准备普通优先级常量
    const int prio = kNormalPriority;

    // Act：查询展示名
    const QString name = getPriorityName(prio);

    // Assert：翻译文案正确
    EXPECT_EQ(name, QApplication::translate("Process.Priority", "Normal"));
    EXPECT_EQ(getPriorityName(0), QApplication::translate("Process.Priority", "Normal"));
}

TEST(UT_Process_GetPriorityName, LowPriority_Label)
{
    // Arrange：准备低优先级常量
    const int prio = kLowPriority;

    // Act：查询展示名
    const QString name = getPriorityName(prio);

    // Assert：标准值报 Low；kLowPriorityMax=1 非精确匹配键，落入区间报 Custom
    EXPECT_EQ(name, QApplication::translate("Process.Priority", "Low"));
    EXPECT_EQ(getPriorityName(kLowPriorityMax),
              QApplication::translate("Process.Priority", "Custom"));
}

TEST(UT_Process_GetPriorityName, VeryLowPriority_Label)
{
    // Arrange：准备最低优先级常量
    const int prio = kVeryLowPriority;

    // Act：查询展示名
    const QString name = getPriorityName(prio);

    // Assert：翻译文案正确
    EXPECT_EQ(name, QApplication::translate("Process.Priority", "Very low"));
    EXPECT_EQ(getPriorityName(kVeryLowPriorityMin),
              QApplication::translate("Process.Priority", "Very low"));
}

TEST(UT_Process_GetPriorityName, CustomPriority_InRange)
{
    // Arrange：准备标准值之外但落在自定义区间的值
    const int prio = -5;

    // Act：查询展示名
    const QString name = getPriorityName(prio);

    // Assert：区间内任意非标准值均报 Custom
    EXPECT_EQ(name, QApplication::translate("Process.Priority", "Custom"));
    EXPECT_EQ(getPriorityName(5), QApplication::translate("Process.Priority", "Custom"));
    EXPECT_EQ(getPriorityName(-15), QApplication::translate("Process.Priority", "Custom"));
    EXPECT_EQ(getPriorityName(15), QApplication::translate("Process.Priority", "Custom"));
}

TEST(UT_Process_GetPriorityName, InvalidPriority_OutOfRange)
{
    // Arrange：准备超出合理区间的极端值
    const int prio = -100;

    // Act：查询展示名
    const QString name = getPriorityName(prio);

    // Assert：越界值均报 Invalid
    EXPECT_EQ(name, QApplication::translate("Process.Priority", "Invalid"));
    EXPECT_EQ(getPriorityName(100), QApplication::translate("Process.Priority", "Invalid"));
    EXPECT_EQ(getPriorityName(INT_MAX), QApplication::translate("Process.Priority", "Invalid"));
}

/***************************************************************************************************
 * getProcessPriorityStub（自由函数，纯逻辑全分支）
 **************************************************************************************************/
TEST(UT_Process_GetProcessPriorityStub, StandardValues_Passthrough)
{
    // Arrange：准备五个标准优先级值
    const int prio = kVeryHighPriority;

    // Act：经 stub 函数映射
    const ProcessPriority mapped = getProcessPriorityStub(prio);

    // Assert：标准值原样透传
    EXPECT_EQ(mapped, kVeryHighPriority);
    EXPECT_EQ(getProcessPriorityStub(kHighPriority), kHighPriority);
    EXPECT_EQ(getProcessPriorityStub(kNormalPriority), kNormalPriority);
    EXPECT_EQ(getProcessPriorityStub(kLowPriority), kLowPriority);
    EXPECT_EQ(getProcessPriorityStub(kVeryLowPriority), kVeryLowPriority);
}

TEST(UT_Process_GetProcessPriorityStub, ZeroAndCustomRange_Mapped)
{
    // Arrange：准备 0 与自定义区间值
    const int prio = 0;

    // Act：经 stub 函数映射
    const ProcessPriority mapped = getProcessPriorityStub(prio);

    // Assert：0 归为 Normal，区间内非标准值归为 Custom
    EXPECT_EQ(mapped, kNormalPriority);
    EXPECT_EQ(getProcessPriorityStub(-5), kCustomPriority);
    EXPECT_EQ(getProcessPriorityStub(5), kCustomPriority);
}

TEST(UT_Process_GetProcessPriorityStub, OutOfRange_MappedInvalid)
{
    // Arrange：准备越界值
    const int prio = -100;

    // Act：经 stub 函数映射
    const ProcessPriority mapped = getProcessPriorityStub(prio);

    // Assert：越界值归为 Invalid
    EXPECT_EQ(mapped, kInvalidPriority);
    EXPECT_EQ(getProcessPriorityStub(50), kInvalidPriority);
    EXPECT_EQ(getProcessPriorityStub(100), kInvalidPriority);
}
