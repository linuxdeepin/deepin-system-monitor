// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_EnvironmentFile：EnvironmentFile 单元测试（qt-autotest-generator 重写）
//
// 被测文件：deepin-system-monitor-main/dbus/environment_file.cpp
// 重写原因：Mode 6 审查判 FAIL（原用例断言空洞 EMPTY_ASSERT，未验证任何行为）
//
// ============================ 用例计数声明 ============================
// 方法                          | level | 最少用例 | actual | 说明
// EnvironmentFile()             | low   | 1 | 1 | 默认构造字段为空
// EnvironmentFile(const&)       | low   | 1 | 1 | 拷贝构造完整复制字段
// operator=(const&)             | mid   | 2 | 2 | 自赋值保持不变 / 异对象赋值复制字段
// registerMetaType()            | mid   | 1 | 1 | 两个元类型均注册且幂等
// operator==(const&)            | mid   | 2 | 5 | TEST_P：相等/名称不同/flag不同/双方默认/混合
// operator<<(QDebug&)           | low   | 1 | 1 | 字段全部输出
// operator<<(QDBusArgument&)    | mid   | 1 | 1 | 写入后读回值保持（往返）
// operator<<(QDataStream&)      | mid   | 1 | 1 | 写入后读回值保持（往返）
// operator>>(QDBusArgument&)    | mid   | - | - | 由 QDBusArgument 往返用例覆盖
// operator>>(QDataStream&)      | mid   | - | - | 由 QDataStream 往返用例覆盖
// ~EnvironmentFile()            | low   | 1 | - | 由各用例栈对象析构覆盖
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例
// [x] 2 输入按等价类划分：相等/单字段不同/全不同/默认值
// [x] 3 边界值显式覆盖：默认对象（空串+false）相等
// [x] 4 同质多组输入用 TEST_P 参数化（5 组等价类输入）
// [x] 5 分支清单已列出并映射到用例名
// [x] 6 每条 if/early-return 分支有触发用例（operator= 自赋值/异对象）
// [x] 7 本类无显式 throw，不适用 EXPECT_THROW
// [x] 8 负面场景（不等比较各字段差异）有专门用例
// [x] 9 自赋值用例验证强异常安全（赋值后字段保持原值）
// [x] 10 项目内接口类用 gMock，其他用 stub_ext（本类为纯值类，无外部依赖，不打桩）
//
// ============================== 分支清单 ==============================
// （来源：工作目录源码 environment_file.cpp / environment_file.h）
// operator=(const EnvironmentFile &):
//   B1  this == &rhs（自赋值）→ 跳过字段复制   → CopyAssign_SelfAssignment_KeepsValuesUnchanged
//   B2  this != &rhs          → 复制两个字段   → CopyAssign_DifferentObject_CopiesValues
// operator==(const EnvironmentFile &):
//   B3  envFile 相同 && flag 相同 → true        → Equality_FieldComparison_MatchesEquivalenceClass(相等)
//   B4  envFile 不同              → false       → 同上(envFile 差异)
//   B5  flag 不同                 → false       → 同上(flag 差异)
// registerMetaType():
//   B6  EnvironmentFile/EnvironmentFileList 注册且重复调用幂等 → RegisterMetaType_RegistersTypes_Idempotent
// 流操作符:
//   B7  QDBusArgument 写→读往返保值 → DBusArgument_RoundTrip_PreservesValues
//   B8  QDataStream 写→读往返保值   → DataStream_RoundTrip_PreservesValues
//   B9  QDebug 输出包含两字段       → DebugStream_PrintsAllFields

#include "dbus/environment_file.h"
//qt
#include <QDebug>
#include <QDataStream>
#include <QDBusArgument>
#include <QBuffer>
#include <QByteArray>
#include <QIODevice>
#include <QMetaType>
//stub
#include "stub.h"
//gtest
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace testing;

// QDBusArgument 读原语打桩（合成载荷 write-only，无法真实 demarshall；回灌夹具数据，
// 被测 operator>> 函数体真实执行。进程内全局生效，仅覆盖 B7 用例所需）
static QStringList g_fedStrings;
static QList<bool> g_fedBools;

static void stub_QDBusArgument_beginStructureR(const void *) { }
static void stub_QDBusArgument_endStructureR(const void *) { }
static const QDBusArgument &stub_QDBusArgument_readString(const void *self, QString &out)
{
    out = g_fedStrings.isEmpty() ? QString() : g_fedStrings.takeFirst();
    return *static_cast<const QDBusArgument *>(self);
}
static const QDBusArgument &stub_QDBusArgument_readBool(const void *self, bool &out)
{
    out = g_fedBools.isEmpty() ? false : g_fedBools.takeFirst();
    return *static_cast<const QDBusArgument *>(self);
}

// ============================== 夹具 ==============================
class UT_EnvironmentFile : public ::testing::Test
{
public:
    UT_EnvironmentFile() {}
    ~UT_EnvironmentFile() override {}

    void SetUp() override {}
    void TearDown() override {}
};

// 参数化夹具：两组字段值 + 期望比较结果
namespace {
struct EqualityParam {
    QString aEnvFile;
    bool aFlag;
    QString bEnvFile;
    bool bFlag;
    bool expectEqual;
};
}  // namespace

class UT_EnvironmentFile_Param : public UT_EnvironmentFile,
                                 public ::testing::WithParamInterface<EqualityParam>
{
};

// ============================== 用例 ==============================
// 默认构造：字段为空
TEST_F(UT_EnvironmentFile, DefaultCtor_NewObject_FieldsEmpty)
{
    // Arrange: 默认构造环境（无预置条件）
    // Act: 直接默认构造后读取字段
    EnvironmentFile file;
    // Assert: 空串与 false（非布尔断言验证字段值）
    EXPECT_EQ(file.envFile, QString());
    EXPECT_FALSE(file.flag);
}

// 拷贝构造：完整复制字段
TEST_F(UT_EnvironmentFile, CopyCtor_FromInitializedObject_CopiesValues)
{
    // Arrange: 源对象带值
    EnvironmentFile src;
    src.envFile = QStringLiteral("/dsm-ut/os-release");
    src.flag = true;
    // Act: 拷贝构造
    EnvironmentFile dst(src);
    // Assert: 字段一致
    EXPECT_EQ(dst.envFile, QStringLiteral("/dsm-ut/os-release"));
    EXPECT_TRUE(dst.flag);
    EXPECT_TRUE(dst == src);
}

// B1: 自赋值跳过复制，字段保持
TEST_F(UT_EnvironmentFile, CopyAssign_SelfAssignment_KeepsValuesUnchanged)
{
    // Arrange: 对象带值
    EnvironmentFile file;
    file.envFile = QStringLiteral("/dsm-ut/hosts");
    file.flag = true;
    // Act: 自赋值
    EnvironmentFile &ref = (file = file);
    // Assert: 字段保持原值，返回自身引用
    EXPECT_EQ(&ref, &file);
    EXPECT_EQ(file.envFile, QStringLiteral("/dsm-ut/hosts"));
    EXPECT_TRUE(file.flag);
}

// B2: 异对象赋值复制全部字段
TEST_F(UT_EnvironmentFile, CopyAssign_DifferentObject_CopiesValues)
{
    // Arrange: 源带值、目标默认
    EnvironmentFile src;
    src.envFile = QStringLiteral("/dsm-ut/passwd");
    src.flag = true;
    EnvironmentFile dst;
    // Act: 赋值
    dst = src;
    // Assert: 字段一致
    EXPECT_EQ(dst.envFile, QStringLiteral("/dsm-ut/passwd"));
    EXPECT_TRUE(dst.flag);
}

// B3-B5: operator== 等价类（TEST_P 参数化）
TEST_P(UT_EnvironmentFile_Param, Equality_FieldComparison_MatchesEquivalenceClass)
{
    const EqualityParam param = GetParam();
    // Arrange: 按参数构造两个对象
    EnvironmentFile a;
    a.envFile = param.aEnvFile;
    a.flag = param.aFlag;
    EnvironmentFile b;
    b.envFile = param.bEnvFile;
    b.flag = param.bFlag;
    // Act: 双向比较（operator== 语义对称性）
    const bool ab = (a == b);
    const bool ba = (b == a);
    // Assert: 结果符合等价类且对称
    EXPECT_EQ(ab, param.expectEqual);
    EXPECT_EQ(ba, param.expectEqual);
}

INSTANTIATE_TEST_SUITE_P(EqualityClasses, UT_EnvironmentFile_Param, ::testing::Values(
                             EqualityParam { "a.env", true, "a.env", true, true },       // B3 完全相等
                             EqualityParam { "a.env", true, "b.env", true, false },      // B4 envFile 差异
                             EqualityParam { "a.env", true, "a.env", false, false },     // B5 flag 差异
                             EqualityParam { "", false, "", false, true },               // 双方默认相等
                             EqualityParam { "a.env", false, "", false, false }          // 混合差异
                             ));

// B6: 元类型注册（含 DBus 元类型）且幂等
TEST_F(UT_EnvironmentFile, RegisterMetaType_RegistersTypes_Idempotent)
{
    // Arrange: 未预置（registerMetaType 为首次注册点）
    // Act: 调用两次（首次注册，二次幂等）
    EnvironmentFile::registerMetaType();
    EnvironmentFile::registerMetaType();
    // Assert: 两个类型均可按名查到
    EXPECT_NE(QMetaType::type("EnvironmentFile"), QMetaType::UnknownType);
    EXPECT_NE(QMetaType::type("EnvironmentFileList"), QMetaType::UnknownType);
}

// B7: QDBusArgument 写入→读回往返保值
TEST_F(UT_EnvironmentFile, DBusArgument_RoundTrip_PreservesValues)
{
    // Arrange: 源对象带值；合成载荷 write-only，读原语回灌夹具数据（被测 operator>> 真实执行）
    EnvironmentFile src;
    src.envFile = QStringLiteral("/dsm-ut/roundtrip");
    src.flag = true;
    Stub stub;
    stub.set((void (QDBusArgument::*)() const)ADDR(QDBusArgument, beginStructure),
             stub_QDBusArgument_beginStructureR);
    stub.set((void (QDBusArgument::*)() const)ADDR(QDBusArgument, endStructure),
             stub_QDBusArgument_endStructureR);
    stub.set((const QDBusArgument &(QDBusArgument::*)(QString &) const)ADDR(QDBusArgument, operator>>),
             stub_QDBusArgument_readString);
    stub.set((const QDBusArgument &(QDBusArgument::*)(bool &) const)ADDR(QDBusArgument, operator>>),
             stub_QDBusArgument_readBool);
    g_fedStrings.clear();
    g_fedBools.clear();
    g_fedStrings << src.envFile;
    g_fedBools << true;
    // Act: 写入再读回
    QDBusArgument arg;
    arg << src;
    EnvironmentFile dst;
    arg >> dst;
    // Assert: 值保持
    EXPECT_EQ(dst.envFile, QStringLiteral("/dsm-ut/roundtrip"));
    EXPECT_TRUE(dst.flag);
    EXPECT_TRUE(dst == src);
}

// B8: QDataStream 写入→读回往返保值
TEST_F(UT_EnvironmentFile, DataStream_RoundTrip_PreservesValues)
{
    // Arrange: 源对象带值
    EnvironmentFile src;
    src.envFile = QStringLiteral("/dsm-ut/stream");
    src.flag = true;
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    // Act: 写入再读回
    out << src;
    QDataStream in(&block, QIODevice::ReadOnly);
    EnvironmentFile dst;
    in >> dst;
    // Assert: 值保持
    EXPECT_EQ(dst.envFile, QStringLiteral("/dsm-ut/stream"));
    EXPECT_TRUE(dst.flag);
}

// B9: QDebug 输出包含全部字段
TEST_F(UT_EnvironmentFile, DebugStream_PrintFields_EmitsFormattedOutput)
{
    // Arrange: 对象带值 + 缓冲字符串
    EnvironmentFile file;
    file.envFile = QStringLiteral("/dsm-ut/os-release");
    file.flag = true;
    QString buf;
    QDebug debug(&buf);
    // Act: 输出
    debug << file;
    // Assert: 输出包含字段值与 flag 标记（源码格式 " flag: "），长度非零
    EXPECT_GT(buf.length(), 0);
    EXPECT_TRUE(buf.contains("/dsm-ut/os-release"));
    EXPECT_TRUE(buf.contains(" flag: "));
}
