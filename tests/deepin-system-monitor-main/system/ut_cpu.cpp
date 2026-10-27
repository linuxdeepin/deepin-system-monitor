// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 用例计数声明（self-check-structural 校验此表）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | CPUInfo | mid | - | 2 | 2 |
// | setIndex | mid | - | 2 | 2 |
// | setCoreId | mid | - | 2 | 2 |
// | setCpuFreq | mid | - | 2 | 3 |
// | setVendorId | mid | - | 2 | 2 |
// | setModelName | mid | - | 2 | 2 |
// | setCacheSize | mid | - | 2 | 2 |
// | cpuFreq | low | - | 1 | 3 |
// | coreID | low | - | 1 | 2 |
// | vendorId | low | - | 1 | 2 |
// | modelName | low | - | 1 | 2 |
// | cacheSize | low | - | 1 | 2 |
// | logicalIndex | low | - | 1 | 2 |
// | logicalName | low | - | 1 | 1 |
// | l1dCache | low | - | 1 | 3 |
// | l1iCache | low | - | 1 | 2 |
// | l2Cache | low | - | 1 | 2 |
// | l3Cache | low | - | 1 | 2 |
// | CPUInfo(copy) | low | - | 1 | 2 |
// | operator= | low | - | 1 | 2 |
// | ~CPUInfo | low | - | 1 | 1 |
//
// 分支映射（源码行号，cpu.cpp）：
// - 构造(:44-48) 默认 index=0 / 带索引构造 index=指定值
// - cpuFreq(:63-70) "-" 占位原样返回 / 普通值追加 MHz / 空值仅剩后缀（现状缺陷记录）
// - coreID/logicalName(:72-78) QString::number / 无公开 setter 恒为空
// - 缓存获取器(:80-163) level+type 双匹配 / 无匹配返回空串 / 边界 1024 换算
// - 拷贝构造(:49-57 附近) QSharedData 共享 + 写时分离
// - operator= / 析构(:58-61) 自赋值安全 / 引用计数释放后副本仍有效
//
// 最小检查清单（10 项）：
// 1. 等价类按 test-types.md §1 划分
// 2. 边界值：1024 字节换算阈值、-1 占位、越界
// 3. AAA 三段注释齐全
// 4. 断言强度 ≥2
// 5. 无外部依赖，无需打桩
// 6. 缓存注入使用 CPUCacheList 直接构造
// 7. 三段式命名
// 8. 同质组（四类缓存）参数化
// 9. 私有访问依赖 -fno-access-control
// 10. 不修改生产代码

#include "system/cpu.h"
#include "system/private/cpu_p.h"

#include <gtest/gtest.h>

using namespace core::system;

class UT_CPU : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_tester = new CPUInfo();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
    }

    CPUInfo *m_tester = nullptr;
};

TEST_F(UT_CPU, Ctor_DefaultConstructs_PlaceholderDefaults)
{
    // Arrange
    // Act
    // Assert: 默认值为占位符
    EXPECT_EQ(m_tester->logicalIndex(), 0);
    EXPECT_EQ(m_tester->coreID().toStdString(), std::string("-1"));
    EXPECT_EQ(m_tester->logicalName().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->cpuFreq().toStdString(), std::string("MHz"));
    EXPECT_EQ(m_tester->vendorId().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->modelName().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->cacheSize().toStdString(), std::string(""));
}

TEST_F(UT_CPU, Ctor_WithIndex_SetsLogicalIndex)
{
    // Arrange
    // Act
    CPUInfo info(5);
    // Assert
    EXPECT_EQ(info.logicalIndex(), 5);
    EXPECT_EQ(info.cpuFreq().toStdString(), std::string("MHz"));
}

TEST_F(UT_CPU, SetIndex_Value_RoundTrips)
{
    // Arrange
    // Act
    m_tester->setIndex(9);
    // Assert
    EXPECT_EQ(m_tester->logicalIndex(), 9);
    EXPECT_EQ(m_tester->logicalIndex() != 0, true);
}

TEST_F(UT_CPU, SetIndex_NegativeValue_KeepsNegative)
{
    // Arrange
    // Act
    m_tester->setIndex(-3);
    // Assert
    EXPECT_EQ(m_tester->logicalIndex(), -3);
    EXPECT_EQ(m_tester->logicalIndex() < 0, true);
}

TEST_F(UT_CPU, SetCoreId_Value_FormatsNumberText)
{
    // Arrange
    // Act
    m_tester->setCoreId(3);
    // Assert
    EXPECT_EQ(m_tester->coreID().toStdString(), std::string("3"));
    EXPECT_EQ(m_tester->coreID().isEmpty(), false);
}

TEST_F(UT_CPU, SetCoreId_NegativeValue_FormatsNegativeText)
{
    // Arrange
    // Act
    m_tester->setCoreId(-2);
    // Assert
    EXPECT_EQ(m_tester->coreID().toStdString(), std::string("-2"));
    EXPECT_EQ(m_tester->coreID().isEmpty(), false);
}

TEST_F(UT_CPU, SetCpuFreq_NormalValue_AppendsMhzSuffix)
{
    // Arrange
    // Act
    m_tester->setCpuFreq("2500");
    // Assert
    EXPECT_EQ(m_tester->cpuFreq().toStdString(), std::string("2500MHz"));
    EXPECT_EQ(m_tester->cpuFreq().endsWith("MHz"), true);
}

TEST_F(UT_CPU, SetCpuFreq_DashPlaceholder_ReturnsDashOnly)
{
    // Arrange
    // Act
    m_tester->setCpuFreq("-");
    // Assert: 占位符走短路分支，原样返回
    EXPECT_EQ(m_tester->cpuFreq().toStdString(), std::string("-"));
    EXPECT_EQ(m_tester->cpuFreq().contains("MHz"), false);
}

TEST_F(UT_CPU, SetCpuFreq_EmptyValue_RendersSuffixOnly)
{
    // Arrange
    // Act: 空值不是占位符，现状会拼出裸后缀（记录该现状）
    m_tester->setCpuFreq("");
    // Assert
    EXPECT_EQ(m_tester->cpuFreq().toStdString(), std::string("MHz"));
    EXPECT_EQ(m_tester->cpuFreq().isEmpty(), false);
}

TEST_F(UT_CPU, SetVendorId_Value_RoundTrips)
{
    // Arrange
    // Act
    m_tester->setVendorId("GenuineIntel");
    // Assert
    EXPECT_EQ(m_tester->vendorId().toStdString(), std::string("GenuineIntel"));
    EXPECT_EQ(m_tester->vendorId().isEmpty(), false);
}

TEST_F(UT_CPU, SetVendorId_EmptyValue_StaysEmpty)
{
    // Arrange
    // Act
    m_tester->setVendorId("");
    // Assert
    EXPECT_EQ(m_tester->vendorId().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->vendorId().isEmpty(), true);
}

TEST_F(UT_CPU, SetModelName_Value_RoundTrips)
{
    // Arrange
    // Act
    m_tester->setModelName("Intel(R) Core(TM) i7");
    // Assert
    EXPECT_EQ(m_tester->modelName().toStdString(), std::string("Intel(R) Core(TM) i7"));
    EXPECT_EQ(m_tester->modelName().isEmpty(), false);
}

TEST_F(UT_CPU, SetModelName_EmptyValue_StaysEmpty)
{
    // Arrange
    // Act
    m_tester->setModelName("");
    // Assert
    EXPECT_EQ(m_tester->modelName().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->modelName().isEmpty(), true);
}

TEST_F(UT_CPU, SetCacheSize_RawText_PreservesSpacing)
{
    // Arrange
    // Act: 缓存尺寸为原样透传，不重排版
    m_tester->setCacheSize(" 2048 KiB");
    // Assert
    EXPECT_EQ(m_tester->cacheSize().toStdString(), std::string(" 2048 KiB"));
    EXPECT_EQ(m_tester->cacheSize().startsWith(" "), true);
}

TEST_F(UT_CPU, SetCacheSize_EmptyValue_StaysEmpty)
{
    // Arrange
    // Act
    m_tester->setCacheSize("");
    // Assert
    EXPECT_EQ(m_tester->cacheSize().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->cacheSize().isEmpty(), true);
}

TEST_F(UT_CPU, LogicalName_NoPublicSetter_StaysEmpty)
{
    // Arrange
    // Act: 逻辑名仅能来自底层私有字段，公开接口不写入
    m_tester->setModelName("Intel Xeon");
    // Assert
    EXPECT_EQ(m_tester->logicalName().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->logicalName().isEmpty(), true);
}

TEST_F(UT_CPU, CopyConstructor_SharesData_ReadsSameFields)
{
    // Arrange
    m_tester->setVendorId("GenuineIntel");
    m_tester->setModelName("Intel Xeon");
    // Act
    CPUInfo copy(*m_tester);
    // Assert
    EXPECT_EQ(copy.vendorId().toStdString(), std::string("GenuineIntel"));
    EXPECT_EQ(copy.modelName().toStdString(), std::string("Intel Xeon"));
    EXPECT_EQ(copy.logicalIndex(), 0);
}

TEST_F(UT_CPU, CopyConstructor_ModifyCopy_OriginalUnchanged)
{
    // Arrange
    m_tester->setVendorId("GenuineIntel");
    // Act: 写时分离——副本写入不影响原对象
    CPUInfo copy(*m_tester);
    copy.setVendorId("AuthenticAMD");
    // Assert
    EXPECT_EQ(copy.vendorId().toStdString(), std::string("AuthenticAMD"));
    EXPECT_EQ(m_tester->vendorId().toStdString(), std::string("GenuineIntel"));
    EXPECT_EQ(copy.logicalIndex(), m_tester->logicalIndex());
}

TEST_F(UT_CPU, Dtor_AfterOriginalDestroyed_CopyRemainsValid)
{
    // Arrange
    // Act: 原对象销毁后副本继续可用（引用计数正确性）
    CPUInfo *orig = new CPUInfo();
    orig->setVendorId("GenuineIntel");
    orig->setCoreId(4);
    CPUInfo copy(*orig);
    delete orig;
    // Assert
    EXPECT_EQ(copy.vendorId().toStdString(), std::string("GenuineIntel"));
    EXPECT_EQ(copy.coreID().toStdString(), std::string("4"));
}

TEST_F(UT_CPU, OperatorAssign_FromOther_CopiesFields)
{
    // Arrange
    CPUInfo target;
    m_tester->setVendorId("GenuineIntel");
    m_tester->setCpuFreq("2500");
    // Act
    target = *m_tester;
    // Assert
    EXPECT_EQ(target.vendorId().toStdString(), std::string("GenuineIntel"));
    EXPECT_EQ(target.cpuFreq().toStdString(), std::string("2500MHz"));
}

TEST_F(UT_CPU, OperatorAssign_SelfAssignment_Unchanged)
{
    // Arrange
    m_tester->setVendorId("GenuineIntel");
    CPUInfo &ref = *m_tester;
    // Act
    *m_tester = ref;
    // Assert
    EXPECT_EQ(m_tester->vendorId().toStdString(), std::string("GenuineIntel"));
    EXPECT_EQ(m_tester->logicalIndex(), 0);
}

// ==================== 缓存获取器参数化套件 ====================
// CPUInfoPrivate::cache 在生产代码中仅 dmidecode 路径填充，此处直接注入
enum InfoCacheMethod { IC_L1D, IC_L1I, IC_L2, IC_L3 };

struct InfoCacheParam {
    int level;
    QByteArray type;
    qulonglong size;
    InfoCacheMethod method;
    QString expected;
};

class UT_CPUCache : public UT_CPU, public ::testing::WithParamInterface<InfoCacheParam> {};

TEST_P(UT_CPUCache, CacheGetters_InjectedEntries_ReturnFormattedSize)
{
    // Arrange
    const InfoCacheParam p = GetParam();
    CPUCache entry(new cpu_cache_t());
    entry->level = p.level;
    entry->type = p.type;
    entry->size = p.size;
    m_tester->d->cache.push_back(std::move(entry));
    // Act
    QString actual;
    switch (p.method) {
    case IC_L1D: actual = m_tester->l1dCache(); break;
    case IC_L1I: actual = m_tester->l1iCache(); break;
    case IC_L2: actual = m_tester->l2Cache(); break;
    case IC_L3: actual = m_tester->l3Cache(); break;
    }
    // Assert
    EXPECT_EQ(actual.toStdString(), p.expected.toStdString());
    EXPECT_EQ(actual.isEmpty(), false);
}

INSTANTIATE_TEST_SUITE_P(InfoCache, UT_CPUCache, ::testing::Values(
    InfoCacheParam {1, "Data", 32768ULL, IC_L1D, "32 KB"},
    InfoCacheParam {1, "Data", 1024ULL, IC_L1D, "1024 B"},
    InfoCacheParam {1, "Data", 1025ULL, IC_L1D, "1 KB"},
    InfoCacheParam {1, "Instruction", 65536ULL, IC_L1I, "64 KB"},
    InfoCacheParam {1, "Instruction", 1024ULL, IC_L1I, "1024 B"},
    InfoCacheParam {2, "Unified", 262144ULL, IC_L2, "256 KB"},
    InfoCacheParam {2, "Unified", 1023ULL, IC_L2, "1023 B"},
    InfoCacheParam {3, "Unified", 8388608ULL, IC_L3, "8 MB"},
    InfoCacheParam {3, "Unified", 1073741824ULL, IC_L3, "1024 MB"}
));

TEST_F(UT_CPU, CacheGetters_LevelMatchesButTypeDiffers_ReturnsEmpty)
{
    // Arrange: L1 Data 存在，但查的是 L1 指令缓存
    CPUCache entry(new cpu_cache_t());
    entry->level = 1;
    entry->type = "Data";
    entry->size = 32768ULL;
    m_tester->d->cache.push_back(std::move(entry));
    // Act
    // Assert: type 不匹配返回空串
    EXPECT_EQ(m_tester->l1iCache().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->l2Cache().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->l3Cache().toStdString(), std::string(""));
}

TEST_F(UT_CPU, CacheGetters_EmptyCache_AllGettersEmpty)
{
    // Arrange
    // Act
    // Assert: 无任何缓存条目时四个获取器均为空串
    EXPECT_EQ(m_tester->l1dCache().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->l1iCache().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->l2Cache().toStdString(), std::string(""));
    EXPECT_EQ(m_tester->l3Cache().toStdString(), std::string(""));
}
