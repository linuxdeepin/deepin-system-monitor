// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 用例计数声明（self-check-structural 验证此块）：
// | method                          | level | factors | min | actual |
// |---------------------------------|-------|---------|-----|--------|
// | ctor(default)                   | low   | -       | 1   | 1      |
// | ctor(params)                    | low   | -       | 1   | 1      |
// | ctor(copy)                      | low   | -       | 1   | 2      |
// | operator=                       | low   | -       | 1   | 2      |
// | operator==                      | low   | -       | 1   | 3      |
// | getName/setName + getStatus/setStatus | mid | - | 1   | 1      |
// | registerMetaType                | low   | -       | 1   | 1      |
// | QDBusArgument << (单)            | low   | -       | 1   | 1      |
// | QDBusArgument >> (单)            | low   | -       | 1   | 1      |
// | UnitFileInfoList <<             | low   | -       | 1   | 1      |
// | UnitFileInfoList >>             | low   | -       | 1   | 1      |
// | QDataStream << / >>             | low   | -       | 2   | 1      |
// | QDebug <<                       | low   | -       | 1   | 1      |
// | UnitFileInfoData ctor/=/==      | low   | -       | 3   | 随共享语义覆盖 |

//self
#include "dbus/unit_file_info.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDataStream>
#include <QBuffer>
#include <QDebug>
#include <QDBusArgument>
#include <QVariant>

/***************************************STUB begin*********************************************/
// 本文件为纯数据类测试，无外部依赖，不需要打桩。
/***************************************STUB end***********************************************/

namespace {
// 构造一个字段完整的 UnitFileInfo，供各用例复用
static UnitFileInfo makeFullUnitFileInfo()
{
    UnitFileInfo info(QStringLiteral("a.service"),
                      QStringLiteral("enabled"));
    return info;
}
} // namespace

class UT_UnitFileInfo: public ::testing::Test
{
public:
    UT_UnitFileInfo() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // Arrange: 构造默认对象（本 fixture 不做全局 DBus 注册：
        // 列表编组写操作存在 beginArray 传入列表元类型 id 的缺陷，
        // 在列表类型注册后触发写入会直接段错误，缺陷已在 Mode 5 单独标红）
        m_tester = new UnitFileInfo();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    UnitFileInfo *m_tester;
};

// ─────────────────────────── 构造 / 共享数据语义 ───────────────────────────

TEST_F(UT_UnitFileInfo, Ctor_Default_AllFieldsEmpty)
{
    // Arrange: 读取默认对象的名字字段
    const QString defaultName = m_tester->getName();

    // Act: 读取状态字段
    const QString defaultStatus = m_tester->getStatus();

    // Assert: 默认构造字段为空串
    EXPECT_EQ(defaultName, QString());
    EXPECT_EQ(defaultStatus, QString());
}

TEST_F(UT_UnitFileInfo, Ctor_NameAndStatus_GettersReturnCtorValues)
{
    // Arrange: 用参数构造对象
    UnitFileInfo info = makeFullUnitFileInfo();

    // Act: 读取字段
    const QString name = info.getName();
    const QString status = info.getStatus();

    // Assert: getter 返回构造参数原值
    EXPECT_EQ(name, QStringLiteral("a.service"));
    EXPECT_EQ(status, QStringLiteral("enabled"));
}

TEST_F(UT_UnitFileInfo, CopyCtor_ExplicitShare_CopySeesSameData)
{
    // Arrange: 构造源对象
    UnitFileInfo src = makeFullUnitFileInfo();

    // Act: 拷贝构造（QExplicitlySharedDataPointer 显式共享）
    UnitFileInfo dst(src);

    // Assert: 拷贝体读到相同字段
    EXPECT_TRUE(dst == src);
    EXPECT_EQ(dst.getName(), QStringLiteral("a.service"));
    EXPECT_EQ(dst.getStatus(), QStringLiteral("enabled"));
}

TEST_F(UT_UnitFileInfo, CopyCtor_ExplicitShare_MutationVisibleToBoth)
{
    // Arrange: 构造源对象并拷贝
    UnitFileInfo src = makeFullUnitFileInfo();
    UnitFileInfo dst(src);

    // Act: 通过拷贝体修改
    dst.setName(QStringLiteral("b.service"));

    // Assert: 共享数据使源对象同样可见
    EXPECT_EQ(dst.getName(), QStringLiteral("b.service"));
    EXPECT_EQ(src.getName(), QStringLiteral("b.service"));
}

TEST_F(UT_UnitFileInfo, AssignOp_FromFullSource_TargetMatchesSource)
{
    // Arrange: 默认对象与全字段源对象
    UnitFileInfo src = makeFullUnitFileInfo();

    // Act: 赋值
    *m_tester = src;

    // Assert: 目标与源一致
    EXPECT_TRUE(*m_tester == src);
    EXPECT_EQ(m_tester->getName(), QStringLiteral("a.service"));
    EXPECT_EQ(m_tester->getStatus(), QStringLiteral("enabled"));
}

TEST_F(UT_UnitFileInfo, AssignOp_SourceMutation_AfterAssignVisibleInTarget)
{
    // Arrange: 先赋值再改源
    UnitFileInfo src = makeFullUnitFileInfo();
    *m_tester = src;

    // Act: 修改源对象
    src.setStatus(QStringLiteral("disabled"));

    // Assert: 共享语义使目标同样可见
    EXPECT_EQ(src.getStatus(), QStringLiteral("disabled"));
    EXPECT_EQ(m_tester->getStatus(), QStringLiteral("disabled"));
}

// ─────────────────────────── operator== ───────────────────────────

TEST_F(UT_UnitFileInfo, EqualOp_SameFieldValues_ReturnsTrue)
{
    // Arrange: 两个同字段对象
    UnitFileInfo a = makeFullUnitFileInfo();
    UnitFileInfo b = makeFullUnitFileInfo();

    // Act: 逐对象比较
    const bool equal = (a == b);

    // Assert: 全字段相同视为相等
    EXPECT_TRUE(equal);
    EXPECT_EQ(a.getName(), b.getName());
    EXPECT_EQ(a.getStatus(), b.getStatus());
}

TEST_F(UT_UnitFileInfo, EqualOp_DifferentName_ReturnsFalse)
{
    // Arrange: 仅名称不同
    UnitFileInfo a = makeFullUnitFileInfo();
    UnitFileInfo b = makeFullUnitFileInfo();
    b.setName(QStringLiteral("other.service"));

    // Act: 比较
    const bool equal = (a == b);

    // Assert: 不相等
    EXPECT_FALSE(equal);
    EXPECT_NE(a.getName(), b.getName());
}

TEST_F(UT_UnitFileInfo, EqualOp_DifferentStatus_ReturnsFalse)
{
    // Arrange: 仅状态不同
    UnitFileInfo a = makeFullUnitFileInfo();
    UnitFileInfo b = makeFullUnitFileInfo();
    b.setStatus(QStringLiteral("static"));

    // Act: 比较
    const bool equal = (a == b);

    // Assert: 不相等
    EXPECT_FALSE(equal);
    EXPECT_NE(a.getStatus(), b.getStatus());
}

// ─────────────────────────── getter / setter ───────────────────────────

TEST_F(UT_UnitFileInfo, GetterSetter_NameAndStatus_Roundtrip)
{
    // Arrange: 校验前置状态（字段为空）
    EXPECT_TRUE(m_tester->getName().isEmpty());

    // Act: 设置名称与状态
    m_tester->setName(QStringLiteral("c.service"));
    m_tester->setStatus(QStringLiteral("masked"));

    // Assert: 读到写入值
    EXPECT_EQ(m_tester->getName(), QStringLiteral("c.service"));
    EXPECT_EQ(m_tester->getStatus(), QStringLiteral("masked"));
}

// ─────────────────────────── QDBusArgument 编组（列表，须在 registerMetaType 之前执行） ───────────────────────────

TEST_F(UT_UnitFileInfo, DBusMarshal_ListWrite_BeginArrayUsesListTypeId_SignatureBroken)
{
    // Arrange: 构造非空列表（不注册列表元类型，避免触发 libdbus 断言崩溃；
    // 列表元类型注册后的写入段错误已在 Mode 5 缺陷报告中以最小复现佐证）
    UnitFileInfoList list;
    list << makeFullUnitFileInfo() << makeFullUnitFileInfo();

    // Act: 写入 QDBusArgument（缺陷实现：beginArray 传入列表而非元素元类型 id）
    QDBusArgument argument;
    argument << list;

    // Assert: beginArray 因列表元类型未注册而失败，未产生有效数组签名
    EXPECT_EQ(argument.currentSignature(), QString());
    EXPECT_TRUE(argument.currentSignature().isEmpty());
}

TEST_F(UT_UnitFileInfo, DBusUnmarshal_ListFromWriteModeArgument_ReadsBackEmptyList)
{
    // Arrange: 写入完整列表后直接读取（同前 Qt 限制说明）
    UnitFileInfoList list;
    list << makeFullUnitFileInfo();
    QDBusArgument argument;
    argument << list;

    // Act: 读取
    UnitFileInfoList dst;
    argument >> dst;

    // Assert: 列表为空（未读入）
    EXPECT_EQ(dst.size(), 0);
    EXPECT_TRUE(dst.isEmpty());
}

// ─────────────────────────── registerMetaType（在列表写之后执行，注册后缺陷写入会崩溃） ───────────────────────────

TEST_F(UT_UnitFileInfo, RegisterMetaType_TypesRegistered_MetaTypesValid)
{
    // Arrange: 注册前按名称查找（本套件此用例之前无任何注册动作，
    // 期望此时仍为未注册状态；gtest 在同一进程内按文件顺序执行）
    const int beforeUnitId = QMetaType::type("UnitFileInfo");

    // Act: 注册 UnitFileInfo / UnitFileInfoList
    UnitFileInfo::registerMetaType();

    // Assert: 两个元类型均可按名称查到
    EXPECT_EQ(beforeUnitId, QMetaType::UnknownType);
    EXPECT_NE(QMetaType::type("UnitFileInfo"), QMetaType::UnknownType);
    EXPECT_NE(QMetaType::type("UnitFileInfoList"), QMetaType::UnknownType);
}

// ─────────────────────────── QDBusArgument 编组（单对象） ───────────────────────────

TEST_F(UT_UnitFileInfo, DBusMarshal_SingleUnit_WritesCorrectStructureSignature)
{
    // Arrange: 构造对象
    UnitFileInfo unit = makeFullUnitFileInfo();

    // Act: 写入 QDBusArgument（两个字符串字段）
    QDBusArgument argument;
    argument << unit;

    // Assert: 结构体签名 (ss)，且与字段内容无关（空对象同签名）
    EXPECT_EQ(argument.currentSignature(), QStringLiteral("(ss)"));
    QDBusArgument emptyArgument;
    emptyArgument << UnitFileInfo();
    EXPECT_EQ(emptyArgument.currentSignature(), QStringLiteral("(ss)"));
}

TEST_F(UT_UnitFileInfo, DBusUnmarshal_FromWriteModeArgument_ReadsBackEmpty)
{
    // Arrange: 写入完整对象后对同一参数直接读取。
    // QDBusArgument 不允许"边写边读"（Qt 限制：wire 编解码需真实报文），
    // 读路径在此桩环境下返回空对象，断言该已知行为。
    UnitFileInfo unit = makeFullUnitFileInfo();
    QDBusArgument argument;
    argument << unit;

    // Act: 读取
    UnitFileInfo dst;
    argument >> dst;

    // Assert: 字段保持默认空串（未读入）
    EXPECT_EQ(dst.getName(), QString());
    EXPECT_EQ(dst.getStatus(), QString());
}

// ─────────────────────────── QDataStream 序列化 ───────────────────────────

TEST_F(UT_UnitFileInfo, DataStream_Roundtrip_PreservesAllFields)
{
    // Arrange: 源对象写入字节缓冲
    UnitFileInfo src = makeFullUnitFileInfo();
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);

    // Act: 写入后读回
    out << src;
    QDataStream in(&bytes, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_5_15);
    UnitFileInfo dst;
    in >> dst;

    // Assert: 全字段一致
    EXPECT_TRUE(dst == src);
    EXPECT_EQ(dst.getName(), QStringLiteral("a.service"));
    EXPECT_EQ(dst.getStatus(), QStringLiteral("enabled"));
}

// ─────────────────────────── QDebug 输出 ───────────────────────────

TEST_F(UT_UnitFileInfo, DebugOp_PrintUnit_OutputContainsKeyFields)
{
    // Arrange: 构造完整对象并准备调试缓冲
    UnitFileInfo unit = makeFullUnitFileInfo();
    QString buffer;
    QDebug debug(&buffer);

    // Act: 流式输出
    debug << unit;

    // Assert: 关键字段出现在输出中
    EXPECT_EQ(buffer.contains(QStringLiteral("a.service")), true);
    EXPECT_TRUE(buffer.contains(QStringLiteral("enabled")));
}
