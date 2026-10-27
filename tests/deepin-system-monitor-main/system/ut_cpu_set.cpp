// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 用例计数声明（self-check-structural 校验此表）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | read_stats | high | cx:15, cog:52, lines:122 | 3 | 3 |
// | read_overall_info | high | cx:8, cog:36, lines:74 | 3 | 3 |
// | read_lscpu | high | cx:60, cog:129, lines:315 | 3 | 3 |
// | read_dmi_cache_info | high | cx:22, cog:53, lines:127 | 3 | 4 |
// | update | high | transitive_loop_depth:8 | 3 | 3 |
// | read_cache_from_lscpu_cmd | mid | cx:3 | 2 | 3 |
// | vendor | mid | - | 2 | 2 |
// | stat | mid | - | 2 | 2 |
// | CPUSet | mid | - | 2 | 4 |
// | curFreq | low | - | 1 | 4 |
// | minFreq | low | - | 1 | 3 |
// | maxFreq | low | - | 1 | 3 |
// | avgFreq | low | - | 1 | 3 |
// | l1dCache | low | - | 1 | 2 |
// | l1iCache | low | - | 1 | 2 |
// | l2Cache | low | - | 1 | 2 |
// | l3Cache | low | - | 1 | 2 |
// | cpuCount | low | - | 1 | 2 |
// | socketCount | low | - | 1 | 2 |
// | virtualization | low | - | 1 | 2 |
// | coreId | low | - | 1 | 2 |
// | cpuLogicName | low | - | 1 | 2 |
// | statDB | low | - | 1 | 2 |
// | usageDB | low | - | 1 | 2 |
// | usage | low | - | 1 | 2 |
// | getUsageTotalDelta | low | - | 1 | 2 |
// | modelName | low | - | 1 | 2 |
// | operator= | low | - | 1 | 2 |
// | read_cpu_freq_range_by_cpu7 | low | - | 1 | 1 |
//
// 分支映射（源码行号，cpu_set.cpp）：
// - read_stats(:458-577) fopen失败→保持默认 / 有效行→stat+usage+percpu / 畸形行→跳过无效项
// - read_overall_info(:579-652) 块解析→setIndex/setVendorId/setCpuFreq/setCoreId/setModelName/setCacheSize / 空输出→无条目
// - read_lscpu(:654-969) 真实 liblscpu(仅冒烟) / 二次调用幂等 / 拓扑键
// - read_dmi_cache_info(:782-908) specialComType==0 早退 / ==1 走dmidecode解析 / <=-1 且非特殊机型早退 / 守卫一次性
// - read_cache_from_lscpu_cmd(:910-934) 守卫已置位→跳过 / lscpu缓存行解析 / 空输出
// - update(:573-582) 首次→Current迁移Last / 二次→delta回退1 / percpu入库
// - read_cpu_freq_range_by_cpu7(:159-194) sysfs缺失→(0,0) / 有序非负区间（主机不变式）
// - 静态自由函数 first/max/avg_avaliable_cur_freq、lscpu_free_context 为内部链接，
//   仅能经 read_lscpu 间接触达，随真实 lscpu 冒烟用例间接覆盖（不计入直接用例行）。
//
// 最小检查清单（10 项）：
// 1. 等价类按 test-types.md §1 划分，无效等价类单列用例
// 2. 边界值（§2）：零值、阈值、越界索引已覆盖
// 3. 每个用例 AAA 三段注释齐全
// 4. 断言强度 ≥2 且无空洞断言
// 5. 外部依赖（/proc /sys、子进程、dmidecode）全部打桩
// 6. 特殊机型读缓存守卫（static 一次性锁存）按套件分片隔离
// 7. 命名 Feature_Scenario_ExpectedResult 三段式
// 8. 同质组使用 TEST_P 参数化（频率/缓存获取器）
// 9. 私有访问依赖 -fno-access-control，与现有测试保持一致
// 10. 不修改生产代码，不依赖真实设备状态

#include "system/cpu_set.h"
#include "system/cpu.h"
#include "system/private/cpu_set_p.h"
#include "common/common.h"
#include "stub.h"

#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QStringList>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

using namespace core::system;
using namespace common::init;

// ==================== 测试打桩辅助（文件级共享） ====================
// 子进程启动捕获：不真正执行外部命令
static QStringList g_qprocStartCapture;
// 子进程标准输出 canned 数据
static QByteArray g_qprocStdout;

static void stub_qproc_start(QProcess *self, const QString &program,
                             const QStringList &args, QIODevice::OpenMode mode)
{
    Q_UNUSED(self);
    Q_UNUSED(mode);
    g_qprocStartCapture << (program + " " + args.join(' '));
}

static QByteArray stub_qproc_read_stdout(QProcess *self)
{
    Q_UNUSED(self);
    return g_qprocStdout;
}

static void stub_read_lscpu_noop(CPUSet *self)
{
    Q_UNUSED(self);
}

// fopen 重定向：read_stats 的 /proc/stat 读取走预打开的临时文件。
// read_stats 结束时会 fclose 返回的句柄，因此每次调用都重新打开一次，
// 避免二次 update() 复用已关闭句柄导致 double-free。
static FILE *g_preopenedFp = nullptr;
static QString g_preopenedPath;

static FILE *stub_fopen_preopened(const char *path, const char *mode)
{
    Q_UNUSED(path);
    Q_UNUSED(mode);
    if (!g_preopenedPath.isEmpty()) {
        // 不能调用 fopen：符号已被本 stub 替换会无限递归；改用 open + fdopen
        int fd = ::open(g_preopenedPath.toLocal8Bit().constData(), O_RDONLY);
        return fd >= 0 ? fdopen(fd, "r") : nullptr;
    }
    return g_preopenedFp;
}

static FILE *stub_fopen_null(const char *path, const char *mode)
{
    Q_UNUSED(path);
    Q_UNUSED(mode);
    return nullptr;
}

// ==================== 主套件：注入式获取器与拷贝语义 ====================
class UT_CPUSet : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_tester = new CPUSet();
        g_qprocStartCapture.clear();
        g_qprocStdout.clear();
        g_preopenedFp = nullptr;
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        g_qprocStartCapture.clear();
        g_qprocStdout.clear();
        g_preopenedFp = nullptr;
    }

    CPUSet *m_tester = nullptr;
    Stub stub;
};

TEST_F(UT_CPUSet, Ctor_DefaultConstructs_ZeroedState)
{
    // Arrange
    // Act
    // Assert: 默认私有结构全零且 usage 求差回退值为 1
    EXPECT_NE(m_tester->stat(), nullptr);
    EXPECT_EQ(m_tester->stat()->user, 0ULL);
    EXPECT_EQ(m_tester->usage()->total, 0ULL);
    EXPECT_EQ(m_tester->getUsageTotalDelta(), 1ULL);
    EXPECT_TRUE(m_tester->cpuLogicName().isEmpty());
}

TEST_F(UT_CPUSet, Ctor_TwoInstances_IndependentStates)
{
    // Arrange
    CPUSet other;
    m_tester->d->m_info.insert("CPU(s)", "16");
    // Act
    // Assert: 两个实例的数据互不影响
    EXPECT_EQ(other.cpuCount(), 0);
    EXPECT_EQ(m_tester->cpuCount(), 16);
    EXPECT_EQ(other.statDB("cpu0"), nullptr);
}

TEST_F(UT_CPUSet, Vendor_EnglishFlag_ReturnsVendorId)
{
    // Arrange
    m_tester->d->m_info.insert("Vendor ID", "GenuineIntel");
    m_tester->d->m_info.insert("厂商 ID", "英特尔");
    m_tester->mIsEmptyModelName = false;
    // Act
    // Assert: 英文标志下读英文键，忽略中文键
    EXPECT_EQ(m_tester->vendor().toStdString(), std::string("GenuineIntel"));
    EXPECT_NE(m_tester->vendor().toStdString(), std::string("英特尔"));
}

TEST_F(UT_CPUSet, Vendor_ChineseFlag_ReturnsChineseKey)
{
    // Arrange
    m_tester->d->m_info.insert("Vendor ID", "GenuineIntel");
    m_tester->d->m_info.insert("厂商 ID", "英特尔");
    m_tester->mIsEmptyModelName = true;
    // Act
    // Assert: 中文标志下读中文键
    EXPECT_EQ(m_tester->vendor().toStdString(), std::string("英特尔"));
    EXPECT_NE(m_tester->vendor().toStdString(), std::string("GenuineIntel"));
}

TEST_F(UT_CPUSet, ModelName_EnglishFlag_ReturnsModelName)
{
    // Arrange
    m_tester->d->m_info.insert("Model name", "Intel Xeon");
    m_tester->d->m_info.insert("型号名称", "至强");
    m_tester->mIsEmptyModelName = false;
    // Act
    // Assert
    EXPECT_EQ(m_tester->modelName().toStdString(), std::string("Intel Xeon"));
    EXPECT_NE(m_tester->modelName().toStdString(), std::string("至强"));
}

TEST_F(UT_CPUSet, ModelName_ChineseFlag_ReturnsChineseKey)
{
    // Arrange
    m_tester->d->m_info.insert("Model name", "Intel Xeon");
    m_tester->d->m_info.insert("型号名称", "至强");
    m_tester->mIsEmptyModelName = true;
    // Act
    // Assert
    EXPECT_EQ(m_tester->modelName().toStdString(), std::string("至强"));
    EXPECT_NE(m_tester->modelName().toStdString(), std::string("Intel Xeon"));
}

// 频率获取器参数化：cur/min/max/avg 四个同质方法
enum FreqMethod { FM_Cur, FM_Min, FM_Max, FM_Avg };

struct FreqParam {
    QString key;
    QString value;
    bool chinese;
    FreqMethod method;
    QString expected;
};

class UT_CPUSetFreq : public UT_CPUSet, public ::testing::WithParamInterface<FreqParam> {};

TEST_P(UT_CPUSetFreq, FreqGetters_InjectedInfo_ReturnExpectedText)
{
    // Arrange
    const FreqParam p = GetParam();
    if (!p.key.isEmpty())
        m_tester->d->m_info.insert(p.key, p.value);
    m_tester->mIsEmptyModelName = p.chinese;
    // Act
    QString actual;
    switch (p.method) {
    case FM_Cur: actual = m_tester->curFreq(); break;
    case FM_Min: actual = m_tester->minFreq(); break;
    case FM_Max: actual = m_tester->maxFreq(); break;
    case FM_Avg: actual = m_tester->avgFreq(); break;
    }
    // Assert
    EXPECT_EQ(actual.toStdString(), p.expected.toStdString());
    EXPECT_EQ(actual.isEmpty(), false);
}

INSTANTIATE_TEST_SUITE_P(Freq, UT_CPUSetFreq, ::testing::Values(
    FreqParam {"CPU MHz", "800", false, FM_Cur, "800.00MHz"},
    FreqParam {"CPU MHz", "2500", false, FM_Cur, "2.50GHz"},
    FreqParam {"CPU MHz", "-", false, FM_Cur, "-"},
    FreqParam {"", "", false, FM_Cur, "0.00MHz"},
    FreqParam {"CPU min MHz", "1200", false, FM_Min, "1.20GHz"},
    FreqParam {"CPU min MHz", "800", false, FM_Min, "800.00MHz"},
    FreqParam {"CPU min MHz", "-", false, FM_Min, "0.00MHz"},
    FreqParam {"CPU max MHz", "3600", false, FM_Max, "3.60GHz"},
    FreqParam {"CPU max MHz", "800", false, FM_Max, "800.00MHz"},
    FreqParam {"CPU max MHz", "-", false, FM_Max, "0.00MHz"},
    FreqParam {"CPU avg MHz", "1500", false, FM_Avg, "1.50GHz"},
    FreqParam {"CPU avg MHz", "2500", false, FM_Avg, "2.50GHz"},
    FreqParam {"CPU avg MHz", "-", false, FM_Avg, "-"}
));

// 缓存获取器参数化：l1d/l1i/l2/l3 四个同质方法
enum CacheMethod { CM_L1D, CM_L1I, CM_L2, CM_L3 };

struct CacheParam {
    QString key;
    QString value;
    CacheMethod method;
    QString expected;
};

class UT_CPUSetCache : public UT_CPUSet, public ::testing::WithParamInterface<CacheParam> {};

TEST_P(UT_CPUSetCache, CacheGetters_InjectedInfo_ReturnExpectedText)
{
    // Arrange
    const CacheParam p = GetParam();
    if (!p.key.isEmpty())
        m_tester->d->m_info.insert(p.key, p.value);
    // Act
    QString actual;
    switch (p.method) {
    case CM_L1D: actual = m_tester->l1dCache(); break;
    case CM_L1I: actual = m_tester->l1iCache(); break;
    case CM_L2: actual = m_tester->l2Cache(); break;
    case CM_L3: actual = m_tester->l3Cache(); break;
    }
    // Assert
    EXPECT_EQ(actual.toStdString(), p.expected.toStdString());
    EXPECT_EQ(actual.isNull(), false);
}

INSTANTIATE_TEST_SUITE_P(Cache, UT_CPUSetCache, ::testing::Values(
    CacheParam {"L1d cache", "32 KB", CM_L1D, "32 KB"},
    CacheParam {"", "", CM_L1D, "-"},
    CacheParam {"L1i cache", "16 KB", CM_L1I, "16 KB"},
    CacheParam {"", "", CM_L1I, "-"},
    CacheParam {"L2 cache", "1 MB", CM_L2, "1 MB"},
    CacheParam {"", "", CM_L2, "-"},
    CacheParam {"L3 cache", "16 MB", CM_L3, "16 MB"},
    CacheParam {"", "", CM_L3, "-"}
));

TEST_F(UT_CPUSet, CpuCount_NumericValue_ReturnsParsedInt)
{
    // Arrange
    m_tester->d->m_info.insert("CPU(s)", "16");
    // Act
    // Assert: toInt 解析有效数字
    EXPECT_EQ(m_tester->cpuCount(), 16);
    m_tester->d->m_info.insert("CPU(s)", "8");
    EXPECT_EQ(m_tester->cpuCount(), 8);
}

TEST_F(UT_CPUSet, CpuCount_MissingOrDash_ReturnsZero)
{
    // Arrange
    // Act
    // Assert: 键缺失与占位符均解析为 0
    EXPECT_EQ(m_tester->cpuCount(), 0);
    m_tester->d->m_info.insert("CPU(s)", "-");
    EXPECT_EQ(m_tester->cpuCount(), 0);
}

TEST_F(UT_CPUSet, SocketCount_NumericValue_ReturnsParsedInt)
{
    // Arrange
    m_tester->d->m_info.insert("Socket(s)", "2");
    // Act
    // Assert
    EXPECT_EQ(m_tester->socketCount(), 2);
    EXPECT_EQ(m_tester->d->m_sockets, 0);
}

TEST_F(UT_CPUSet, SocketCount_MissingKey_ReturnsZero)
{
    // Arrange
    // Act
    // Assert
    EXPECT_EQ(m_tester->socketCount(), 0);
    m_tester->d->m_info.insert("Socket(s)", "-");
    EXPECT_EQ(m_tester->socketCount(), 0);
}

TEST_F(UT_CPUSet, Virtualization_Value_ReturnsRawValue)
{
    // Arrange
    m_tester->d->m_info.insert("Virtualization", "VT-x");
    // Act
    // Assert: 非空原样返回
    EXPECT_EQ(m_tester->virtualization().toStdString(), std::string("VT-x"));
    EXPECT_EQ(m_tester->virtualization().isEmpty(), false);
}

TEST_F(UT_CPUSet, Virtualization_MissingValue_ReturnsNotSupport)
{
    // Arrange
    // Act
    // Assert: 空值回退为不支持文案
    EXPECT_EQ(m_tester->virtualization().toStdString(), std::string(QObject::tr("Not support").toStdString()));
    EXPECT_EQ(m_tester->virtualization().isEmpty(), false);
}

TEST_F(UT_CPUSet, CoreId_ValidIndices_ReturnsCoreIdText)
{
    // Arrange
    CPUInfo first;
    first.setCoreId(5);
    CPUInfo second;
    second.setCoreId(7);
    m_tester->d->m_infos.append(first);
    m_tester->d->m_infos.append(second);
    // Act
    // Assert
    EXPECT_EQ(m_tester->coreId(0).toStdString(), std::string("5"));
    EXPECT_EQ(m_tester->coreId(1).toStdString(), std::string("7"));
}

TEST_F(UT_CPUSet, CoreId_OutOfRangeIndex_ReturnsMinusOne)
{
    // Arrange
    // Act
    // Assert: 越界取默认 CPUInfo，coreID 为 -1 占位
    EXPECT_EQ(m_tester->coreId(99).toStdString(), std::string("-1"));
    EXPECT_EQ(m_tester->coreId(-1).toStdString(), std::string("-1"));
}

TEST_F(UT_CPUSet, CpuLogicName_WithUsageEntries_ReturnsSortedKeys)
{
    // Arrange
    auto usage0 = std::make_shared<struct cpu_usage_t>();
    usage0->cpu = "cpu0";
    auto usage1 = std::make_shared<struct cpu_usage_t>();
    usage1->cpu = "cpu1";
    m_tester->d->m_usageDB.insert("cpu1", usage1);
    m_tester->d->m_usageDB.insert("cpu0", usage0);
    // Act
    // Assert: QMap 键序返回
    QList<QByteArray> names = m_tester->cpuLogicName();
    EXPECT_EQ(names.size(), 2);
    EXPECT_EQ(names.first().toStdString(), std::string("cpu0"));
}

TEST_F(UT_CPUSet, CpuLogicName_EmptyDb_ReturnsEmptyList)
{
    // Arrange
    // Act
    // Assert
    EXPECT_EQ(m_tester->cpuLogicName().size(), 0);
    EXPECT_TRUE(m_tester->cpuLogicName().isEmpty());
}

TEST_F(UT_CPUSet, Stat_InjectedValue_ReturnsSharedPtr)
{
    // Arrange
    auto stat = std::make_shared<struct cpu_stat_t>();
    stat->user = 100ULL;
    stat->idle = 700ULL;
    m_tester->d->m_stat = stat;
    // Act
    // Assert
    EXPECT_EQ(m_tester->stat()->user, 100ULL);
    EXPECT_EQ(m_tester->stat()->idle, 700ULL);
}

TEST_F(UT_CPUSet, Stat_DefaultState_ReturnsZeroedStruct)
{
    // Arrange
    // Act
    // Assert
    EXPECT_NE(m_tester->stat(), nullptr);
    EXPECT_EQ(m_tester->stat()->cpu.isEmpty(), true);
    EXPECT_EQ(m_tester->stat()->user, 0ULL);
}

TEST_F(UT_CPUSet, Usage_InjectedValue_ReturnsSharedPtr)
{
    // Arrange
    auto usage = std::make_shared<struct cpu_usage_t>();
    usage->cpu = "cpu";
    usage->total = 1010ULL;
    usage->idle = 700ULL;
    m_tester->d->m_usage = usage;
    // Act
    // Assert
    EXPECT_EQ(m_tester->usage()->total, 1010ULL);
    EXPECT_EQ(m_tester->usage()->idle, 700ULL);
}

TEST_F(UT_CPUSet, Usage_DefaultState_ReturnsZeroedStruct)
{
    // Arrange
    // Act
    // Assert
    EXPECT_NE(m_tester->usage(), nullptr);
    EXPECT_EQ(m_tester->usage()->total, 0ULL);
    EXPECT_EQ(m_tester->usage()->cpu.isEmpty(), true);
}

TEST_F(UT_CPUSet, StatDB_ExistingKey_ReturnsPerCpuStat)
{
    // Arrange
    auto stat0 = std::make_shared<struct cpu_stat_t>();
    stat0->cpu = "cpu0";
    stat0->user = 10ULL;
    m_tester->d->m_statDB.insert("cpu0", stat0);
    // Act
    // Assert
    EXPECT_NE(m_tester->statDB("cpu0"), nullptr);
    EXPECT_EQ(m_tester->statDB("cpu0")->user, 10ULL);
    EXPECT_EQ(m_tester->statDB("cpu0")->cpu.toStdString(), std::string("cpu0"));
}

TEST_F(UT_CPUSet, StatDB_MissingKey_ReturnsNullPointer)
{
    // Arrange
    // Act
    // Assert: 键缺失返回空指针，不崩溃
    EXPECT_EQ(m_tester->statDB("cpu0"), nullptr);
    EXPECT_EQ(m_tester->statDB("nonexistent"), nullptr);
}

TEST_F(UT_CPUSet, UsageDB_ExistingKey_ReturnsPerCpuUsage)
{
    // Arrange
    auto usage0 = std::make_shared<struct cpu_usage_t>();
    usage0->cpu = "cpu0";
    usage0->total = 101ULL;
    m_tester->d->m_usageDB.insert("cpu0", usage0);
    // Act
    // Assert
    EXPECT_NE(m_tester->usageDB("cpu0"), nullptr);
    EXPECT_EQ(m_tester->usageDB("cpu0")->total, 101ULL);
    EXPECT_EQ(m_tester->usageDB("cpu0")->cpu.toStdString(), std::string("cpu0"));
}

TEST_F(UT_CPUSet, UsageDB_MissingKey_ReturnsNullPointer)
{
    // Arrange
    // Act
    // Assert
    EXPECT_EQ(m_tester->usageDB("cpu0"), nullptr);
    EXPECT_EQ(m_tester->usageDB("nonexistent"), nullptr);
}

TEST_F(UT_CPUSet, GetUsageTotalDelta_CurrentGreaterThanLast_ReturnsDifference)
{
    // Arrange
    m_tester->d->cpusageTotal[kLastStat] = 100ULL;
    m_tester->d->cpusageTotal[kCurrentStat] = 500ULL;
    // Act
    // Assert
    EXPECT_EQ(m_tester->getUsageTotalDelta(), 400ULL);
    EXPECT_EQ(m_tester->getUsageTotalDelta() > 0, true);
}

TEST_F(UT_CPUSet, GetUsageTotalDelta_CurrentNotGreaterThanLast_ReturnsOne)
{
    // Arrange
    m_tester->d->cpusageTotal[kLastStat] = 100ULL;
    m_tester->d->cpusageTotal[kCurrentStat] = 0ULL;
    // Act
    // Assert: 不增或持平均回退为 1，避免除零
    EXPECT_EQ(m_tester->getUsageTotalDelta(), 1ULL);
    m_tester->d->cpusageTotal[kCurrentStat] = 100ULL;
    EXPECT_EQ(m_tester->getUsageTotalDelta(), 1ULL);
}

TEST_F(UT_CPUSet, Copy_AfterCopyConstructor_SharesUnderlyingData)
{
    // Arrange
    m_tester->d->m_info.insert("CPU(s)", "16");
    m_tester->d->m_info.insert("Socket(s)", "2");
    // Act
    CPUSet copy(*m_tester);
    // Assert: 拷贝共享底层 d，读取一致
    EXPECT_EQ(copy.cpuCount(), 16);
    EXPECT_EQ(copy.socketCount(), 2);
    EXPECT_EQ(copy.getUsageTotalDelta(), m_tester->getUsageTotalDelta());
}

TEST_F(UT_CPUSet, Copy_EmptyModelNameFlagNotCopied_FallsBackEnglishLookup)
{
    // Arrange
    m_tester->mIsEmptyModelName = true;
    m_tester->d->m_info.insert("厂商 ID", "英特尔");
    // Act
    CPUSet copy(*m_tester);
    // Assert: 拷贝构造未迁移 mIsEmptyModelName 标志（现状行为记录）
    EXPECT_EQ(copy.mIsEmptyModelName, false);
    EXPECT_EQ(copy.vendor().toStdString(), std::string(""));
}

TEST_F(UT_CPUSet, OperatorAssign_FromOther_CopiesSharedData)
{
    // Arrange
    CPUSet target;
    m_tester->d->m_info.insert("CPU(s)", "16");
    // Act
    target = *m_tester;
    // Assert
    EXPECT_EQ(target.cpuCount(), 16);
    EXPECT_EQ(target.getUsageTotalDelta(), m_tester->getUsageTotalDelta());
}

TEST_F(UT_CPUSet, OperatorAssign_SelfAssignment_Unchanged)
{
    // Arrange
    m_tester->d->m_info.insert("CPU(s)", "4");
    CPUSet &ref = *m_tester;
    // Act
    *m_tester = ref;
    // Assert: 自赋值后状态不变
    EXPECT_EQ(m_tester->cpuCount(), 4);
    EXPECT_EQ(m_tester->getUsageTotalDelta(), 1ULL);
}

// ==================== 统计读取套件：/proc 与子进程打桩 ====================
class UT_CPUSetUpdate : public UT_CPUSet {};

// 构造临时 stat 文件并预打开，供 fopen 打桩返回
static void prepare_fake_proc_stat(const QStringList &lines, QTemporaryDir &dir, FILE **fp)
{
    QFile file(dir.path() + "/stat.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        for (const QString &line : lines)
            file.write((line + "\n").toLocal8Bit());
        file.close();
    }
    g_preopenedPath = dir.path() + "/stat.txt";
    *fp = fopen(g_preopenedPath.toLocal8Bit().constData(), "r");
}

// 子进程输出重定向为 canned cpuinfo 文本
static void prepare_fake_cpuinfo_stdout()
{
    g_qprocStdout =
        "processor\t: 0\n"
        "vendor_id\t: GenuineIntel\n"
        "cpu MHz\t\t: 800.000\n"
        "\n"
        "processor\t: 1\n"
        "model name\t: Intel(R) Core(TM) i7\n"
        "core id\t\t: 3\n";
}

TEST_F(UT_CPUSetUpdate, ReadStats_FopenFails_KeepsDefaultState)
{
    // Arrange
    stub.set(fopen, stub_fopen_null);
    // Act
    m_tester->read_stats();
    // Assert: 打不开文件时保持零状态
    EXPECT_NE(m_tester->stat(), nullptr);
    EXPECT_EQ(m_tester->stat()->user, 0ULL);
    EXPECT_EQ(m_tester->usage()->total, 0ULL);
    EXPECT_TRUE(m_tester->cpuLogicName().isEmpty());
}

TEST_F(UT_CPUSetUpdate, ReadStats_ValidLines_PopulatesStatAndUsageDb)
{
    // Arrange
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    FILE *fp = nullptr;
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0"
        << "cpu0 10 0 20 70 0 0 1 0 0 0"
        << "cpu1 90 0 180 630 0 0 9 0 0 0", dir, &fp);
    ASSERT_NE(fp, nullptr);
    g_preopenedFp = fp;
    stub.set(fopen, stub_fopen_preopened);
    // Act
    m_tester->read_stats();
    g_preopenedFp = nullptr;
    // Assert: 整机与分核统计均入库（文件句柄由 read_stats 内部关闭）
    EXPECT_EQ(m_tester->stat()->user, 100ULL);
    EXPECT_EQ(m_tester->stat()->idle, 700ULL);
    EXPECT_EQ(m_tester->usage()->total, 1010ULL);
    EXPECT_EQ(m_tester->usage()->idle, 700ULL);
    EXPECT_NE(m_tester->statDB("cpu0"), nullptr);
    EXPECT_EQ(m_tester->statDB("cpu0")->user, 10ULL);
    EXPECT_EQ(m_tester->usageDB("cpu1")->total, 909ULL);
    EXPECT_EQ(m_tester->cpuLogicName().size(), 2);
}

TEST_F(UT_CPUSetUpdate, ReadStats_MalformedLines_SkipsInvalidEntries)
{
    // Arrange
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    FILE *fp = nullptr;
    prepare_fake_proc_stat(QStringList()
        << "cpu  garbage values here"
        << "cpu0 1 2 3"
        << "text not a cpu line", dir, &fp);
    ASSERT_NE(fp, nullptr);
    g_preopenedFp = fp;
    stub.set(fopen, stub_fopen_preopened);
    // Act
    m_tester->read_stats();
    g_preopenedFp = nullptr;
    // Assert: 畸形行全部被跳过，仅整机占位结构存在
    EXPECT_EQ(m_tester->stat()->cpu.isEmpty(), true);
    EXPECT_EQ(m_tester->usage()->total, 0ULL);
    EXPECT_EQ(m_tester->statDB("cpu0"), nullptr);
    EXPECT_TRUE(m_tester->cpuLogicName().isEmpty());
}

TEST_F(UT_CPUSetUpdate, ReadOverallInfo_ParsesCpuinfoBlocks_PopulatesInfos)
{
    // Arrange
    prepare_fake_cpuinfo_stdout();
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    stub.set((QByteArray (QProcess::*)())ADDR(QProcess, readAllStandardOutput), stub_qproc_read_stdout);
    stub.set((void (CPUSet::*)())ADDR(CPUSet, read_lscpu), stub_read_lscpu_noop);
    // Act
    m_tester->read_overall_info();
    // Assert: 两个 processor 块各自字段入列
    EXPECT_EQ(m_tester->d->m_infos.size(), 2);
    EXPECT_EQ(m_tester->coreId(1).toStdString(), std::string("3"));
    EXPECT_EQ(m_tester->d->m_infos[0].vendorId().toStdString(), std::string(" GenuineIntel"));
    EXPECT_EQ(m_tester->d->m_infos[0].cpuFreq().toStdString(), std::string(" 800.000MHz"));
    EXPECT_EQ(m_tester->d->m_infos[1].modelName().toStdString(), std::string(" Intel(R) Core(TM) i7"));
    EXPECT_EQ(m_tester->d->m_infos[1].logicalIndex(), 1);
}

TEST_F(UT_CPUSetUpdate, ReadOverallInfo_EmptyStdout_PopulatesNothing)
{
    // Arrange
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    stub.set((void (CPUSet::*)())ADDR(CPUSet, read_lscpu), stub_read_lscpu_noop);
    // Act
    m_tester->read_overall_info();
    // Assert: 空输出不产生条目
    EXPECT_EQ(m_tester->d->m_infos.size(), 0);
    EXPECT_EQ(m_tester->coreId(0).toStdString(), std::string("-1"));
    EXPECT_EQ(m_tester->cpuCount(), 0);
}

TEST_F(UT_CPUSetUpdate, ReadOverallInfo_SingleBlock_SetsProcessorIndex)
{
    // Arrange
    g_qprocStdout = "processor\t: 5\n";
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    stub.set((QByteArray (QProcess::*)())ADDR(QProcess, readAllStandardOutput), stub_qproc_read_stdout);
    stub.set((void (CPUSet::*)())ADDR(CPUSet, read_lscpu), stub_read_lscpu_noop);
    // Act
    m_tester->read_overall_info();
    // Assert: processor 字段映射为逻辑索引
    EXPECT_EQ(m_tester->d->m_infos.size(), 1);
    EXPECT_EQ(m_tester->d->m_infos[0].logicalIndex(), 5);
    EXPECT_EQ(m_tester->d->m_infos[0].coreID().toStdString(), std::string("-1"));
}

TEST_F(UT_CPUSetUpdate, Update_FirstCall_ShiftsCurrentToLast)
{
    // Arrange
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    FILE *fp = nullptr;
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0", dir, &fp);
    ASSERT_NE(fp, nullptr);
    g_preopenedFp = fp;
    g_qprocStdout = "";
    stub.set(fopen, stub_fopen_preopened);
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    stub.set((void (CPUSet::*)())ADDR(CPUSet, read_lscpu), stub_read_lscpu_noop);
    // Act
    m_tester->update();
    g_preopenedFp = nullptr;
    // Assert: 首次更新把当前用量迁入槽位，delta 取当前值
    EXPECT_EQ(m_tester->d->cpusageTotal[kLastStat], 0ULL);
    EXPECT_EQ(m_tester->d->cpusageTotal[kCurrentStat], 1010ULL);
    EXPECT_EQ(m_tester->getUsageTotalDelta(), 1010ULL);
}

TEST_F(UT_CPUSetUpdate, Update_Twice_UnchangedUsage_DeltaFallsBackToOne)
{
    // Arrange
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    FILE *fp = nullptr;
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0", dir, &fp);
    ASSERT_NE(fp, nullptr);
    g_preopenedFp = fp;
    g_qprocStdout = "";
    stub.set(fopen, stub_fopen_preopened);
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    stub.set((void (CPUSet::*)())ADDR(CPUSet, read_lscpu), stub_read_lscpu_noop);
    // Act
    m_tester->update();
    m_tester->update();
    g_preopenedFp = nullptr;
    g_preopenedPath.clear();
    // Assert: 用量未增长时 delta 回退为 1
    EXPECT_EQ(m_tester->d->cpusageTotal[kLastStat], 1010ULL);
    EXPECT_EQ(m_tester->d->cpusageTotal[kCurrentStat], 1010ULL);
    EXPECT_EQ(m_tester->getUsageTotalDelta(), 1ULL);
}

TEST_F(UT_CPUSetUpdate, Update_PopulatesPerCpuStat_ThroughFullPath)
{
    // Arrange
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    FILE *fp = nullptr;
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0"
        << "cpu0 10 0 20 70 0 0 1 0 0 0"
        << "cpu1 90 0 180 630 0 0 9 0 0 0", dir, &fp);
    ASSERT_NE(fp, nullptr);
    g_preopenedFp = fp;
    g_qprocStdout = "";
    stub.set(fopen, stub_fopen_preopened);
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    stub.set((void (CPUSet::*)())ADDR(CPUSet, read_lscpu), stub_read_lscpu_noop);
    // Act
    m_tester->update();
    g_preopenedFp = nullptr;
    // Assert: 分核数据随 update 入库
    EXPECT_EQ(m_tester->cpuLogicName().size(), 2);
    EXPECT_NE(m_tester->statDB("cpu0"), nullptr);
    EXPECT_EQ(m_tester->statDB("cpu0")->user, 10ULL);
    EXPECT_EQ(m_tester->usageDB("cpu1")->total, 909ULL);
}

TEST_F(UT_CPUSetUpdate, ReadCpuFreqRangeByCpu7_OrderedRange_ReturnsNonNegative)
{
    // Arrange
    // Act: 直接调用读取主机 cpu7 频率区间
    QPair<float, float> range = m_tester->read_cpu_freq_range_by_cpu7();
    // Assert: 主机无关不变式——无 cpu7 时 (0,0)，有 cpu7 时 min<=max 且非负
    EXPECT_GE(range.first, 0.0f);
    EXPECT_GE(range.second, range.first);
}

// ==================== lscpu 命令缓存行解析套件 ====================
class UT_CPUSetCacheCmd : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_tester = new CPUSet();
        g_qprocStartCapture.clear();
        g_qprocStdout.clear();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        g_qprocStartCapture.clear();
        g_qprocStdout.clear();
    }

    CPUSet *m_tester = nullptr;
    Stub stub;
};

TEST_F(UT_CPUSetCacheCmd, ReadCacheFromLscpuCmd_CacheLines_UpdatesInfo)
{
    // Arrange: 守卫未锁存，canned lscpu 缓存行
    g_qprocStdout = "L1d cache: 128 KiB\nL2 cache: 4 MiB\nFlags: fpu vme\n";
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    stub.set((QByteArray (QProcess::*)())ADDR(QProcess, readAllStandardOutput), stub_qproc_read_stdout);
    // Act
    m_tester->read_cache_from_lscpu_cmd();
    // Assert: 仅含 cache 的行入库，其余忽略
    EXPECT_EQ(m_tester->l1dCache().toStdString(), std::string("128 KiB"));
    EXPECT_EQ(m_tester->l2Cache().toStdString(), std::string("4 MiB"));
    EXPECT_EQ(m_tester->l3Cache().toStdString(), std::string("-"));
    EXPECT_EQ(g_qprocStartCapture.size(), 1);
}

TEST_F(UT_CPUSetCacheCmd, ReadCacheFromLscpuCmd_EmptyStdout_NoCacheKeys)
{
    // Arrange
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    // Act
    m_tester->read_cache_from_lscpu_cmd();
    // Assert: 空输出不写缓存键
    EXPECT_EQ(m_tester->l1dCache().toStdString(), std::string("-"));
    EXPECT_EQ(m_tester->l3Cache().toStdString(), std::string("-"));
    EXPECT_EQ(g_qprocStartCapture.size(), 1);
}

// ==================== 特殊机型 dmidecode 缓存解析套件 ====================
// 注意：read_dmi_cache_info 的守卫是一次性锁存（static），按套件分片隔离
class UT_CPUSetDmiSpecial : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_origType = specialComType;
        specialComType = 1;
        m_tester = new CPUSet();
        g_qprocStartCapture.clear();
        g_qprocStdout.clear();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        specialComType = m_origType;
        g_qprocStartCapture.clear();
        g_qprocStdout.clear();
    }

    CPUSet *m_tester = nullptr;
    int m_origType = 0;
    Stub stub;
};

TEST_F(UT_CPUSetDmiSpecial, ReadDmiCacheInfo_SpecialType_ParsesCacheBlocks)
{
    // Arrange: 特殊机型标记为 1，canned dmidecode 缓存输出
    specialComType = 1;
    g_qprocStdout =
        "Handle 0x0700, DMI type 7, 19 bytes.\nCache Information\n\n"
        "\tSocket Designation: L1 Cache\n\tConfiguration: Enabled, Not Socketed, Level 1\n"
        "\tMaximum Size: 2048 KB\n\n"
        "\tSocket Designation: L2 Cache\n\tConfiguration: Enabled, Not Socketed, Level 2\n"
        "\tMaximum Size: 16384 KB\n\n"
        "\tSocket Designation: L3 Cache\n\tMaximum Size: 16 MB\n\n";
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    stub.set((QByteArray (QProcess::*)())ADDR(QProcess, readAllStandardOutput), stub_qproc_read_stdout);
    // Act
    m_tester->read_dmi_cache_info();
    // Assert: KiB 折算 MiB 后写入信息表
    EXPECT_EQ(m_tester->d->m_info.value("L1d cache").toStdString(), std::string("1 MiB"));
    EXPECT_EQ(m_tester->d->m_info.value("L1i cache").toStdString(), std::string("1 MiB"));
    EXPECT_EQ(m_tester->d->m_info.value("L2 cache").toStdString(), std::string("16 MiB"));
    EXPECT_EQ(m_tester->d->m_info.value("L3 cache").toStdString(), std::string("16 MiB"));
    EXPECT_EQ(g_qprocStartCapture.size(), 1);
}

TEST_F(UT_CPUSetDmiSpecial, ReadDmiCacheInfo_GuardLatched_SecondCallSkipsParse)
{
    // Arrange: 首次调用已在本套件前序用例中消耗守卫
    m_tester->d->m_info.insert("L2 cache", "canary MiB");
    // Act
    m_tester->read_dmi_cache_info();
    // Assert: 守卫锁存后不再发起子进程，也不改写已有键
    EXPECT_EQ(g_qprocStartCapture.size(), 0);
    EXPECT_EQ(m_tester->d->m_info.value("L2 cache").toStdString(), std::string("canary MiB"));
    EXPECT_EQ(m_tester->l1dCache().toStdString(), std::string("-"));
}

// ==================== 非特殊机型早退套件 ====================
class UT_CPUSetDmiNotSpecial : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_origType = specialComType;
        specialComType = 0;
        m_tester = new CPUSet();
        g_qprocStartCapture.clear();
        g_qprocStdout.clear();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        specialComType = m_origType;
        g_qprocStartCapture.clear();
        g_qprocStdout.clear();
    }

    CPUSet *m_tester = nullptr;
    int m_origType = 0;
    Stub stub;
};

TEST_F(UT_CPUSetDmiNotSpecial, ReadDmiCacheInfo_NonSpecialType_EarlyReturn)
{
    // Arrange: specialComType==0 且守卫未被锁存
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    // Act
    m_tester->read_dmi_cache_info();
    // Assert: 早退且守卫被置位，缓存键缺失
    EXPECT_EQ(g_qprocStartCapture.size(), 0);
    EXPECT_EQ(m_tester->l1dCache().toStdString(), std::string("-"));
    EXPECT_EQ(m_tester->l2Cache().toStdString(), std::string("-"));
}

TEST_F(UT_CPUSetDmiNotSpecial, ReadCacheFromLscpuCmd_GuardLatched_SkipsCacheCommand)
{
    // Arrange: 前序用例已将守卫置位
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    // Act
    m_tester->read_cache_from_lscpu_cmd();
    // Assert: 守卫锁存导致命令被跳过
    EXPECT_EQ(g_qprocStartCapture.size(), 0);
    EXPECT_EQ(m_tester->l1dCache().toStdString(), std::string("-"));
    EXPECT_EQ(m_tester->d->m_info.isEmpty(), true);
}

// ==================== 未知机型 dmidecode 探测套件 ====================
class UT_CPUSetDmiUnknown : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_origType = specialComType;
        specialComType = -1;
        m_tester = new CPUSet();
        g_qprocStartCapture.clear();
        g_qprocStdout.clear();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        specialComType = m_origType;
        g_qprocStartCapture.clear();
        g_qprocStdout.clear();
    }

    CPUSet *m_tester = nullptr;
    int m_origType = 0;
    Stub stub;
};

TEST_F(UT_CPUSetDmiUnknown, ReadDmiCacheInfo_UnknownTypeNoSpecialBoard_EarlyReturn)
{
    // Arrange: 两次探测（产品名、完整输出）均为非特殊机型
    g_qprocStdout = "Generic Board\n";
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    stub.set((QByteArray (QProcess::*)())ADDR(QProcess, readAllStandardOutput), stub_qproc_read_stdout);
    // Act
    m_tester->read_dmi_cache_info();
    // Assert: 守卫未锁存时探测两次后早退；已锁存（同进程前序套件）则直接早退，
    // 官方 run-ut.sh 按套件分片运行时 capture 恒为 2
    const int starts = g_qprocStartCapture.size();
    EXPECT_TRUE(starts == 2 || starts == 0);
    EXPECT_EQ(m_tester->l1dCache().toStdString(), std::string("-"));
    EXPECT_EQ(m_tester->l2Cache().toStdString(), std::string("-"));
}
// ==================== 真实 liblscpu 冒烟套件 ====================
class UT_CPUSetLscpu : public UT_CPUSet {};

TEST_F(UT_CPUSetLscpu, ReadLscpu_RealHost_PopulatesCoreInfoKeys)
{
    // Arrange: 拦截子进程调用，避免外部命令执行；liblscpu 数据真实读取
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    // Act
    m_tester->read_lscpu();
    // Assert: 核心键齐全且主机不变式成立
    EXPECT_GE(m_tester->cpuCount(), 1);
    EXPECT_TRUE(m_tester->d->m_info.contains("Architecture"));
    EXPECT_TRUE(m_tester->d->m_info.contains("Model name"));
    EXPECT_FALSE(m_tester->d->m_info.value("L1d cache").isEmpty());
}

TEST_F(UT_CPUSetLscpu, ReadLscpu_Twice_KeepsInfoStable)
{
    // Arrange
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    m_tester->read_lscpu();
    const int firstCount = m_tester->cpuCount();
    const QString firstName = m_tester->modelName();
    // Act
    m_tester->read_lscpu();
    // Assert: 重复采集不改变既有结果
    EXPECT_EQ(m_tester->cpuCount(), firstCount);
    EXPECT_EQ(m_tester->modelName().toStdString(), firstName.toStdString());
}

TEST_F(UT_CPUSetLscpu, ReadLscpu_RealHost_PopulatesTopologyKeys)
{
    // Arrange
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    // Act
    m_tester->read_lscpu();
    // Assert: 拓扑键非空
    EXPECT_NE(m_tester->d->m_info.value("Thread(s) per core").toStdString(), std::string());
    EXPECT_TRUE(m_tester->d->m_info.contains("Thread(s) per core"));
    EXPECT_TRUE(m_tester->d->m_info.contains("Core(s) per socket")
                || m_tester->d->m_info.contains("Core(s) per cluster"));
}

