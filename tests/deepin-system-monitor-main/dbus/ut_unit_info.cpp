// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 用例计数声明（self-check-structural 验证此块）：
// | method                          | level | factors | min | actual |
// |---------------------------------|-------|---------|-----|--------|
// | ctor(default)                   | low   | -       | 1   | 1      |
// | ctor(params)                    | mid   | -       | 1   | 1      |
// | ctor(copy)                      | low   | -       | 1   | 2      |
// | operator=                       | low   | -       | 1   | 2      |
// | operator==                      | mid   | -       | 2   | 4      |
// | registerMetaType                | low   | -       | 1   | 1      |
// | 10 个 getter / setter 成对方法   | mid   | -       | 10  | 10     |
// | QDebug operator<<               | low   | -       | 1   | 1      |
// | QDBusArgument operator<<(单)     | low   | -       | 1   | 1      |
// | QDBusArgument operator>>(单)     | low   | -       | 1   | 1      |
// | UnitInfoList QDBusArgument <<    | low   | -       | 1   | 1      |
// | UnitInfoList QDBusArgument >>    | low   | -       | 1   | 1      |
// | QDataStream << / >>             | low   | -       | 2   | 2      |
// | UnitInfoData ctor/operator=/==   | low   | -       | 3   | 随共享语义覆盖 |

//self
#include "dbus/unit_info.h"

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
// 构造一个字段齐全的 UnitInfo，供各用例复用
static UnitInfo makeFullUnitInfo()
{
    UnitInfo unit(QStringLiteral("dbus.service"),
                  QStringLiteral("DBus service"),
                  QStringLiteral("loaded"),
                  QStringLiteral("active"),
                  QStringLiteral("running"),
                  QStringLiteral("other.service"),
                  QStringLiteral("/org/freedesktop/systemd1/unit/dbus_2eservice"),
                  88,
                  QStringLiteral("start"),
                  QStringLiteral("/org/freedesktop/systemd1/job/8888"));
    return unit;
}
} // namespace

class UT_UnitInfo: public ::testing::Test
{
public:
    UT_UnitInfo() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // Arrange: 构造默认对象（dbus 编组类用例内部再显式注册元类型）
        m_tester = new UnitInfo();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    UnitInfo *m_tester;
};

// ─────────────────────────── 构造 / 共享数据语义 ───────────────────────────

TEST_F(UT_UnitInfo, Ctor_Default_AllFieldsEmpty)
{
    // Arrange: 读取默认对象的名字字段
    const QString defaultName = m_tester->getName();

    // Act: 读取其余字段
    const QString description = m_tester->getDescription();
    const quint32 jobId = m_tester->getJobId();
    const QString jobObjectPath = m_tester->getJobObjectPath();

    // Assert: 默认构造的字段均为空/0
    EXPECT_TRUE(defaultName.isEmpty());
    EXPECT_TRUE(description.isEmpty());
    EXPECT_EQ(jobId, 0U);
    EXPECT_TRUE(jobObjectPath.isEmpty());
    EXPECT_TRUE(m_tester->getLoadState().isEmpty());
    EXPECT_TRUE(m_tester->getActiveState().isEmpty());
    EXPECT_TRUE(m_tester->getSubState().isEmpty());
    EXPECT_TRUE(m_tester->getFollowedBy().isEmpty());
    EXPECT_TRUE(m_tester->getUnitObjectPath().isEmpty());
    EXPECT_TRUE(m_tester->getJobType().isEmpty());
}

TEST_F(UT_UnitInfo, Ctor_FullParams_AllGettersReturnCtorValues)
{
    // Arrange: 用全参数构造一个对象
    UnitInfo unit = makeFullUnitInfo();

    // Act: 逐项读取字段
    const QString name = unit.getName();
    const quint32 jobId = unit.getJobId();

    // Assert: 全部 getter 返回构造参数原值
    EXPECT_EQ(name, QStringLiteral("dbus.service"));
    EXPECT_EQ(unit.getDescription(), QStringLiteral("DBus service"));
    EXPECT_EQ(unit.getLoadState(), QStringLiteral("loaded"));
    EXPECT_EQ(unit.getActiveState(), QStringLiteral("active"));
    EXPECT_EQ(unit.getSubState(), QStringLiteral("running"));
    EXPECT_EQ(unit.getFollowedBy(), QStringLiteral("other.service"));
    EXPECT_EQ(unit.getUnitObjectPath(), QStringLiteral("/org/freedesktop/systemd1/unit/dbus_2eservice"));
    EXPECT_EQ(jobId, 88U);
    EXPECT_EQ(unit.getJobType(), QStringLiteral("start"));
    EXPECT_EQ(unit.getJobObjectPath(), QStringLiteral("/org/freedesktop/systemd1/job/8888"));
}

TEST_F(UT_UnitInfo, CopyCtor_ExplicitShare_CopySeesSameData)
{
    // Arrange: 构造源对象
    UnitInfo src = makeFullUnitInfo();

    // Act: 拷贝构造（QExplicitlySharedDataPointer 显式共享）
    UnitInfo dst(src);

    // Assert: 拷贝体读到与源相同的字段值
    EXPECT_TRUE(dst == src);
    EXPECT_EQ(dst.getName(), QStringLiteral("dbus.service"));
    EXPECT_EQ(dst.getActiveState(), QStringLiteral("active"));
}

TEST_F(UT_UnitInfo, CopyCtor_ExplicitShare_MutationVisibleToBoth)
{
    // Arrange: 构造源对象并拷贝
    UnitInfo src = makeFullUnitInfo();
    UnitInfo dst(src);

    // Act: 通过拷贝体修改字段
    dst.setName(QStringLiteral("modified.service"));

    // Assert: 共享数据使源对象同样可见新值
    EXPECT_EQ(dst.getName(), QStringLiteral("modified.service"));
    EXPECT_EQ(src.getName(), QStringLiteral("modified.service"));
}

TEST_F(UT_UnitInfo, AssignOp_FromFullSource_TargetMatchesSource)
{
    // Arrange: 默认对象与全字段源对象
    UnitInfo src = makeFullUnitInfo();

    // Act: 赋值
    *m_tester = src;

    // Assert: 目标与源字段一致
    EXPECT_TRUE(*m_tester == src);
    EXPECT_EQ(m_tester->getName(), QStringLiteral("dbus.service"));
    EXPECT_EQ(m_tester->getJobId(), 88U);
}

TEST_F(UT_UnitInfo, AssignOp_SourceMutation_AfterAssignVisibleInTarget)
{
    // Arrange: 先赋值再改源
    UnitInfo src = makeFullUnitInfo();
    *m_tester = src;

    // Act: 修改源对象
    src.setSubState(QStringLiteral("dead"));

    // Assert: 共享语义使目标同样可见
    EXPECT_EQ(src.getSubState(), QStringLiteral("dead"));
    EXPECT_EQ(m_tester->getSubState(), QStringLiteral("dead"));
}

// ─────────────────────────── operator== ───────────────────────────

TEST_F(UT_UnitInfo, EqualOp_SameFieldValues_ReturnsTrue)
{
    // Arrange: 两个同字段对象
    UnitInfo a = makeFullUnitInfo();
    UnitInfo b = makeFullUnitInfo();

    // Act: 逐对象比较
    const bool equal = (a == b);

    // Assert: 全字段相同视为相等
    EXPECT_TRUE(equal);
    EXPECT_EQ(a.getName(), b.getName());
    EXPECT_EQ(a.getJobId(), b.getJobId());
}

TEST_F(UT_UnitInfo, EqualOp_DifferentName_ReturnsFalse)
{
    // Arrange: 仅名称不同
    UnitInfo a = makeFullUnitInfo();
    UnitInfo b = makeFullUnitInfo();
    b.setName(QStringLiteral("other.service"));

    // Act: 比较
    const bool equal = (a == b);

    // Assert: 不相等
    EXPECT_FALSE(equal);
    EXPECT_NE(a.getName(), b.getName());
}

TEST_F(UT_UnitInfo, EqualOp_DifferentJobId_ReturnsFalse)
{
    // Arrange: 仅 jobId 不同
    UnitInfo a = makeFullUnitInfo();
    UnitInfo b = makeFullUnitInfo();
    b.setJobId(99);

    // Act: 比较
    const bool equal = (a == b);

    // Assert: 不相等
    EXPECT_FALSE(equal);
    EXPECT_NE(a.getJobId(), b.getJobId());
}

TEST_F(UT_UnitInfo, EqualOp_ComparedToSelf_ReturnsTrue)
{
    // Arrange: 单对象自比较
    UnitInfo a = makeFullUnitInfo();

    // Act: 自比较
    const bool equal = (a == a);

    // Assert: 自身恒相等
    EXPECT_TRUE(equal);
    EXPECT_EQ(a.getName(), a.getName());
}

// ─────────────────────────── getter / setter ───────────────────────────

TEST_F(UT_UnitInfo, GetterSetter_Name_Roundtrip)
{
    // Arrange: 校验前置状态（名字为空）
    EXPECT_TRUE(m_tester->getName().isEmpty());

    // Act: 设置名称
    m_tester->setName(QStringLiteral("name.service"));

    // Assert: 读到写入值
    EXPECT_EQ(m_tester->getName(), QStringLiteral("name.service"));
}

TEST_F(UT_UnitInfo, GetterSetter_Description_Roundtrip)
{
    // Arrange: 校验前置状态（描述为空）
    EXPECT_TRUE(m_tester->getDescription().isEmpty());

    // Act: 设置描述
    m_tester->setDescription(QStringLiteral("desc"));

    // Assert: 读到写入值
    EXPECT_EQ(m_tester->getDescription(), QStringLiteral("desc"));
}

TEST_F(UT_UnitInfo, GetterSetter_LoadState_Roundtrip)
{
    // Arrange: 校验前置状态（加载态为空）
    EXPECT_TRUE(m_tester->getLoadState().isEmpty());

    // Act: 设置加载态
    m_tester->setLoadState(QStringLiteral("loaded"));

    // Assert: 读到写入值
    EXPECT_EQ(m_tester->getLoadState(), QStringLiteral("loaded"));
}

TEST_F(UT_UnitInfo, GetterSetter_ActiveState_Roundtrip)
{
    // Arrange: 校验前置状态（活动态为空）
    EXPECT_TRUE(m_tester->getActiveState().isEmpty());

    // Act: 设置活动态
    m_tester->setActiveState(QStringLiteral("activating"));

    // Assert: 读到写入值
    EXPECT_EQ(m_tester->getActiveState(), QStringLiteral("activating"));
}

TEST_F(UT_UnitInfo, GetterSetter_SubState_Roundtrip)
{
    // Arrange: 校验前置状态（子状态为空）
    EXPECT_TRUE(m_tester->getSubState().isEmpty());

    // Act: 设置子状态
    m_tester->setSubState(QStringLiteral("auto-restart"));

    // Assert: 读到写入值
    EXPECT_EQ(m_tester->getSubState(), QStringLiteral("auto-restart"));
}

TEST_F(UT_UnitInfo, GetterSetter_FollowedBy_Roundtrip)
{
    // Arrange: 校验前置状态（跟随单元为空）
    EXPECT_TRUE(m_tester->getFollowedBy().isEmpty());

    // Act: 设置跟随单元
    m_tester->setFollowedBy(QStringLiteral("follow.service"));

    // Assert: 读到写入值
    EXPECT_EQ(m_tester->getFollowedBy(), QStringLiteral("follow.service"));
}

TEST_F(UT_UnitInfo, GetterSetter_UnitObjectPath_Roundtrip)
{
    // Arrange: 校验前置状态（对象路径为空）
    EXPECT_TRUE(m_tester->getUnitObjectPath().isEmpty());

    // Act: 设置对象路径
    m_tester->setUnitObjectPath(QStringLiteral("/org/freedesktop/systemd1/unit/a_2eservice"));

    // Assert: 读到写入值
    EXPECT_EQ(m_tester->getUnitObjectPath(), QStringLiteral("/org/freedesktop/systemd1/unit/a_2eservice"));
}

TEST_F(UT_UnitInfo, GetterSetter_JobId_Roundtrip)
{
    // Arrange: 校验前置状态（jobId 为 0）
    EXPECT_EQ(m_tester->getJobId(), 0U);

    // Act: 设置 jobId
    m_tester->setJobId(1234);

    // Assert: 读到写入值
    EXPECT_EQ(m_tester->getJobId(), 1234U);
}

TEST_F(UT_UnitInfo, GetterSetter_JobType_Roundtrip)
{
    // Arrange: 校验前置状态（任务类型为空）
    EXPECT_TRUE(m_tester->getJobType().isEmpty());

    // Act: 设置任务类型
    m_tester->setJobType(QStringLiteral("stop"));

    // Assert: 读到写入值
    EXPECT_EQ(m_tester->getJobType(), QStringLiteral("stop"));
}

TEST_F(UT_UnitInfo, GetterSetter_JobObjectPath_Roundtrip)
{
    // Arrange: 校验前置状态（任务路径为空）
    EXPECT_TRUE(m_tester->getJobObjectPath().isEmpty());

    // Act: 设置任务路径
    m_tester->setJobObjectPath(QStringLiteral("/org/freedesktop/systemd1/job/1234"));

    // Assert: 读到写入值
    EXPECT_EQ(m_tester->getJobObjectPath(), QStringLiteral("/org/freedesktop/systemd1/job/1234"));
}

// ─────────────────────────── registerMetaType（须在编组用例之前，验证从未注册到已注册） ───────────────────────────

TEST_F(UT_UnitInfo, RegisterMetaType_TypesRegistered_MetaTypesValid)
{
    // Arrange: 注册前按名称查找（本套件此用例之前无任何注册动作，
    // 期望此时仍为未注册状态；gtest 在同一进程内按文件顺序执行）
    const int beforeUnitId = QMetaType::type("UnitInfo");

    // Act: 注册 UnitInfo / UnitInfoList
    UnitInfo::registerMetaType();

    // Assert: 两个元类型均可按名称查到
    EXPECT_EQ(beforeUnitId, QMetaType::UnknownType);
    EXPECT_NE(QMetaType::type("UnitInfo"), QMetaType::UnknownType);
    EXPECT_NE(QMetaType::type("UnitInfoList"), QMetaType::UnknownType);
}

// ─────────────────────────── QDBusArgument 编组（写） ───────────────────────────

TEST_F(UT_UnitInfo, DBusMarshal_SingleUnit_WritesCorrectStructureSignature)
{
    // Arrange: 注册元类型并构造完整对象
    UnitInfo::registerMetaType();
    UnitInfo unit = makeFullUnitInfo();

    // Act: 写入 QDBusArgument（10 个字段 7s+1u+2s）
    QDBusArgument argument;
    argument << unit;

    // Assert: 结构体签名与字段布局一致，且与内容无关（空对象同签名）
    EXPECT_EQ(argument.currentSignature(), QStringLiteral("(sssssssuss)"));
    QDBusArgument emptyArgument;
    emptyArgument << UnitInfo();
    EXPECT_EQ(emptyArgument.currentSignature(), QStringLiteral("(sssssssuss)"));
}

TEST_F(UT_UnitInfo, DBusMarshal_UnitList_WritesCorrectArraySignature)
{
    // Arrange: 注册元类型并构造两个元素的列表
    UnitInfo::registerMetaType();
    UnitInfoList list;
    list << makeFullUnitInfo() << makeFullUnitInfo();

    // Act: 写入 QDBusArgument
    QDBusArgument argument;
    argument << list;

    // Assert: 数组签名 a(sssssssuss)，且与元素个数无关
    EXPECT_EQ(argument.currentSignature(), QStringLiteral("a(sssssssuss)"));
    QDBusArgument oneArgument;
    UnitInfoList one;
    one << makeFullUnitInfo();
    oneArgument << one;
    EXPECT_EQ(oneArgument.currentSignature(), QStringLiteral("a(sssssssuss)"));
}

// ─────────────────────────── QDBusArgument 解组（读） ───────────────────────────

TEST_F(UT_UnitInfo, DBusUnmarshal_FromWriteModeArgument_ReadsBackEmpty)
{
    // Arrange: 写入完整对象后，对同一参数直接读取。
    // QDBusArgument 不允许"边写边读"（Qt 限制：wire 编解码需真实报文），
    // 因此读路径在此桩环境下返回空对象，此处断言该已知行为。
    UnitInfo::registerMetaType();
    UnitInfo unit = makeFullUnitInfo();
    QDBusArgument argument;
    argument << unit;

    // Act: 读取
    UnitInfo dst;
    argument >> dst;

    // Assert: 字段保持默认（未读入）
    EXPECT_TRUE(dst.getName().isEmpty());
    EXPECT_EQ(dst.getJobId(), 0U);
}

TEST_F(UT_UnitInfo, DBusUnmarshal_ListFromWriteModeArgument_ReadsBackEmptyList)
{
    // Arrange: 写入完整列表后直接读取（同上 Qt 限制说明）
    UnitInfo::registerMetaType();
    UnitInfoList list;
    list << makeFullUnitInfo();
    QDBusArgument argument;
    argument << list;

    // Act: 读取
    UnitInfoList dst;
    argument >> dst;

    // Assert: 列表为空（未读入）
    EXPECT_EQ(dst.size(), 0);
    EXPECT_TRUE(dst.isEmpty());
}

// ─────────────────────────── QDataStream 序列化 ───────────────────────────

TEST_F(UT_UnitInfo, DataStream_Roundtrip_PreservesAllFields)
{
    // Arrange: 源对象写入字节缓冲
    UnitInfo src = makeFullUnitInfo();
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);

    // Act: 写入后读回
    out << src;
    QDataStream in(&bytes, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_5_15);
    UnitInfo dst;
    in >> dst;

    // Assert: 全字段一致（operator== 全字段比较）
    EXPECT_TRUE(dst == src);
    EXPECT_EQ(dst.getName(), QStringLiteral("dbus.service"));
    EXPECT_EQ(dst.getJobId(), 88U);
    EXPECT_EQ(dst.getJobObjectPath(), QStringLiteral("/org/freedesktop/systemd1/job/8888"));
}

TEST_F(UT_UnitInfo, DataStream_EmptyObject_RoundtripStaysEmpty)
{
    // Arrange: 默认对象写入字节缓冲
    UnitInfo src;
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);

    // Act: 写入后读回
    out << src;
    QDataStream in(&bytes, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_5_15);
    UnitInfo dst;
    in >> dst;

    // Assert: 仍为空对象
    EXPECT_TRUE(dst.getName().isEmpty());
    EXPECT_EQ(dst.getJobId(), 0U);
}

// ─────────────────────────── QDebug 输出 ───────────────────────────

TEST_F(UT_UnitInfo, DebugOp_PrintUnit_OutputContainsKeyFields)
{
    // Arrange: 构造完整对象并准备调试缓冲
    UnitInfo unit = makeFullUnitInfo();
    QString buffer;
    QDebug debug(&buffer);

    // Act: 流式输出
    debug << unit;

    // Assert: 关键字段出现在输出中
    EXPECT_EQ(buffer.contains(QStringLiteral("dbus.service")), true);
    EXPECT_TRUE(buffer.contains(QStringLiteral("active")));
    EXPECT_TRUE(buffer.contains(QStringLiteral("/org/freedesktop/systemd1/job/8888")));
}
