// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 最小清单（生成前完成情况）：
// | # | 检查项 | 完成情况 |
// | 1 | 每个公开方法 ≥ 1 用例 | ✅ ctor/四个 getter/~NetInfo/resdNetInfo 全覆盖（~NetInfo 经 TearDown 间接覆盖） |
// | 2 | 输入维度等价类划分 | ✅ /proc/net/dev 桩文件：打开失败/首轮/次轮/含畸形行/全无效行 |
// | 3 | 边界值显式覆盖 | ✅ 首轮(m_netStat[kLastStat] 为空)、次轮零增量(bps=0)、fopen 失败早退 |
// | 4 | 同质多组输入 TEST_P | ✅ N/A：各场景桩文件内容结构与断言焦点不同（首轮 vs 次轮 vs 畸形行），独立用例表达更准确 |
// | 5 | 分支清单已列出并映射用例 | ✅ 下方分支清单（来源：get_code_snippet 实源码，口径与 extract_branches 一致：8 if + 1 while + 1 return = 10） |
// | 6 | 每条 if/循环/早退 return 分支有用例 | ✅ 见分支清单映射 |
// | 7 | EXPECT_THROW 精确类型 | ✅ N/A：resdNetInfo 无异常路径，失败均以状态保持/警告表达 |
// | 8 | 负面场景专门用例 | ✅ 打开失败(002)、畸形接口名/统计字段(005)、全无效行(006) |
// | 9 | 负面用例强异常安全 | ✅ 失败路径后断言成员状态保持初值/首轮语义，无野指针（QSharedPointer 管理） |
// | 10 | 打桩选择 | ✅ fopen（外部 /proc 文件）与 SysInfo::uptime（系统时钟）打桩；测试数据写入 QTemporaryDir，不触真实 /proc |

// 用例计数声明表（方法 | level | factors | min | actual）
// | NetInfo::NetInfo | low | - | 1 | 1 |
// | ~NetInfo | low | destructor | 1 | 1 |
// | recvBps | low | - | 1 | 5 |
// | sentBps | low | - | 1 | 5 |
// | totalRecvBytes | low | - | 1 | 4 |
// | totalSentBytes | low | - | 1 | 4 |
// | resdNetInfo | high | complexity:9,lines:110,linear_scan_in_loop:1 | 4 | 5 |

//self
#include "system/net_info.h"
#include "system/sys_info.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>

#include <cstdio>

using namespace core::system;

/***************************************STUB begin*********************************************/
// fopen 桩：将 /proc/net/dev 读取重定向到测试预备的临时文件句柄（ nullptr 模拟打开失败）。
static FILE *g_fakeFile = nullptr;
static FILE *stub_fopen(const char *, const char *)
{
    return g_fakeFile;
}

// uptime 桩：固定返回同一时钟值，使 resdNetInfo 的 interval 走 else 分支恒为 1，
// 保证 recvBps/sentBps 断言确定性。
static timeval stub_uptime(void *)
{
    return timeval {1234, 5678};
}
/***************************************STUB end**********************************************/

namespace {
// 向临时目录写入模拟 /proc/net/dev 内容（真实格式：2 头行 + 每接口 16 个数字字段）
static void writeDevFile(const QString &path, const QString &content)
{
    QFile f(path);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(content.toUtf8());
}

// 两接口（lo + eth0）的合法样例：rx_bytes=1000+5000=6000, tx_bytes=2000+7000=9000
static const char *kDevContentA =
    "Inter-|   Receive                                                |  Transmit\n"
    " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
    "    lo: 1000 10 0 0 0 0 0 0        2000 20 0 0 0 0 0 0\n"
    "  eth0: 5000 50 0 0 0 0 0 0        7000 70 0 0 0 0 0 0\n";

// 次轮样例：rx_bytes=3000+7000=10000, tx_bytes=4000+8000=12000
static const char *kDevContentB =
    "Inter-|   Receive                                                |  Transmit\n"
    " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
    "    lo: 3000 30 0 0 0 0 0 0        4000 40 0 0 0 0 0 0\n"
    "  eth0: 7000 70 0 0 0 0 0 0        8000 80 0 0 0 0 0 0\n";

// 混入畸形行：头两行无冒号(B3)、空接口名 sscanf 失败(B4)、统计字段不足 rc!=7(B5)，仅 lo 合法
static const char *kDevContentMalformed =
    "Inter-|   Receive                                                |  Transmit\n"
    " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
    ": 999 9 0 0 0 0 0 0        999 9 0 0 0 0 0 0\n"
    "  eth0: garbage\n"
    "    lo: 1000 10 0 0 0 0 0 0        2000 20 0 0 0 0 0 0\n";

// 全无效行：无任何合法接口 => b=false
static const char *kDevContentAllInvalid =
    "Inter-|   Receive                                                |  Transmit\n"
    " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
    ": 999 9 0 0 0 0 0 0        999 9 0 0 0 0 0 0\n"
    "  eth0: garbage\n";
} // namespace

class UT_NetInfo: public ::testing::Test
{
public:
    UT_NetInfo() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 预热 SysInfo 单例：避免其构造期的 /proc 读取被 fopen 桩干扰
        SysInfo::instance();
        m_tester = new NetInfo();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            // 间接覆盖 ~NetInfo
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetInfo *m_tester;
};

// ============================================================================
// 分支清单（来源：NetInfo::resdNetInfo，get_code_snippet 实源码；
// 实际 10 分支 = 8 if + 1 while + 1 早退 return；rxdiff/txdiff/interval 三处
// 三目运算经 extract_branches 验证不计入分支数）
// B1: if fopen==nullptr → 警告并 return → 002
// B2: while fgets 逐行遍历 → 003/004/005/006
// B3: if !pos（行无冒号）→ continue → 005（两行表头）
// B4: if sscanf(接口名)!=1 → continue → 005（空接口名行）
// B5: if sscanf(统计)!=7 → 警告+continue → 005（garbage 行）
// B6: if !b（ferror 或无有效行）→ 警告 → 006
// B7: if kCurrentStat 非空 → 取 crxb/ctxb → 003/004/005/006
// B8: if kLastStat 非空 → 取 prxb/ptxb → 004/005（003 为否分支）
// B9: if kLastStat 为空 → 首轮 bps=1 并 return → 003/005/006
// B10: if rtime>ltime → interval=rtime-ltime，否则 1 → 004/005（uptime 桩恒定 → 走 else=1）
// ============================================================================

// 构造后初始状态：四个指标均为 0
TEST_F(UT_NetInfo, netInfo_Ctor_ZeroInitialState_001)
{
    // Arrange：断言夹具已就绪
    ASSERT_NE(m_tester, nullptr);
    // Act：读取四个 getter
    qreal rb = m_tester->recvBps();
    qreal sb = m_tester->sentBps();
    qulonglong trb = m_tester->totalRecvBytes();
    qulonglong tsb = m_tester->totalSentBytes();

    // Assert：零初始状态
    EXPECT_EQ(rb, 0);
    EXPECT_EQ(sb, 0);
    EXPECT_EQ(trb, 0u);
    EXPECT_EQ(tsb, 0u);
}

// fopen 失败 => 早退，状态不受影响
TEST_F(UT_NetInfo, resdNetInfo_OpenFails_StateUntouched_002)
{
    // Arrange：fopen 桩返回 nullptr
    Stub stub;
    stub.set(fopen, stub_fopen);
    stub.set(ADDR(SysInfo, uptime), stub_uptime);
    g_fakeFile = nullptr;

    // Act
    m_tester->resdNetInfo();

    // Assert：早退，各成员保持初值
    EXPECT_EQ(m_tester->recvBps(), 0);
    EXPECT_EQ(m_tester->sentBps(), 0);
    EXPECT_EQ(m_tester->totalRecvBytes(), 0u);
    EXPECT_EQ(m_tester->totalSentBytes(), 0u);
}

// 首轮（kLastStat 为空）=> bps 置 1，总量不更新
TEST_F(UT_NetInfo, resdNetInfo_FirstRun_BpsIsOneTotalsZero_003)
{
    // Arrange：临时文件 A（sum rx=6000, tx=9000），fopen 桩指向它
    QTemporaryDir dir;
    QString path = dir.path() + "/proc_net_dev_A";
    writeDevFile(path, QLatin1String(kDevContentA));
    FILE *fp = fopen(path.toLocal8Bit().constData(), "r");
    ASSERT_NE(fp, nullptr);

    Stub stub;
    stub.set(fopen, stub_fopen);
    stub.set(ADDR(SysInfo, uptime), stub_uptime);
    g_fakeFile = fp;

    // Act
    m_tester->resdNetInfo();

    // Assert：首轮语义
    EXPECT_EQ(m_tester->recvBps(), 1);
    EXPECT_EQ(m_tester->sentBps(), 1);
    EXPECT_EQ(m_tester->totalRecvBytes(), 0u);
    EXPECT_EQ(m_tester->totalSentBytes(), 0u);
}

// 次轮（kLastStat 非空）=> 总量取当前值，bps = 增量/interval(桩恒定=1)
TEST_F(UT_NetInfo, resdNetInfo_SecondRun_ComputesTotalsAndBps_004)
{
    // Arrange：两份样例文件，fopen 桩先后指向 A、B
    QTemporaryDir dir;
    QString pathA = dir.path() + "/proc_net_dev_A";
    QString pathB = dir.path() + "/proc_net_dev_B";
    writeDevFile(pathA, QLatin1String(kDevContentA));
    writeDevFile(pathB, QLatin1String(kDevContentB));
    FILE *fpA = fopen(pathA.toLocal8Bit().constData(), "r");
    FILE *fpB = fopen(pathB.toLocal8Bit().constData(), "r");
    ASSERT_NE(fpA, nullptr);
    ASSERT_NE(fpB, nullptr);

    Stub stub;
    stub.set(fopen, stub_fopen);
    stub.set(ADDR(SysInfo, uptime), stub_uptime);
    g_fakeFile = fpA;
    m_tester->resdNetInfo();   // 首轮：sum rx=6000, tx=9000

    // Act：次轮换文件 B（sum rx=10000, tx=12000）
    g_fakeFile = fpB;
    m_tester->resdNetInfo();

    // Assert：增量 rx=4000、tx=3000，interval=1
    EXPECT_EQ(m_tester->recvBps(), 4000);
    EXPECT_EQ(m_tester->sentBps(), 3000);
    EXPECT_EQ(m_tester->totalRecvBytes(), 10000u);
    EXPECT_EQ(m_tester->totalSentBytes(), 12000u);
}

// 混入畸形行：无冒号/空接口名/统计字段不足的行被跳过，仅合法行计入
TEST_F(UT_NetInfo, resdNetInfo_MalformedLines_SkipsInvalidRows_005)
{
    // Arrange：两份相同内容的畸形样例文件（仅 lo 合法：rx=1000, tx=2000），
    // 生产代码内部会 fclose 桩返回的句柄，故每次调用需独立句柄
    QTemporaryDir dir;
    QString path1 = dir.path() + "/proc_net_dev_M1";
    QString path2 = dir.path() + "/proc_net_dev_M2";
    writeDevFile(path1, QLatin1String(kDevContentMalformed));
    writeDevFile(path2, QLatin1String(kDevContentMalformed));
    FILE *fp1 = fopen(path1.toLocal8Bit().constData(), "r");
    FILE *fp2 = fopen(path2.toLocal8Bit().constData(), "r");
    ASSERT_NE(fp1, nullptr);
    ASSERT_NE(fp2, nullptr);

    Stub stub;
    stub.set(fopen, stub_fopen);
    stub.set(ADDR(SysInfo, uptime), stub_uptime);
    g_fakeFile = fp1;
    m_tester->resdNetInfo();   // 首轮：bps=1

    // Act：次轮读第二份相同文件
    g_fakeFile = fp2;
    m_tester->resdNetInfo();

    // Assert：总量仅含 lo；同内容零增量 => bps=0
    EXPECT_EQ(m_tester->totalRecvBytes(), 1000u);
    EXPECT_EQ(m_tester->totalSentBytes(), 2000u);
    EXPECT_EQ(m_tester->recvBps(), 0);
    EXPECT_EQ(m_tester->sentBps(), 0);
}

// 全部行无效 => b=false 走警告分支，聚合值为零，首轮语义 bps=1
TEST_F(UT_NetInfo, resdNetInfo_AllInvalidRows_WarnsAndZeroSum_006)
{
    // Arrange：全无效样例文件
    QTemporaryDir dir;
    QString path = dir.path() + "/proc_net_dev_I";
    writeDevFile(path, QLatin1String(kDevContentAllInvalid));
    FILE *fp = fopen(path.toLocal8Bit().constData(), "r");
    ASSERT_NE(fp, nullptr);

    Stub stub;
    stub.set(fopen, stub_fopen);
    stub.set(ADDR(SysInfo, uptime), stub_uptime);
    g_fakeFile = fp;

    // Act
    m_tester->resdNetInfo();

    // Assert：聚合为零 + 首轮 bps=1（总量不更新）
    EXPECT_EQ(m_tester->recvBps(), 1);
    EXPECT_EQ(m_tester->sentBps(), 1);
    EXPECT_EQ(m_tester->totalRecvBytes(), 0u);
    EXPECT_EQ(m_tester->totalSentBytes(), 0u);
}
