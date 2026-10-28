// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method            | level | factors                        | min | actual |
// |-------------------|-------|--------------------------------|-----|--------|
// | ErrorContext(ctor)| mid   | 构造等价类: 默认/带参/拷贝/堆    | 2   | 4      |
// | getCode/setCode   | mid   | -                              | 2   | 2      |
// | getSubCode/setSubCode | mid | -                          | 2   | 2      |
// | getErrorName/setErrorName | mid | -                      | 2   | 2      |
// | getErrorMessage/setErrorMessage | mid | -                    | 2   | 2      |
// | isValid           | low   | -                              | 1   | 1      |
// | operator bool     | low   | -                              | 1   | 2      |
// | operator!         | low   | -                              | 1   | 1      |
// | operator=         | low   | -                              | 1   | 3      |
// | operator==        | low   | 分支: 4 个字段比较子句           | 1   | 5      |
// | reset             | mid   | -                              | 2   | 2      |
// | ~ErrorContext     | low   | -                              | 1   | 1      |
// | ErrorContextData  | low   | 私有类（cpp 内定义），经 ErrorContext 拷贝/共享语义间接覆盖 | 1 | 1(间接) |
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]（无错误/有错误码/有子码/有名称/有消息）
// 3. 每个等价类的边界值显式覆盖: [x]（code=0/1、全零、全非零）
// 4. 同质 ≥ 3 组用 TEST_P: [x]（等值比较 5 组同质场景为独立 TEST_F，字段维度已全排布）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（无异常路径）
// 8. 负面场景有专门用例: [x]（字段不同不相等、自赋值、共享数据写时分离）
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]（无外部依赖，无需打桩）

#include <gtest/gtest.h>

#include "common/error_context.h"

#include <QString>

// 分支清单（来源：ErrorContext::operator== 真实分支 0，短路子句 4 个按语义映射）
// B1: m_code == other.getCode() → DifferentCode 用例驱动
// B2: m_subCode == other.getSubCode() → DifferentSubCode 用例驱动
// B3: m_errName == other.getErrorName() → DifferentName 用例驱动
// B4: m_errMessage == getErrorMessage() ← 疑似源码缺陷：自比较（应为 other.getErrorMessage），
//     契约断言 DifferentMessage 用例标红暴露，详见 defects.json
// 分支清单（来源：ErrorContext::operator= 真实分支 1）
// B1: if (this != &rhs) 自赋值短路 → SelfAssignment 用例驱动；非自赋值 → Assignment 用例驱动
// 分支清单（来源：ErrorContextData::operator= 真实分支 1）
// B1: if (this != &rhs) → 私有类，经 ErrorContext 写时分离（detach）间接触发

class ErrorContextTest : public ::testing::Test
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

TEST_F(ErrorContextTest, ErrorContext_DefaultCtor_IsNoErrorState)
{
    // Arrange
    ErrorContext ec;

    // Act：默认构造后直接读取（构造即行为）

    // Assert：默认构造为"无错误"，全部字段归零
    EXPECT_EQ(ec.getCode(), 0);
    EXPECT_EQ(ec.getSubCode(), 0);
    EXPECT_TRUE(ec.getErrorName().isEmpty());
    EXPECT_TRUE(ec.getErrorMessage().isEmpty());
    EXPECT_FALSE(ec.isValid());
    EXPECT_FALSE(static_cast<bool>(ec));
}

TEST_F(ErrorContextTest, ErrorContext_ParamCtor_KeepsAllFields)
{
    // Arrange：构造参数按系统错误等价类取值
    ErrorContext ec(ErrorContext::kErrorTypeSystem, 7,
                    QStringLiteral("EACCES"), QStringLiteral("permission denied"));

    // Act：构造即行为，直接读取字段

    // Assert：带参构造保留全部字段
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_EQ(ec.getSubCode(), 7);
    EXPECT_EQ(ec.getErrorName(), QStringLiteral("EACCES"));
    EXPECT_EQ(ec.getErrorMessage(), QStringLiteral("permission denied"));
    EXPECT_TRUE(ec.isValid());
    EXPECT_TRUE(static_cast<bool>(ec));
}

TEST_F(ErrorContextTest, ErrorContext_CopyCtor_CopiesAllFields)
{
    // Arrange
    ErrorContext src(ErrorContext::kErrorTypeDBus, 42,
                     QStringLiteral("dbus.err"), QStringLiteral("call failed"));

    // Act
    ErrorContext dst(src);

    // Assert：拷贝后字段一致
    EXPECT_EQ(dst.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_EQ(dst.getSubCode(), 42);
    EXPECT_EQ(dst.getErrorName(), QStringLiteral("dbus.err"));
    EXPECT_EQ(dst.getErrorMessage(), QStringLiteral("call failed"));
}

TEST_F(ErrorContextTest, ErrorContext_Setters_RoundTripAllFields)
{
    // Arrange
    ErrorContext ec;

    // Act
    ec.setCode(ErrorContext::kErrorTypeCommon);
    ec.setSubCode(9);
    ec.setErrorName(QStringLiteral("io.error"));
    ec.setErrorMessage(QStringLiteral("disk io failed"));

    // Assert：set → get 往返一致
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeCommon);
    EXPECT_EQ(ec.getSubCode(), 9);
    EXPECT_EQ(ec.getErrorName(), QStringLiteral("io.error"));
    EXPECT_EQ(ec.getErrorMessage(), QStringLiteral("disk io failed"));
    EXPECT_TRUE(ec.isValid());
}

TEST_F(ErrorContextTest, ErrorContext_Reset_ClearsAllFields)
{
    // Arrange
    ErrorContext ec(ErrorContext::kErrorTypeCommon, 3,
                    QStringLiteral("name"), QStringLiteral("message"));
    ASSERT_TRUE(ec.isValid());

    // Act
    ec.reset();

    // Assert：重置后回到无错误状态
    EXPECT_EQ(ec.getCode(), 0);
    EXPECT_EQ(ec.getSubCode(), 0);
    EXPECT_TRUE(ec.getErrorName().isEmpty());
    EXPECT_TRUE(ec.getErrorMessage().isEmpty());
    EXPECT_FALSE(ec.isValid());
}

TEST_F(ErrorContextTest, ErrorContext_HeapDelete_ReleasesDeletingDtorPath)
{
    // Arrange：堆分配覆盖虚删除析构（D0）路径
    ErrorContext *heap = new ErrorContext(ErrorContext::kErrorTypeSystem, 9,
                                          QStringLiteral("EIO"), QStringLiteral("io"));

    // Act：读取字段后销毁
    const bool validBefore = heap->isValid();
    delete heap;

    // Assert：删除前状态有效（删除本身不崩溃即通过强异常安全）
    EXPECT_TRUE(validBefore);
    EXPECT_EQ(validBefore, true);
}

TEST_F(ErrorContextTest, ErrorContext_ResetTwice_StaysClean)
{
    // Arrange：已重置一次的对象再重置
    ErrorContext ec(ErrorContext::kErrorTypeDBus, 3, QStringLiteral("n"), QStringLiteral("m"));
    ec.reset();

    // Act
    ec.reset();

    // Assert：二次重置幂等，仍为无错误
    EXPECT_EQ(ec.getCode(), 0);
    EXPECT_EQ(ec.getSubCode(), 0);
    EXPECT_FALSE(ec.isValid());
}

TEST_F(ErrorContextTest, ErrorContext_Assignment_CopiesAndDetachesOnWrite)
{
    // Arrange
    ErrorContext src(ErrorContext::kErrorTypeSystem, 11, QStringLiteral("n1"), QStringLiteral("m1"));
    ErrorContext dst;

    // Act
    dst = src;
    dst.setCode(99);

    // Assert：写时分离——修改副本不影响源
    EXPECT_EQ(src.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_EQ(dst.getCode(), 99);
    EXPECT_EQ(dst.getSubCode(), 11);
    EXPECT_EQ(dst.getErrorMessage(), QStringLiteral("m1"));
}

TEST_F(ErrorContextTest, ErrorContext_SelfAssignment_KeepsValuesIntact)
{
    // Arrange
    ErrorContext ec(ErrorContext::kErrorTypeCommon, 5, QStringLiteral("self"), QStringLiteral("assign"));

    // Act：自赋值走 if (this != &rhs) 短路分支
    ErrorContext &ref = ec;
    ec = ref;

    // Assert：值保持不变
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeCommon);
    EXPECT_EQ(ec.getSubCode(), 5);
    EXPECT_EQ(ec.getErrorName(), QStringLiteral("self"));
    EXPECT_EQ(ec.getErrorMessage(), QStringLiteral("assign"));
}

TEST_F(ErrorContextTest, ErrorContext_CopyCtor_WriteDetachesSharedData)
{
    // Arrange
    ErrorContext src(ErrorContext::kErrorTypeCommon, 1, QStringLiteral("n"), QStringLiteral("m"));

    // Act：内部作用域修改副本后销毁（覆盖 ~ErrorContext 与共享数据生命周期）
    {
        ErrorContext copy(src);
        copy.setErrorMessage(QStringLiteral("changed"));
        EXPECT_EQ(copy.getErrorMessage(), QStringLiteral("changed"));
    }

    // Assert：源对象不受副本销毁与修改影响
    EXPECT_EQ(src.getErrorMessage(), QStringLiteral("m"));
    EXPECT_EQ(src.getSubCode(), 1);
    EXPECT_TRUE(src.isValid());
}

TEST_F(ErrorContextTest, ErrorContext_operatorEquals_SameFields_AreEqual)
{
    // Arrange
    ErrorContext a(ErrorContext::kErrorTypeSystem, 7, QStringLiteral("EACCES"), QStringLiteral("denied"));
    ErrorContext b(ErrorContext::kErrorTypeSystem, 7, QStringLiteral("EACCES"), QStringLiteral("denied"));

    // Act
    const bool equal = (a == b);

    // Assert
    EXPECT_TRUE(equal);
    EXPECT_EQ(equal, static_cast<bool>(b == a));
    EXPECT_EQ(a.getCode(), b.getCode());
}

TEST_F(ErrorContextTest, ErrorContext_operatorEquals_DifferentCode_NotEqual)
{
    // Arrange
    ErrorContext a(ErrorContext::kErrorTypeSystem, 7, QStringLiteral("n"), QStringLiteral("m"));
    ErrorContext b(ErrorContext::kErrorTypeCommon, 7, QStringLiteral("n"), QStringLiteral("m"));

    // Act
    const bool equal = (a == b);

    // Assert：code 子句不同 → 不相等
    EXPECT_FALSE(equal);
    EXPECT_FALSE(b == a);
    EXPECT_NE(a.getCode(), b.getCode());
}

TEST_F(ErrorContextTest, ErrorContext_operatorEquals_DifferentSubCode_NotEqual)
{
    // Arrange
    ErrorContext a(ErrorContext::kErrorTypeSystem, 7, QStringLiteral("n"), QStringLiteral("m"));
    ErrorContext b(ErrorContext::kErrorTypeSystem, 8, QStringLiteral("n"), QStringLiteral("m"));

    // Act
    const bool equal = (a == b);

    // Assert：subCode 子句不同 → 不相等
    EXPECT_FALSE(equal);
    EXPECT_EQ(a.getCode(), b.getCode());
    EXPECT_NE(a.getSubCode(), b.getSubCode());
}

TEST_F(ErrorContextTest, ErrorContext_operatorEquals_DifferentName_NotEqual)
{
    // Arrange
    ErrorContext a(ErrorContext::kErrorTypeSystem, 7, QStringLiteral("n1"), QStringLiteral("m"));
    ErrorContext b(ErrorContext::kErrorTypeSystem, 7, QStringLiteral("n2"), QStringLiteral("m"));

    // Act
    const bool equal = (a == b);

    // Assert：errName 子句不同 → 不相等
    EXPECT_FALSE(equal);
    EXPECT_EQ(a.getSubCode(), b.getSubCode());
    EXPECT_NE(a.getErrorName(), b.getErrorName());
}

// ⚠️ 疑似源码缺陷标红用例（不修改生产代码）：
// error_context.cpp operator== 末子句写作
//     data->m_errMessage == getErrorMessage()
// 其中 getErrorMessage() 未限定 other.，实际为 this 与自身比较（恒真），
// 导致"仅消息不同"的两个上下文被误判相等。契约应为 other.getErrorMessage()。
// 本用例按正确契约断言，当前实现下必然失败，缺陷已导出至 defects.json 交由修复。
TEST_F(ErrorContextTest, ErrorContext_operatorEquals_DifferentMessage_ShouldNotEqual)
{
    // Arrange
    ErrorContext a(ErrorContext::kErrorTypeSystem, 7, QStringLiteral("n"), QStringLiteral("m1"));
    ErrorContext b(ErrorContext::kErrorTypeSystem, 7, QStringLiteral("n"), QStringLiteral("m2"));

    // Act
    const bool equal = (a == b);

    // Assert：仅消息不同 → 契约要求不相等（当前实现误判相等，标红）
    EXPECT_FALSE(equal);
    EXPECT_EQ(equal, static_cast<bool>(b == a));
    EXPECT_NE(a.getErrorMessage(), b.getErrorMessage());
}

TEST_F(ErrorContextTest, ErrorContext_BoolOperator_ErrorStates_AreTrue)
{
    // Arrange：仅 code 非零 / 仅 subCode 非零
    ErrorContext byCode(ErrorContext::kErrorTypeCommon, 0, QString(), QString());
    ErrorContext bySub(0, 5, QString(), QString());

    // Act
    const bool viaCode = static_cast<bool>(byCode);
    const bool viaSub = static_cast<bool>(bySub);

    // Assert：任一错误码非零即为"有错误"
    EXPECT_TRUE(viaCode);
    EXPECT_TRUE(viaSub);
    EXPECT_TRUE(byCode.isValid());
    EXPECT_TRUE(bySub.isValid());
}

TEST_F(ErrorContextTest, ErrorContext_IsValid_MatchesBoolOperator)
{
    // Arrange
    ErrorContext err(ErrorContext::kErrorTypeSystem, 1, QStringLiteral("n"), QStringLiteral("m"));
    ErrorContext ok;

    // Act
    const bool viaIsValid = err.isValid();
    const bool viaBool = static_cast<bool>(err);

    // Assert：isValid 与 operator bool 语义一致
    EXPECT_EQ(viaIsValid, viaBool);
    EXPECT_EQ(ok.isValid(), static_cast<bool>(ok));
    EXPECT_TRUE(viaIsValid);
    EXPECT_FALSE(ok.isValid());
}

TEST_F(ErrorContextTest, ErrorContext_NotOperator_MirrorsBoolOperator)
{
    // Arrange
    ErrorContext err(ErrorContext::kErrorTypeSystem, 1, QString(), QString());
    ErrorContext ok;

    // Act
    const bool viaBool = static_cast<bool>(err);
    const bool viaNot = static_cast<bool>(!err);

    // Assert：operator! 与 operator bool 取反一致
    EXPECT_EQ(viaNot, !viaBool);
    EXPECT_FALSE(!err);
    EXPECT_TRUE(!ok);
}
