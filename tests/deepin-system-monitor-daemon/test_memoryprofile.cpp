// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 MemoryProfile 单元测试
// 被测类：deepin-system-monitor-daemon/src/memoryprofile.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method                    | level | factors             | min | actual |
// |---------------------------|-------|---------------------|-----|--------|
// | MemoryProfile (ctor)      | low   | -                   | 1   | 1      |
// | updateSystemMemoryUsage   | mid   | lines:70            | 2   | 6      |
// | getMemUsage               | low   | -                   | 1   | 1      |
//
// 分支清单（源自真实源码 worktree memoryprofile.cpp，self-check §2c）：
// - updateSystemMemoryUsage:
//   B1 file.exists()&&open(ReadOnly) 为假 → qCWarning + 返回局部 0.0，mMemUsage 不变
//      （exists 声明为 out-of-line 但补丁拦截不可靠，与 open 失败同属一个分支，
//       统一用 open 失败覆盖）
//                                   → UpdateSystemMemoryUsage_FileUnopenable_ReturnsZeroAndKeepsState
//   B2 三行任一为空（size()==0）→ qCWarning + 返回 0.0
//                                   → UpdateSystemMemoryUsage_EmptyLine_ReturnsZero
//   B3 三行任一分段数 < 3 → qCWarning + 返回 0.0
//                                   → UpdateSystemMemoryUsage_TooFewFields_ReturnsZero
//   B4 含 MemTotal: 且含 MemAvailable: 且 MemTotal != 0
//      → memUsage=(MemTotal-MemAvailable)*100/MemTotal，mMemUsage 更新
//                                   → UpdateSystemMemoryUsage_ValidData_ComputesUsage
//   B5 MemTotal == 0（条件为假，else 告警分支）→ 返回 0.0，mMemUsage 不变
//                                   → UpdateSystemMemoryUsage_ZeroTotalMem_KeepsState
//   B5 缺 MemAvailable: 键（else 告警分支）→ 返回 0.0，mMemUsage 不变
//                                   → UpdateSystemMemoryUsage_MissingAvailKey_KeepsState
// - ctor: mMemUsage 初始化为 0    → MemoryProfile_Ctor_StartsAtZero
// - getMemUsage: 返回 mMemUsage   → MemoryProfile_GetMemUsage_ReturnsLastUsage
//
// stub 决策（dependency-tracer §3）：
// - /proc/meminfo 路径硬编码（宏 PROC_MEM_INFOI_PATH）→ stub QFile::exists/open +
//   QIODevice::readLine(qint64) 按序注入 3 行受控数据；Qt 类成员补丁沿用仓库既有
//   测试的 stub.set + 静态函数模式（set_lamda 对 QtCore 成员存在还原缺陷，实测崩溃）
// - 本项目代码不 stub；不触网、不启子进程

#include "memoryprofile.h"

#include "stubext.h"

#include <QFile>
#include <QIODevice>
#include <gtest/gtest.h>

/***************************************STUB begin*********************************************/
typedef bool (*QFileExistsPtr)(QFile *);
typedef bool (*QFileOpenPtr)(QFile *, QIODevice::OpenMode);
typedef QByteArray (*QIODeviceReadLinePtr)(QIODevice *, qint64);

static bool g_existsResult = true;
static bool g_openResult = true;
static QList<QByteArray> g_memLines;
static int g_memLineIdx = 0;

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
    if (g_memLineIdx < g_memLines.size())
        return g_memLines.at(g_memLineIdx++);
    return QByteArray();
}

// stub /proc/meminfo 打开成功并按序返回受控行
static void stubMemFileOpen(stub_ext::StubExt &stub)
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

class UT_MemoryProfile : public ::testing::Test
{
public:
    void SetUp() override
    {
        g_existsResult = true;
        g_openResult = true;
        g_memLines.clear();
        g_memLineIdx = 0;
    }

    void TearDown() override
    {
        stub.clear();
    }

    stub_ext::StubExt stub;
};

static void feedMemLines(const char *l1, const char *l2, const char *l3)
{
    g_memLines = { QByteArray(l1), QByteArray(l2), QByteArray(l3) };
}

// ctor: mMemUsage 初始为 0
TEST_F(UT_MemoryProfile, MemoryProfile_Ctor_StartsAtZero)
{
    // Arrange: 真实 /proc/meminfo 可读（ctor 不采样，仅初始化）
    ASSERT_TRUE(QFile::exists(QStringLiteral("/proc/meminfo")));

    // Act: 构造后读取初始占用率
    MemoryProfile profile;
    double initial = profile.getMemUsage();

    // Assert: ctor 不采样，mMemUsage 从 0 开始
    EXPECT_DOUBLE_EQ(initial, 0.0);
    EXPECT_DOUBLE_EQ(profile.mMemUsage, 0.0);
}

// B1: /proc/meminfo 不可得（open 失败）→ 返回 0.0 且 mMemUsage 不变
TEST_F(UT_MemoryProfile, UpdateSystemMemoryUsage_FileUnopenable_ReturnsZeroAndKeepsState)
{
    // Arrange
    MemoryProfile profile;
    stubMemFileOpen(stub);
    g_openResult = false;

    // Act
    double usage = profile.updateSystemMemoryUsage();

    // Assert
    EXPECT_DOUBLE_EQ(usage, 0.0);
    EXPECT_DOUBLE_EQ(profile.getMemUsage(), 0.0);
}

// B2: 第三行为空（EOF）→ 返回 0.0 且 mMemUsage 不变
TEST_F(UT_MemoryProfile, UpdateSystemMemoryUsage_EmptyLine_ReturnsZero)
{
    // Arrange
    MemoryProfile profile;
    feedMemLines("MemTotal:       1000 kB\n", "MemFree:         400 kB\n", "");
    stubMemFileOpen(stub);

    // Act
    double usage = profile.updateSystemMemoryUsage();

    // Assert
    EXPECT_DOUBLE_EQ(usage, 0.0);
    EXPECT_DOUBLE_EQ(profile.getMemUsage(), 0.0);
}

// B3: 首行仅 2 个分段（MemTotal: 100）→ 返回 0.0 且 mMemUsage 不变
TEST_F(UT_MemoryProfile, UpdateSystemMemoryUsage_TooFewFields_ReturnsZero)
{
    // Arrange
    MemoryProfile profile;
    feedMemLines("MemTotal: 100\n", "MemFree:         400 kB\n", "MemAvailable:    200 kB\n");
    stubMemFileOpen(stub);

    // Act
    double usage = profile.updateSystemMemoryUsage();

    // Assert
    EXPECT_DOUBLE_EQ(usage, 0.0);
    EXPECT_DOUBLE_EQ(profile.getMemUsage(), 0.0);
}

// B4: 正常数据 → (MemTotal - MemAvailable) * 100 / MemTotal，状态更新
// 1000 / 400 / 200 → (1000-200)*100/1000 = 80.0
TEST_F(UT_MemoryProfile, UpdateSystemMemoryUsage_ValidData_ComputesUsage)
{
    // Arrange
    MemoryProfile profile;
    feedMemLines("MemTotal:       1000 kB\n", "MemFree:         400 kB\n", "MemAvailable:    200 kB\n");
    stubMemFileOpen(stub);

    // Act
    double usage = profile.updateSystemMemoryUsage();

    // Assert
    EXPECT_DOUBLE_EQ(usage, 80.0);
    EXPECT_DOUBLE_EQ(profile.getMemUsage(), 80.0);
}

// B5: MemTotal == 0 → 条件为假走 else 告警分支，mMemUsage 保持上次有效值
TEST_F(UT_MemoryProfile, UpdateSystemMemoryUsage_ZeroTotalMem_KeepsState)
{
    // Arrange
    MemoryProfile profile;
    feedMemLines("MemTotal:       1000 kB\n", "MemFree:         400 kB\n", "MemAvailable:    200 kB\n");
    stubMemFileOpen(stub);
    profile.updateSystemMemoryUsage();   // mMemUsage = 80.0

    // Act: 注入 MemTotal 为 0 的数据
    g_memLineIdx = 0;
    feedMemLines("MemTotal:          0 kB\n", "MemFree:         400 kB\n", "MemAvailable:    200 kB\n");
    double usage = profile.updateSystemMemoryUsage();

    // Assert: 本次返回局部 0.0，但 mMemUsage 保持 80.0
    EXPECT_DOUBLE_EQ(usage, 0.0);
    EXPECT_DOUBLE_EQ(profile.getMemUsage(), 80.0);
}

// B5: 缺 MemAvailable: 键（第三行为 MemCached）→ else 告警分支，mMemUsage 不变
TEST_F(UT_MemoryProfile, UpdateSystemMemoryUsage_MissingAvailKey_KeepsState)
{
    // Arrange
    MemoryProfile profile;
    feedMemLines("MemTotal:       1000 kB\n", "MemFree:         400 kB\n", "MemCached:      300 kB\n");
    stubMemFileOpen(stub);

    // Act
    double usage = profile.updateSystemMemoryUsage();

    // Assert
    EXPECT_DOUBLE_EQ(usage, 0.0);
    EXPECT_DOUBLE_EQ(profile.getMemUsage(), 0.0);
}

// getMemUsage: 返回最近一次成功计算的占用率
TEST_F(UT_MemoryProfile, MemoryProfile_GetMemUsage_ReturnsLastUsage)
{
    // Arrange
    MemoryProfile profile;
    feedMemLines("MemTotal:       2000 kB\n", "MemFree:        1000 kB\n", "MemAvailable:    500 kB\n");
    stubMemFileOpen(stub);

    // Act
    profile.updateSystemMemoryUsage();

    // Assert: (2000-500)*100/2000 = 75.0
    EXPECT_DOUBLE_EQ(profile.getMemUsage(), 75.0);
    // 与内部状态一致
    EXPECT_DOUBLE_EQ(profile.getMemUsage(), profile.mMemUsage);
}
