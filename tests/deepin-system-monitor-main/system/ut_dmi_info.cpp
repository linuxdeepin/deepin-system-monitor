// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// DmiInfo 单元测试：验证 SMBIOS/DMI 表解析逻辑
//
// 通过 DmiInfo::reloadForTest 将解析目标重定向到临时 sysfs 目录，
// 注入手工构造的 SMBIOS 表，不依赖真实设备状态与 root 权限。
//
// 覆盖点：
// - Type 1/4/7/11 结构解析（产品名、频率、缓存、OEM 字符串）
// - 缓存容量 16 位/32 位（SMBIOS 3.1+）编码
// - 字符串索引边界（0、越界）、非可打印字符过滤
// - 畸形表防护（结构头截断、字符串区未终止、空表）
// - 单例语义与 reloadForTest 重置

#include "system/dmi_info.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cstring>

namespace {

// ==================== SMBIOS 结构构造辅助 ====================

// 按 SMBIOS 规范拼接单个结构：头部（type/length/handle）+ 格式化区填充 + 字符串区 + 列表终止 NUL
// 注：每个字符串自带 NUL 结尾，字符串列表以一个额外的 NUL 结束（无字符串时为双 NUL）
struct DmiStructBuilder
{
    QByteArray data;
    bool m_hasStrings = false;

    void addHeader(uchar type, uchar len, quint16 handle)
    {
        data += char(type);
        data += char(len);
        data += char(handle & 0xFF);
        data += char(handle >> 8);
        while (data.size() < len)
            data += char(0);
    }

    void setByte(int offset, uchar value) { data[offset] = char(value); }

    void setWord(int offset, quint16 value)
    {
        data[offset] = char(value & 0xFF);
        data[offset + 1] = char((value >> 8) & 0xFF);
    }

    void setDWord(int offset, quint32 value)
    {
        data[offset] = char(value & 0xFF);
        data[offset + 1] = char((value >> 8) & 0xFF);
        data[offset + 2] = char((value >> 16) & 0xFF);
        data[offset + 3] = char((value >> 24) & 0xFF);
    }

    void addString(const char *s)
    {
        data += s;
        data += char(0);
        m_hasStrings = true;
    }

    void finish()
    {
        if (!m_hasStrings)
            data += char(0);  // 无字符串：空字符串的终止 NUL
        data += char(0);      // 字符串列表终止 NUL
    }
};

// 将表写入临时 sysfs 目录并触发 DmiInfo 重新加载；table 为空时使用空目录
static void loadTable(const QByteArray &table)
{
    QTemporaryDir dir;
    if (!table.isEmpty()) {
        QDir().mkpath(dir.path() + "/firmware/dmi/tables");
        QFile f(dir.path() + "/firmware/dmi/tables/DMI");
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        ASSERT_EQ(f.write(table), qint64(table.size()));
        f.close();
    }
    DmiInfo::reloadForTest(dir.path());
}

// 基础表：Type 1（厂商/产品名）+ Type 4（频率）+ Type 127 结束
static QByteArray buildBaseTable(const char *product = "Test-PC")
{
    QByteArray table;
    {
        DmiStructBuilder sys;
        sys.addHeader(1, 0x1B, 0x0001);
        sys.setByte(0x04, 1);
        sys.setByte(0x05, 2);
        sys.addString("TestVendor Inc.");
        sys.addString(product);
        sys.finish();
        table += sys.data;
    }
    {
        DmiStructBuilder cpu;
        cpu.addHeader(4, 0x23, 0x0004);
        cpu.setByte(0x04, 1);
        cpu.setWord(0x10, 3600);  // Max Speed
        cpu.setWord(0x12, 2800);  // Current Speed
        cpu.addString("CPU0");
        cpu.finish();
        table += cpu.data;
    }
    {
        DmiStructBuilder eot;
        eot.addHeader(127, 4, 0x7F00);
        eot.finish();
        table += eot.data;
    }
    return table;
}

// ==================== 测试套件 ====================

class UT_DmiInfo : public ::testing::Test
{
public:
    void TearDown() override
    {
        // 恢复真实 sysfs 环境，避免污染其他套件
        DmiInfo::reloadForTest(QStringLiteral("/sys"));
    }
};

TEST_F(UT_DmiInfo, ParseTable_BaseTable_IsValidAndPopulatesSystemInfo)
{
    // Arrange: Type 1 + Type 4 + Type 127
    loadTable(buildBaseTable());

    // Act
    DmiInfo *dmi = DmiInfo::instance();

    // Assert
    EXPECT_TRUE(dmi->isValid());
    EXPECT_EQ(dmi->systemManufacturer().toStdString(), std::string("TestVendor Inc."));
    EXPECT_EQ(dmi->systemProductName().toStdString(), std::string("Test-PC"));
    EXPECT_EQ(dmi->maxCpuSpeedMHz(), 3600);
    EXPECT_EQ(dmi->currentCpuSpeedMHz(), 2800);
}

TEST_F(UT_DmiInfo, ParseTable_Type4_MultipleProcessors_UsesFirst)
{
    // Arrange: 两个 Type 4 结构，频率不同
    QByteArray table = buildBaseTable();
    {
        DmiStructBuilder cpu2;
        cpu2.addHeader(4, 0x23, 0x0005);
        cpu2.setByte(0x04, 1);
        cpu2.setWord(0x10, 9999);
        cpu2.setWord(0x12, 8888);
        cpu2.addString("CPU1");
        cpu2.finish();
        table += cpu2.data;
    }
    loadTable(table);

    // Act
    DmiInfo *dmi = DmiInfo::instance();

    // Assert: 只取第一个处理器结构
    EXPECT_EQ(dmi->maxCpuSpeedMHz(), 3600);
    EXPECT_EQ(dmi->currentCpuSpeedMHz(), 2800);
}

TEST_F(UT_DmiInfo, ParseTable_Cache16BitFormat_ParsesSizes)
{
    // Arrange: Type 7 len 0x0F（16 位容量字段），Maximum/Installed = 512 KB
    QByteArray table = buildBaseTable();
    {
        DmiStructBuilder l1;
        l1.addHeader(7, 0x0F, 0x0007);
        l1.setByte(0x04, 1);
        l1.setWord(0x07, 512);
        l1.setWord(0x09, 512);
        l1.addString("L1 Cache");
        l1.finish();
        table += l1.data;
    }
    loadTable(table);

    // Act
    const QList<DmiInfo::CacheInfo> caches = DmiInfo::instance()->caches();

    // Assert
    ASSERT_EQ(caches.size(), 1);
    EXPECT_EQ(caches.first().socketDesignation.toStdString(), std::string("L1 Cache"));
    EXPECT_EQ(caches.first().maximumSizeKB, 512u);
    EXPECT_EQ(caches.first().installedSizeKB, 512u);
}

TEST_F(UT_DmiInfo, ParseTable_Cache32BitFormat_ParsesSizes)
{
    // Arrange: Type 7 len 0x1B（SMBIOS 3.1+ 32 位容量字段）
    // bit31 置位表示单位 64KB：0x80000008 -> 8*64 = 512 KB
    QByteArray table = buildBaseTable();
    {
        DmiStructBuilder l2;
        l2.addHeader(7, 0x1B, 0x0008);
        l2.setByte(0x04, 1);
        l2.setDWord(0x13, 0x80000008u);
        l2.setDWord(0x17, 0x80000008u);
        l2.addString("L2 Cache");
        l2.finish();
        table += l2.data;
    }
    {
        DmiStructBuilder l3;
        l3.addHeader(7, 0x1B, 0x0009);
        l3.setByte(0x04, 1);
        l3.setDWord(0x13, 16384u);  // 16 MB
        l3.addString("L3 Cache");
        l3.finish();
        table += l3.data;
    }
    loadTable(table);

    // Act
    const QList<DmiInfo::CacheInfo> caches = DmiInfo::instance()->caches();

    // Assert
    ASSERT_EQ(caches.size(), 2);
    EXPECT_EQ(caches[0].socketDesignation.toStdString(), std::string("L2 Cache"));
    EXPECT_EQ(caches[0].maximumSizeKB, 512u);
    EXPECT_EQ(caches[0].installedSizeKB, 512u);
    EXPECT_EQ(caches[1].socketDesignation.toStdString(), std::string("L3 Cache"));
    EXPECT_EQ(caches[1].maximumSizeKB, 16384u);
    EXPECT_EQ(caches[1].installedSizeKB, 0u);  // len 0x1B 时 Installed 在 0x17，未填充则为 0
}

TEST_F(UT_DmiInfo, ParseTable_OemStrings_IndexedAccess)
{
    // Arrange: Type 11，4 个字符串
    QByteArray table = buildBaseTable();
    {
        DmiStructBuilder oem;
        oem.addHeader(11, 0x05, 0x000B);
        oem.setByte(0x04, 4);
        oem.addString("first");
        oem.addString("second");
        oem.addString("PWC30-TEST");
        oem.addString("PGUX-TEST");
        oem.finish();
        table += oem.data;
    }
    loadTable(table);

    // Act
    DmiInfo *dmi = DmiInfo::instance();

    // Assert: 索引从 1 开始，越界返回空
    EXPECT_EQ(dmi->oemString(1).toStdString(), std::string("first"));
    EXPECT_EQ(dmi->oemString(2).toStdString(), std::string("second"));
    EXPECT_EQ(dmi->oemString(3).toStdString(), std::string("PWC30-TEST"));
    EXPECT_EQ(dmi->oemString(4).toStdString(), std::string("PGUX-TEST"));
    EXPECT_TRUE(dmi->oemString(5).isEmpty());
    EXPECT_TRUE(dmi->oemString(0).isEmpty());
    EXPECT_TRUE(dmi->oemString(-1).isEmpty());
}

TEST_F(UT_DmiInfo, ParseTable_StringIndexZero_ReturnsEmpty)
{
    // Arrange: Type 1 引用字符串索引 0（保留值，表示未指定）
    QByteArray table;
    {
        DmiStructBuilder sys;
        sys.addHeader(1, 0x1B, 0x0001);
        sys.setByte(0x04, 0);
        sys.setByte(0x05, 1);
        sys.addString("OnlyProduct");
        sys.finish();
        table += sys.data;
    }
    loadTable(table);

    // Act
    DmiInfo *dmi = DmiInfo::instance();

    // Assert
    EXPECT_TRUE(dmi->systemManufacturer().isEmpty());
    EXPECT_EQ(dmi->systemProductName().toStdString(), std::string("OnlyProduct"));
}

TEST_F(UT_DmiInfo, ParseTable_NonPrintableChars_ReplacedWithDot)
{
    // Arrange: 产品名含非可打印 ASCII 字节（与 dmidecode 的 ascii_filter 行为一致）
    QByteArray table;
    {
        DmiStructBuilder sys;
        sys.addHeader(1, 0x1B, 0x0001);
        sys.setByte(0x04, 1);
        sys.setByte(0x05, 2);
        sys.addString("Ven\x01""xor");
        QByteArray product = "Prod\x7F""uct";
        sys.data += product;
        sys.data += char(0);
        sys.finish();
        table += sys.data;
    }
    loadTable(table);

    // Act
    DmiInfo *dmi = DmiInfo::instance();

    // Assert
    EXPECT_EQ(dmi->systemManufacturer().toStdString(), std::string("Ven.xor"));
    EXPECT_EQ(dmi->systemProductName().toStdString(), std::string("Prod.uct"));
}

TEST_F(UT_DmiInfo, ParseTable_TruncatedHeader_StopsSafely)
{
    // Arrange: 表尾出现 length < 4 的畸形结构（无法定位下一结构）
    QByteArray table = buildBaseTable();
    table += char(7);
    table += char(2);
    table += char(0);
    table += char(0);
    loadTable(table);

    // Act
    DmiInfo *dmi = DmiInfo::instance();

    // Assert: 解析在畸形结构前正常完成，不崩溃
    EXPECT_TRUE(dmi->isValid());
    EXPECT_EQ(dmi->systemProductName().toStdString(), std::string("Test-PC"));
}

TEST_F(UT_DmiInfo, ParseTable_UnterminatedStringRegion_DoesNotRunPastEnd)
{
    // Arrange: 最后一个结构缺少字符串区结尾的双 NUL（next 越过表尾）
    DmiStructBuilder sys;
    sys.addHeader(1, 0x1B, 0x0001);
    sys.setByte(0x04, 1);
    sys.setByte(0x05, 2);
    sys.addString("TestVendor Inc.");
    sys.addString("NoTerminator");
    // 不调用 finish()：缺少双 NUL
    QByteArray table = sys.data;
    loadTable(table);

    // Act
    DmiInfo *dmi = DmiInfo::instance();

    // Assert: 不越界崩溃，字段按可得数据解析
    EXPECT_TRUE(dmi->isValid());
}

TEST_F(UT_DmiInfo, ParseTable_EmptyTable_IsInvalid)
{
    // Arrange: 空目录（模拟无权限/无表）
    loadTable(QByteArray());

    // Act
    DmiInfo *dmi = DmiInfo::instance();

    // Assert: 全部字段为空/零
    EXPECT_FALSE(dmi->isValid());
    EXPECT_TRUE(dmi->systemProductName().isEmpty());
    EXPECT_TRUE(dmi->systemManufacturer().isEmpty());
    EXPECT_EQ(dmi->maxCpuSpeedMHz(), 0);
    EXPECT_EQ(dmi->currentCpuSpeedMHz(), 0);
    EXPECT_TRUE(dmi->caches().isEmpty());
}

TEST_F(UT_DmiInfo, ParseTable_UnknownTypes_SkippedGracefully)
{
    // Arrange: 仅含未知类型结构（如 Type 126/禁用结构）
    QByteArray table;
    {
        DmiStructBuilder disabled;
        disabled.addHeader(126, 0x08, 0x007E);
        disabled.addString("ignored");
        disabled.finish();
        table += disabled.data;
    }
    loadTable(table);

    // Act
    DmiInfo *dmi = DmiInfo::instance();

    // Assert: 表有效但无目标数据
    EXPECT_TRUE(dmi->isValid());
    EXPECT_TRUE(dmi->systemProductName().isEmpty());
    EXPECT_TRUE(dmi->caches().isEmpty());
}

TEST_F(UT_DmiInfo, Singleton_RepeatedInstance_ReturnsSamePointer)
{
    // Arrange
    DmiInfo *first = DmiInfo::instance();

    // Act
    DmiInfo *second = DmiInfo::instance();

    // Assert
    EXPECT_EQ(first, second);
}

} // namespace
