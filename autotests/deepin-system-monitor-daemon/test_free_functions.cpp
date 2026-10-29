// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的自由函数单元测试
// 被测函数组：
//   - deepin-system-monitor-daemon/src/plugin.cpp：DSMRegister / DSMUnRegister
//     （daemon 为 deepin-service-manager MODULE 插件，二者为 extern "C" 入口，
//      头文件中无声明，测试内自行 extern 声明）
//   - deepin-system-monitor-daemon/src/systemmonitorservice.cpp：getNameByPid
//
// 用例计数声明（self-check-structural 验证此块）：
// | function        | level | factors              | min | actual |
// |-----------------|-------|----------------------|-----|--------|
// | DSMRegister     | mid   | 正常注册/重复注册    | 2   | 2      |
// | DSMUnRegister   | mid   | 注册后注销/循环注销  | 2   | 2      |
// | getNameByPid    | mid   | 自身pid/无效pid      | 2   | 2      |
//
// 分支清单（源自真实源码，self-check §2c）：
// - DSMRegister(name, data): new SystemMonitorService(name) 存入全局 monitorService
//   → 返回 0（data 丢弃）
//     → DSMRegister_ValidName_ReturnsZero
//     → DSMRegister_Repeated_OverwritesGlobal（现状锁定缺陷 D8）
// - DSMUnRegister(name, data): monitorService->deleteLater() + 置空 → 返回 0
//     → DSMUnRegister_AfterRegister_ReturnsZero
//     → DSMUnRegister_AfterReregister_ReturnsZero（注销后可再次注册并注销，全局复位）
//   （未注册时调用 → 空指针解引用崩溃 —— 缺陷 D9 不可写通过型用例，仅记录）
// - getNameByPid(pid): QFile /proc/<pid>/cmdline 打开成功 → 返回首行
//     → GetNameByPid_SelfPid_ReturnsCmdlineFirstLine
//   打开失败（pid<=0 / 不存在）→ 返回空串
//     → GetNameByPid_InvalidPid_ReturnsEmpty
//
// 已知源码缺陷（仅标红不修复，详见 defects 报告）：
//   D8 DSMRegister 重复调用：旧 monitorService 直接覆盖，前一实例泄漏（无 delete）
//   D9 DSMUnRegister 无空指针保护：monitorService 为空（未注册/已注销）时
//      monitorService->deleteLater() 解引用崩溃（测试不构造该路径）
//
// stub 决策（dependency-tracer §3）：
// - /proc/<pid>/cmdline 为真实文件读取（getNameByPid 用自身进程 pid，环境可控）
// - DSMRegister/DSMUnRegister 内部构造 SystemMonitorService → 复用服务测试的
//   配置隔离手段：QStandardPaths::writableLocation → QTemporaryDir
// - 不触网、不启子进程

#include "systemmonitorservice.h"

#include "stubext.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <gtest/gtest.h>
#include <unistd.h>

// daemon 插件 extern "C" 入口（src/plugin.cpp，无头文件声明）
extern "C" int DSMRegister(const char *name, void *data);
extern "C" int DSMUnRegister(const char *name, void *data);

// 自由函数（src/systemmonitorservice.cpp，头文件无声明）
QString getNameByPid(pid_t pid);

static QString g_configRoot;

class UT_FreePlugin : public ::testing::Test
{
public:
    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());
        g_configRoot = m_tempDir.path();
        stub.set_lamda(static_cast<QString (*)(QStandardPaths::StandardLocation)>(
                           &QStandardPaths::writableLocation),
                       [](QStandardPaths::StandardLocation) -> QString {
                           return g_configRoot;
                       });
    }

    void TearDown() override
    {
        stub.clear();
        g_configRoot.clear();
    }

    stub_ext::StubExt stub;
    QTemporaryDir m_tempDir;
};

class UT_FreeGetNameByPid : public ::testing::Test
{
public:
    void TearDown() override
    {
        stub.clear();
    }

    stub_ext::StubExt stub;
};

// DSMRegister: 正常注册返回 0（内部构造服务实例）
TEST_F(UT_FreePlugin, DSMRegister_ValidName_ReturnsZero)
{
    // Arrange: fixture 初始全局未注册（monitorService 为空），配置目录已隔离
    ASSERT_FALSE(g_configRoot.isEmpty());

    // Act
    int ret = DSMRegister("ut.dsm.free.register", nullptr);

    // Assert
    EXPECT_EQ(ret, 0);

    // 清理: 注销避免状态残留
    EXPECT_EQ(DSMUnRegister(nullptr, nullptr), 0);
}

// DSMRegister: 重复注册覆盖旧实例并再次返回 0（现状锁定缺陷 D8）
TEST_F(UT_FreePlugin, DSMRegister_Repeated_OverwritesGlobal)
{
    // Arrange: 配置目录已隔离，先注册一次使全局已有实例
    ASSERT_FALSE(g_configRoot.isEmpty());

    // Act
    EXPECT_EQ(DSMRegister("ut.dsm.free.first", nullptr), 0);
    int ret = DSMRegister("ut.dsm.free.second", nullptr);

    // Assert: 重复注册返回 0（旧实例泄漏 —— D8 仅标红）
    EXPECT_EQ(ret, 0);

    // 清理
    EXPECT_EQ(DSMUnRegister(nullptr, nullptr), 0);
}

// DSMUnRegister: 注册后注销返回 0，且全局状态复位后可再次注册
TEST_F(UT_FreePlugin, DSMUnRegister_AfterRegister_ReturnsZero)
{
    // Arrange
    ASSERT_EQ(DSMRegister("ut.dsm.free.unregister", nullptr), 0);

    // Act
    int ret = DSMUnRegister(nullptr, nullptr);

    // Assert
    EXPECT_EQ(ret, 0);

    // Assert: 注销后再次注册仍可用（全局指针已置空）
    EXPECT_EQ(DSMRegister("ut.dsm.free.again", nullptr), 0);
    EXPECT_EQ(DSMUnRegister(nullptr, nullptr), 0);
}

// DSMUnRegister: 注销后重新注册、再次注销 → 全局复位路径可循环使用
TEST_F(UT_FreePlugin, DSMUnRegister_AfterReregister_ReturnsZero)
{
    // Arrange: 注册并注销一次（全局指针已置空）
    ASSERT_EQ(DSMRegister("ut.dsm.free.cycle", nullptr), 0);
    ASSERT_EQ(DSMUnRegister(nullptr, nullptr), 0);

    // Act: 重新注册后注销
    ASSERT_EQ(DSMRegister("ut.dsm.free.cycle2", nullptr), 0);
    int ret = DSMUnRegister(nullptr, nullptr);

    // Assert
    EXPECT_EQ(ret, 0);

    // 清理: 第三轮循环注册后注销（全局复位路径可反复使用）
    EXPECT_EQ(DSMRegister("ut.dsm.free.cycle3", nullptr), 0);
    EXPECT_EQ(DSMUnRegister(nullptr, nullptr), 0);
}

// getNameByPid: 自身进程 pid → /proc/<pid>/cmdline 首段（readLine 读到 EOF，
// 含末尾 NUL 分隔符；断言以 argv[0] 开头）
TEST_F(UT_FreeGetNameByPid, GetNameByPid_SelfPid_ReturnsCmdlineFirstLine)
{
    // Arrange
    pid_t selfPid = getpid();

    // Act
    QString name = getNameByPid(selfPid);

    // Assert: 与进程启动参数 argv[0] 一致（cmdline 无换行符，readLine 返回整段含尾部 \0）
    QString expected = QCoreApplication::arguments().value(0);
    EXPECT_FALSE(name.isEmpty());
    EXPECT_EQ(name.startsWith(expected), true);
}

// getNameByPid: 无效 pid → 文件打开失败 → 返回空串
TEST_F(UT_FreeGetNameByPid, GetNameByPid_InvalidPid_ReturnsEmpty)
{
    // Arrange: pid 0 与负值均无对应 /proc/<pid>/cmdline 文件
    const pid_t badPidZero = 0;
    const pid_t badPidNegative = -1;

    // Act: 分别查询两个无效 pid
    QString nameZero = getNameByPid(badPidZero);
    QString nameNegative = getNameByPid(badPidNegative);

    // Assert: 均无法打开 /proc 文件 → 空串
    EXPECT_EQ(nameZero, QString());
    EXPECT_EQ(nameNegative, QString());
}
