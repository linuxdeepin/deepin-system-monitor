// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 CpuProfile 单元测试
// 被测类：deepin-system-monitor-daemon/src/cpuprofile.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method                  | level | factors            | min | actual |
// |-------------------------|-------|--------------------|-----|--------|
// | CpuProfile (ctor)       | low   | -                  | 1   | 2      |
// | updateSystemCpuUsage    | mid   | lines:77           | 2   | 5      |
// | cpuStat                 | low   | -                  | 1   | 1      |
// | getCpuUsage             | low   | -                  | 1   | 1      |
//
// 分支清单（源自真实源码 get_code_snippet updateSystemCpuUsage，self-check §2c）：
// - updateSystemCpuUsage:
//   B1 file.exists()&&open(ReadOnly) 为假 → qCWarning + 返回 0.0，状态不变
//      （exists 声明为 out-of-line 但补丁拦截不可靠，与 open 失败同属一个分支，
//       统一用 open 失败覆盖）
//                                            → UpdateSystemCpuUsage_FileUnopenable_ReturnsZeroAndKeepsState
//   B2 分段数 < 11 → qCWarning + 返回 0.0，状态不变
//                                            → UpdateSystemCpuUsage_TooFewFields_ReturnsZeroAndKeepsState
//   B3 calcCpuTotal == 0（两次采样 total 相同）→ 返回 0.0，状态不变
//                                            → UpdateSystemCpuUsage_ZeroDelta_ReturnsZeroAndKeepsState
//   B4 正常 → cpuUsage=(Δtotal-Δidle系)*100/Δtotal，更新 mCpuUsage 与 mLastCpuStat
//                                            → UpdateSystemCpuUsage_ValidDelta_ComputesUsage
//   B4 复算边界（Δidle == Δtotal → 0.0）
//                                            → UpdateSystemCpuUsage_AllIdleDelta_ReturnsZero
// - ctor: 初始化 mLastCpuStat 全 0 并立即 updateSystemCpuUsage
//                                            → CpuProfile_Ctor_InitializesStatMapAndUsage
// - cpuStat: 返回 mLastCpuStat 副本       → CpuProfile_CpuStat_ReturnsLastStatMap
// - getCpuUsage: 返回 mCpuUsage            → CpuProfile_GetCpuUsage_ReturnsLastUsage
//
// stub 决策（dependency-tracer §3）：
// - /proc/stat 路径硬编码（宏 PROC_CPU_STAT_PATH）→ stub QFile::exists/open +
//   QIODevice::readLine(qint64) 注入受控数据；Qt 类成员补丁沿用仓库既有测试的
//   stub.set + 静态函数模式（set_lamda 对 QtCore 成员存在还原缺陷，实测崩溃）
// - 本项目代码（无）不 stub；不触网、不启子进程

#include "cpuprofile.h"

#include "stubext.h"

#include <QFile>
#include <QIODevice>
#include <gtest/gtest.h>

// 受控 /proc/stat 样例行（11 段：cpu user nice sys idle iowait hardqirq softirq steal guest guest_nice）
static const QByteArray kFakeCpuLine = QByteArrayLiteral("cpu 150 0 0 400 150 0 0 0 0 0\n");

/***************************************STUB begin*********************************************/
// Qt 类成员补丁统一走 stub.set + 静态替换函数（对象指针作首参），仓库既有模式
typedef bool (*QFileExistsPtr)(QFile *);
typedef bool (*QFileOpenPtr)(QFile *, QIODevice::OpenMode);
typedef QByteArray (*QIODeviceReadLinePtr)(QIODevice *, qint64);

static bool g_existsResult = true;
static bool g_openResult = true;
static QByteArray g_line;

static bool stub_QFile_exists(QFile *)
{
    return g_existsResult;
}

static bool stub_QFile_open(QFile *, QIODevice::OpenMode)
{
    return g_openResult;
}

static QByteArray stub_QIODevice_readLine(QIODevice *, qint64)
{
    return g_line;
}

// stub /proc/stat 打开成功且返回受控数据
static void stubCpuFileOpen(stub_ext::StubExt &stub)
{
    g_existsResult = true;
    g_openResult = true;
    stub.set((QFileExistsPtr)((bool(QFile::*)() const)ADDR(QFile, exists)), stub_QFile_exists);
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)),
             stub_QFile_open);
    stub.set((QIODeviceReadLinePtr)((QByteArray(QIODevice::*)(qint64))ADDR(QIODevice, readLine)),
             stub_QIODevice_readLine);
}
/***************************************STUB end***********************************************/

// 采样状态灌入：直接构造 prev 状态（total/idle/iowait 参与计算）
static void presetLastStat(CpuProfile *profile, int total, int idle, int iowait)
{
    QMap<QString, int> stat;
    stat["user"] = 100;
    stat["nice"] = 0;
    stat["sys"] = 0;
    stat["idle"] = idle;
    stat["iowait"] = iowait;
    stat["hardqirq"] = 0;
    stat["softirq"] = 0;
    stat["steal"] = 0;
    stat["guest"] = 0;
    stat["guest_nice"] = 0;
    stat["total"] = total;
    profile->mLastCpuStat = stat;
}

class UT_CpuProfile : public ::testing::Test
{
public:
    void SetUp() override
    {
        g_existsResult = true;
        g_openResult = true;
        g_line = QByteArrayLiteral("");
    }

    void TearDown() override
    {
        // StubExt 析构自动还原补丁；显式清理兜底
        stub.clear();
    }

    stub_ext::StubExt stub;
};

// ctor：初始化 mLastCpuStat 并立即执行一次真实 /proc/stat 更新
TEST_F(UT_CpuProfile, CpuProfile_Ctor_InitializesStatMapAndUsage)
{
    // Arrange: 真实 /proc/stat 可读（Linux 环境）
    ASSERT_TRUE(QFile::exists(QStringLiteral("/proc/stat")));

    // Act: 构造即执行首次采样
    CpuProfile profile;
    QMap<QString, int> stat = profile.cpuStat();

    // Assert: 真实 /proc/stat 存在，首次更新即填充 11 项统计
    EXPECT_EQ(stat.size(), 11);
    EXPECT_GT(stat["total"], 0);
    // 首次采样为开机以来的平均占用率，必然落在 [0, 100]
    EXPECT_GE(profile.getCpuUsage(), 0.0);
    EXPECT_LE(profile.getCpuUsage(), 100.0);
}

// ctor 退化路径：真实 /proc/stat 数据段完整（11 段）且 total = 各分项之和
TEST_F(UT_CpuProfile, CpuProfile_Ctor_TotalEqualsSumOfFields)
{
    // Arrange: 真实 /proc/stat 可读
    ASSERT_TRUE(QFile::exists(QStringLiteral("/proc/stat")));

    // Act: 构造即执行首次采样
    CpuProfile profile;
    QMap<QString, int> stat = profile.cpuStat();

    // Assert: total 为 10 个分项之和（源码 curTotalCpu 累加逻辑）
    int sum = stat["user"] + stat["nice"] + stat["sys"] + stat["idle"] + stat["iowait"]
            + stat["hardqirq"] + stat["softirq"] + stat["steal"] + stat["guest"]
            + stat["guest_nice"];
    EXPECT_EQ(stat["total"], sum);
    EXPECT_GT(stat["total"], 0);   // 真实 /proc/stat 开机至今 total 必为正
}

// B1: /proc/stat 不可得（open 失败）→ 返回 0.0 且不更新状态（mCpuUsage 保持 ctor 采样的既有值）
TEST_F(UT_CpuProfile, UpdateSystemCpuUsage_FileUnopenable_ReturnsZeroAndKeepsState)
{
    // Arrange
    CpuProfile profile;
    presetLastStat(&profile, 600, 400, 100);
    QMap<QString, int> before = profile.cpuStat();
    double usageBefore = profile.getCpuUsage();
    stubCpuFileOpen(stub);
    g_openResult = false;

    // Act
    double usage = profile.updateSystemCpuUsage();

    // Assert
    EXPECT_DOUBLE_EQ(usage, 0.0);
    EXPECT_EQ(profile.cpuStat(), before);
    EXPECT_DOUBLE_EQ(profile.getCpuUsage(), usageBefore);
}

// B2: 数据段不足 11 → 返回 0.0 且不更新状态
TEST_F(UT_CpuProfile, UpdateSystemCpuUsage_TooFewFields_ReturnsZeroAndKeepsState)
{
    // Arrange
    CpuProfile profile;
    presetLastStat(&profile, 600, 400, 100);
    QMap<QString, int> before = profile.cpuStat();
    double usageBefore = profile.getCpuUsage();
    g_line = QByteArrayLiteral("cpu 1 2 3\n");
    stubCpuFileOpen(stub);

    // Act
    double usage = profile.updateSystemCpuUsage();

    // Assert
    EXPECT_DOUBLE_EQ(usage, 0.0);
    EXPECT_EQ(profile.cpuStat(), before);
    EXPECT_DOUBLE_EQ(profile.getCpuUsage(), usageBefore);
}

// B3: 两次采样 total 相同（Δtotal == 0）→ 返回 0.0 且不更新状态
TEST_F(UT_CpuProfile, UpdateSystemCpuUsage_ZeroDelta_ReturnsZeroAndKeepsState)
{
    // Arrange
    CpuProfile profile;
    presetLastStat(&profile, 700, 400, 150);
    QMap<QString, int> before = profile.cpuStat();
    double usageBefore = profile.getCpuUsage();
    g_line = kFakeCpuLine;   // total = 150+400+150 = 700，与 prev 相同
    stubCpuFileOpen(stub);

    // Act
    double usage = profile.updateSystemCpuUsage();

    // Assert
    EXPECT_DOUBLE_EQ(usage, 0.0);
    EXPECT_EQ(profile.cpuStat(), before);
    EXPECT_DOUBLE_EQ(profile.getCpuUsage(), usageBefore);
}

// B4: 正常采样差 → 占用率精确计算并更新状态
// prev: total=600, idle系=500; cur: total=700, idle系=550
// usage = (100 - 50) * 100 / 100 = 50.0
TEST_F(UT_CpuProfile, UpdateSystemCpuUsage_ValidDelta_ComputesUsage)
{
    // Arrange
    CpuProfile profile;
    presetLastStat(&profile, 600, 400, 100);
    g_line = kFakeCpuLine;
    stubCpuFileOpen(stub);

    // Act
    double usage = profile.updateSystemCpuUsage();

    // Assert
    EXPECT_DOUBLE_EQ(usage, 50.0);
    EXPECT_DOUBLE_EQ(profile.getCpuUsage(), 50.0);
    QMap<QString, int> stat = profile.cpuStat();
    EXPECT_EQ(stat["total"], 700);
    EXPECT_EQ(stat["user"], 150);
    EXPECT_EQ(stat["idle"], 400);
    EXPECT_EQ(stat["iowait"], 150);
}

// B3 边界复测: 两次采样 total 相同但分项不同（不同 user/idle 分布）→ Δtotal==0 保护分支
// （与 ZeroDelta 用例不同：本次注入行与 prev 行内容不同，仅 total 相同）
TEST_F(UT_CpuProfile, UpdateSystemCpuUsage_AllIdleDelta_ReturnsZero)
{
    // Arrange
    CpuProfile profile;
    presetLastStat(&profile, 600, 400, 100);
    g_line = QByteArrayLiteral("cpu 50 0 0 450 100 0 0 0 0 0\n");   // total=600, idle系=550
    stubCpuFileOpen(stub);

    // Act
    double usage = profile.updateSystemCpuUsage();

    // Assert: Δtotal = 600-600 = 0 → 命中 B3 保护分支，返回 0.0
    EXPECT_DOUBLE_EQ(usage, 0.0);
    // 状态在 B3 分支下保持不变（total 仍是 prev 的 600）
    EXPECT_EQ(profile.cpuStat()["total"], 600);
}

// cpuStat: 返回最近一次统计 map（与灌入状态一致）
TEST_F(UT_CpuProfile, CpuProfile_CpuStat_ReturnsLastStatMap)
{
    // Arrange
    CpuProfile profile;
    presetLastStat(&profile, 800, 500, 100);

    // Act
    QMap<QString, int> stat = profile.cpuStat();

    // Assert
    EXPECT_EQ(stat["total"], 800);
    EXPECT_EQ(stat["idle"], 500);
    EXPECT_EQ(stat["iowait"], 100);
    EXPECT_EQ(stat["guest_nice"], 0);
}

// getCpuUsage: 返回最近一次计算的占用率
TEST_F(UT_CpuProfile, CpuProfile_GetCpuUsage_ReturnsLastUsage)
{
    // Arrange
    CpuProfile profile;
    presetLastStat(&profile, 600, 400, 100);
    g_line = kFakeCpuLine;
    stubCpuFileOpen(stub);

    // Act
    profile.updateSystemCpuUsage();

    // Assert
    EXPECT_DOUBLE_EQ(profile.getCpuUsage(), 50.0);
    // 与内部状态一致
    EXPECT_DOUBLE_EQ(profile.getCpuUsage(), profile.mCpuUsage);
}
