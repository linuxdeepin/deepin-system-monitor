// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// popup 模块 CPUSet 单元测试（deepin-system-monitor-plugin-popup/system/cpu_set.cpp）
//
// 用例计数声明（self-check-structural 校验此表）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | read_overall_info | high | cx:10, cog:39, lines:63, transitive_loop_depth:3, linear_scan_in_loop:2, alloc_in_loop:1 | 4 | 4 |
// | read_stats | high | cx:15, cog:52, lines:112, alloc_in_loop:1 | 4 | 4 |
// | update | high | transitive_loop_depth:3 | 4 | 4 |
// | CPUSet(拷贝构造) | mid | in_degree:1 | 2 | 2 |
// | operator= | low | - | 1 | 1 |
// | ~CPUSet | low | destructor | 1 | 1 |
// | cpuCount | low | - | 1 | 2 |
// | socketCount | low | - | 1 | 2 |
// | modelName | low | - | 1 | 2 |
// | vendor | low | - | 1 | 2 |
// | virtualization | low | - | 1 | 2 |
// | curFreq | low | - | 1 | 1 |
// | minFreq | low | - | 1 | 1 |
// | maxFreq | low | - | 1 | 1 |
// | l1dCache | low | - | 1 | 1 |
// | l1iCache | low | - | 1 | 1 |
// | l2Cache | low | - | 1 | 1 |
// | l3Cache | low | - | 1 | 1 |
// | coreId | low | - | 1 | 1 |
// | cpuLogicName | low | - | 1 | 2 |
// | stat | low | - | 1 | 2 |
// | usage | low | - | 1 | 2 |
// | statDB | low | - | 1 | 2 |
// | usageDB | low | - | 1 | 2 |
// | getUsageTotalDelta | low | - | 1 | 3 |
//
// 分支映射（源码行号，popup system/cpu_set.cpp）：
// - read_stats(:331-442) fopen失败→print_errno早退(:339-341) / "cpu "整机行 sscanf==10→m_stat+m_usage 汇总
//   / sscanf!=10→print_errno(:382) / "cpuN"分核行 sscanf==11→statDB+usageDB / !=11→print_errno(:421)
//   / "btime"行 sscanf==1→ThreadManager 链（结果未使用，安全）/ sscanf!=1→跳过 / ferror→print_errno
// - read_overall_info(:444-506) cpuinfo 按"\n\n"分块（空输出→零迭代）逐行前缀分发：
//   processor→setIndex / vendor_id→setVendorId / cpu MHz→setCpuFreq / core id→setCoreId
//   / model name→setModelName / cache size→setCacheSize / 未知前缀→忽略；
//   lscpu 行 split(":") count>1→m_info[trimmed键]=trimmed值 / count<=1→跳过
// - update(:322-329) read_stats+read_overall_info → Last←Current←m_usage->total（m_usage 私有构造
//   时预初始化为空共享指针，缺文件时为 0 而非空指针）
// - CPUSet(:316-321) 拷贝构造共享 d（QSharedDataPointer）；写时分离（update 触发 detach）
// - operator=(:217-224) d 共享赋值（QSharedDataPointer 引用计数），写时分离
// - statDB/usageDB(:254-262) const 路径不插入未知键，返回空共享指针
// - getUsageTotalDelta(:239-247) current<=last→1（防回退）/ 否则 current-last
//
// 最小检查清单（10 项）：
// 1. 等价类按 test-types.md §1 划分，无效等价类（畸形行、未知键、空输出）单列用例
// 2. 边界值（§2）：零迭代（空文件）、相等总量（delta 回退 1）、总量回退已覆盖
// 3. 每个用例 AAA 三段注释齐全
// 4. 断言强度 ≥2 且无空洞断言
// 5. 外部依赖（/proc/stat 读、QProcess 子进程）全部打桩，不依赖真实设备状态
// 6. btime 分支依赖的 ThreadManager 线程由 main-ut.cpp 中 Application 构造注入，结果未使用
// 7. 命名 Feature_Scenario_ExpectedResult 三段式
// 8. 同质组（频率/缓存获取器）使用 TEST_P 参数化
// 9. 私有成员访问依赖 -fno-access-control，与现有测试保持一致
// 10. 不修改生产代码；测试文件仅存于 popup autotests

#include "system/cpu_set.h"
#include "system/cpu.h"
#include "system/private/cpu_set_p.h"
#include "../../tests/stub.h"

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

// ==================== 测试打桩辅助（文件级共享） ====================
// QProcess 子进程捕获：不真正执行外部命令；按命令名返回 canned 输出
static QByteArray g_cpuinfoStdout;
static QByteArray g_lscpuStdout;
static QString g_lastQprocCmd;

static void stub_qproc_start(QProcess *self, const QString &program,
                             const QStringList &args, QIODevice::OpenMode mode)
{
    Q_UNUSED(self);
    Q_UNUSED(args);
    Q_UNUSED(mode);
    g_lastQprocCmd = program;
}

static bool stub_qproc_wait(QProcess *self, int msecs)
{
    Q_UNUSED(self);
    Q_UNUSED(msecs);
    return true;
}

static QByteArray stub_qproc_read_stdout(QProcess *self)
{
    Q_UNUSED(self);
    if (g_lastQprocCmd.contains("cpuinfo"))
        return g_cpuinfoStdout;
    if (g_lastQprocCmd.contains("lscpu"))
        return g_lscpuStdout;
    return QByteArray();
}

// fopen 重定向：read_stats 的 /proc/stat 读取走预写好的临时文件。
// read_stats 结束时会 fclose 返回的句柄，因此每次调用都重新打开一次，
// 避免二次 update() 复用已关闭句柄导致 double-free。
static QString g_preopenedPath;

static FILE *stub_fopen_preopened(const char *path, const char *mode)
{
    Q_UNUSED(path);
    Q_UNUSED(mode);
    // 不能调用 fopen：符号已被本 stub 替换会无限递归；改用 open + fdopen
    int fd = ::open(g_preopenedPath.toLocal8Bit().constData(), O_RDONLY);
    return fd >= 0 ? fdopen(fd, "r") : nullptr;
}

static FILE *stub_fopen_null(const char *path, const char *mode)
{
    Q_UNUSED(path);
    Q_UNUSED(mode);
    return nullptr;
}

// 写入伪 /proc/stat 内容并登记预打开路径
static void prepare_fake_proc_stat(const QStringList &lines, QTemporaryDir &dir)
{
    QFile file(dir.path() + "/stat.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        for (const QString &line : lines)
            file.write((line + "\n").toLocal8Bit());
        file.close();
    }
    g_preopenedPath = dir.path() + "/stat.txt";
}

static const char *kStdCpuinfo =
    "processor\t: 0\n"
    "vendor_id\t: GenuineIntel\n"
    "cpu MHz\t\t: 800.000\n"
    "\n"
    "processor\t: 1\n"
    "model name\t: Intel(R) Core(TM) i7\n"
    "core id\t\t: 3\n"
    "cache size\t: 4096 KB\n";

static const char *kStdLscpu =
    "Architecture:        x86_64\n"
    "CPU(s):              8\n"
    "Socket(s):           2\n"
    "Model name:          Intel(R) Xeon(R) CPU E5-2630 v4 @ 2.20GHz\n"
    "Vendor ID:           GenuineIntel\n"
    "CPU MHz:             800.024\n"
    "CPU min MHz:         400.000\n"
    "CPU max MHz:         3600.900\n"
    "L1d cache:           32K\n"
    "L1i cache:           32K\n"
    "L2 cache:            256K\n"
    "L3 cache:            20480K\n"
    "Virtualization:      VT-x\n"
    "Flags:\n"; // 空值键：split SkipEmptyParts 后 count<=1，应被跳过

// 为 read_overall_info 安装 QProcess 打桩（不真正执行子进程）
static void install_qproc_stubs(Stub &stub)
{
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_qproc_start);
    stub.set((bool (QProcess::*)(int))ADDR(QProcess, waitForFinished), stub_qproc_wait);
    stub.set((QByteArray (QProcess::*)())ADDR(QProcess, readAllStandardOutput), stub_qproc_read_stdout);
}

// ==================== 主套件 ====================
class UT_CPUSet : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_tester = new CPUSet();
        g_cpuinfoStdout.clear();
        g_lscpuStdout.clear();
        g_lastQprocCmd.clear();
        g_preopenedPath.clear();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        g_cpuinfoStdout.clear();
        g_lscpuStdout.clear();
        g_lastQprocCmd.clear();
        g_preopenedPath.clear();
    }

    CPUSet *m_tester = nullptr;
    Stub stub;
};

// ---------- read_stats（高复杂度，分支全覆盖） ----------

// B2/B3 正常路径：整机行与分核行均解析入库
TEST_F(UT_CPUSet, ReadStats_ValidLines_PopulatesStatAndUsageDb)
{
    // Arrange: 伪 /proc/stat 含整机行(sscanf 10 字段)与两个分核行(11 字段)
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0"
        << "cpu0 10 0 20 70 0 0 1 0 0 0"
        << "cpu1 90 0 180 630 0 0 9 0 0 0", dir);
    stub.set(fopen, stub_fopen_preopened);
    // Act
    m_tester->read_stats();
    // Assert: 整机 stat 字段与 usage 总量/空闲均按汇总计算；分核入库
    EXPECT_EQ(m_tester->stat()->user, 100ULL);
    EXPECT_EQ(m_tester->stat()->idle, 700ULL);
    EXPECT_EQ(m_tester->usage()->total, 1010ULL);
    EXPECT_EQ(m_tester->usage()->idle, 700ULL);
    EXPECT_NE(m_tester->statDB("cpu0"), nullptr);
    EXPECT_EQ(m_tester->statDB("cpu0")->user, 10ULL);
    EXPECT_EQ(m_tester->usageDB("cpu1")->total, 909ULL);
    EXPECT_EQ(m_tester->cpuLogicName().size(), 2);
}

// B1 fopen 失败：print_errno 后早退，保持零状态（无效等价类）
TEST_F(UT_CPUSet, ReadStats_FopenFails_KeepsDefaultState)
{
    // Arrange: fopen 打桩为返回空
    stub.set(fopen, stub_fopen_null);
    // Act
    m_tester->read_stats();
    // Assert: 打不开文件时保持默认零状态且不崩溃
    EXPECT_NE(m_tester->stat(), nullptr);
    EXPECT_EQ(m_tester->stat()->user, 0ULL);
    EXPECT_EQ(m_tester->usage()->total, 0ULL);
    EXPECT_TRUE(m_tester->cpuLogicName().isEmpty());
}

// B2'/B3' 畸形行：字段数不足走 print_errno 分支并被跳过（无效等价类）
TEST_F(UT_CPUSet, ReadStats_MalformedLines_SkipsInvalidEntries)
{
    // Arrange: 整机行字段不足(≠10)、分核行字段不足(≠11)、无关行、btime 值非法
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    prepare_fake_proc_stat(QStringList()
        << "cpu  garbage values here"
        << "cpu0 1 2 3"
        << "text not a cpu line"
        << "btime notanumber", dir);
    stub.set(fopen, stub_fopen_preopened);
    // Act
    m_tester->read_stats();
    // Assert: 畸形行全部被跳过，仅保留默认占位结构
    EXPECT_EQ(m_tester->stat()->cpu.isEmpty(), true);
    EXPECT_EQ(m_tester->usage()->total, 0ULL);
    EXPECT_EQ(m_tester->statDB("cpu0"), nullptr);
    EXPECT_TRUE(m_tester->cpuLogicName().isEmpty());
}

// B4 btime 分支：sscanf==1 走 ThreadManager 链（结果未使用）；不破坏其余解析
TEST_F(UT_CPUSet, ReadStats_BtimeLine_ToleratedWithoutBreakingParsing)
{
    // Arrange: 有效统计行 + btime 行(值合法) + btime 行(值非法)混合
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0"
        << "cpu0 10 0 20 70 0 0 1 0 0 0"
        << "btime 1700000000"
        << "btime garbage", dir);
    stub.set(fopen, stub_fopen_preopened);
    // Act
    m_tester->read_stats();
    // Assert: btime 行不影响统计解析，且 ThreadManager 链不崩溃
    EXPECT_EQ(m_tester->usage()->total, 1010ULL);
    EXPECT_EQ(m_tester->usageDB("cpu0")->total, 101ULL);
    EXPECT_EQ(m_tester->cpuLogicName().size(), 1);
}

// ---------- read_overall_info（高复杂度，分支全覆盖） ----------

// B7 六种前缀全命中：cpuinfo 分块逐字段入 CPUInfo（有效等价类）
TEST_F(UT_CPUSet, ReadOverallInfo_ParsesCpuinfoBlocks_PopulatesInfos)
{
    // Arrange: canned cpuinfo 两个 processor 块覆盖全部前缀
    g_cpuinfoStdout = kStdCpuinfo;
    install_qproc_stubs(stub);
    // Act
    m_tester->read_overall_info();
    // Assert: 两个块各字段按原始(未 trim)值入列
    EXPECT_EQ(m_tester->d->m_infos.size(), 2);
    EXPECT_EQ(m_tester->d->m_infos[0].logicalIndex(), 0);
    EXPECT_EQ(m_tester->d->m_infos[0].vendorId().toStdString(), std::string(" GenuineIntel"));
    EXPECT_EQ(m_tester->d->m_infos[0].cpuFreq().toStdString(), std::string(" 800.000MHz"));
    EXPECT_EQ(m_tester->d->m_infos[1].modelName().toStdString(), std::string(" Intel(R) Core(TM) i7"));
    EXPECT_EQ(m_tester->d->m_infos[1].coreID().toStdString(), std::string("3"));
    EXPECT_EQ(m_tester->d->m_infos[1].cacheSize().toStdString(), std::string(" 4096 KB"));
    EXPECT_EQ(m_tester->coreId(1).toStdString(), std::string("3"));
}

// B8 lscpu 键值行：count>1 入 m_info（trim 后），空值键跳过
TEST_F(UT_CPUSet, ReadOverallInfo_LscpuKeyValues_PopulatesInfoMap)
{
    // Arrange: canned lscpu 含 13 个有效键值行 + 1 个空值行
    g_lscpuStdout = kStdLscpu;
    install_qproc_stubs(stub);
    // Act
    m_tester->read_overall_info();
    // Assert: 各公开获取器返回 trim 后的值
    EXPECT_EQ(m_tester->cpuCount(), 8);
    EXPECT_EQ(m_tester->socketCount(), 2);
    EXPECT_EQ(m_tester->modelName().toStdString(), std::string("Intel(R) Xeon(R) CPU E5-2630 v4 @ 2.20GHz"));
    EXPECT_EQ(m_tester->vendor().toStdString(), std::string("GenuineIntel"));
    EXPECT_EQ(m_tester->virtualization().toStdString(), std::string("VT-x"));
    EXPECT_EQ(m_tester->l1dCache().toStdString(), std::string("32K"));
    EXPECT_EQ(m_tester->l2Cache().toStdString(), std::string("256K"));
}

// B6/B7' 空输出：零迭代边界，m_infos 与 m_info 均为空（无效等价类）
TEST_F(UT_CPUSet, ReadOverallInfo_EmptyOutput_LeavesDefaults)
{
    // Arrange: QProcess 两个命令均返回空
    install_qproc_stubs(stub);
    // Act
    m_tester->read_overall_info();
    // Assert: 无任何条目，获取器回退默认值
    EXPECT_EQ(m_tester->d->m_infos.size(), 0);
    EXPECT_EQ(m_tester->cpuCount(), 0);
    EXPECT_TRUE(m_tester->modelName().isEmpty());
}

// B7'' 未知前缀被忽略，仅匹配的前缀生效（负面分发用例）
TEST_F(UT_CPUSet, ReadOverallInfo_UnknownPrefixes_SkippedSafely)
{
    // Arrange: 块内混合未知前缀行与部分已知前缀
    g_cpuinfoStdout =
        "processor\t: 0\n"
        "bogus field\t: x\n"
        "model name\t: TestCPU\n";
    install_qproc_stubs(stub);
    // Act
    m_tester->read_overall_info();
    // Assert: 仅 model name 入列，未知行不产生副作用
    ASSERT_EQ(m_tester->d->m_infos.size(), 1);
    EXPECT_EQ(m_tester->d->m_infos[0].modelName().toStdString(), std::string(" TestCPU"));
    EXPECT_TRUE(m_tester->d->m_infos[0].vendorId().isEmpty());
}

// ---------- update（高复杂度 + 循环因子） ----------

// B10 首次 update：Last←0，Current←usage 总量（循环边界：初次迭代）
TEST_F(UT_CPUSet, Update_AfterFirstCall_CollectsTotalWithDelta)
{
    // Arrange: 伪 /proc/stat 整机总量 1010；QProcess 空输出
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0", dir);
    stub.set(fopen, stub_fopen_preopened);
    install_qproc_stubs(stub);
    // Act
    m_tester->update();
    // Assert: 总量入库，last=0 → delta=1010
    EXPECT_EQ(m_tester->usage()->total, 1010ULL);
    EXPECT_EQ(m_tester->getUsageTotalDelta(), 1010ULL);
}

// B10 二次 update 总量不变：delta 回退为 1（getUsageTotalDelta 相等边界）
TEST_F(UT_CPUSet, GetUsageTotalDelta_SameTotals_ReturnsOne)
{
    // Arrange: 两次 update 读到相同总量
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0", dir);
    stub.set(fopen, stub_fopen_preopened);
    install_qproc_stubs(stub);
    // Act
    m_tester->update();
    m_tester->update();
    // Assert: last==current → 回退 1
    EXPECT_EQ(m_tester->getUsageTotalDelta(), 1ULL);
    EXPECT_EQ(m_tester->usage()->total, 1010ULL);
}

// B10 总量回退：current<last → delta 回退为 1（无效等价类）
TEST_F(UT_CPUSet, GetUsageTotalDelta_DecreasingTotal_ClampedToOne)
{
    // Arrange: 第一次总量 1010，第二次总量降为 500
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0", dir);
    stub.set(fopen, stub_fopen_preopened);
    install_qproc_stubs(stub);
    // Act
    m_tester->update();
    prepare_fake_proc_stat(QStringList()
        << "cpu  50 0 100 300 0 0 50 0 0 0", dir);
    m_tester->update();
    // Assert: current(500) < last(1010) → 回退 1
    EXPECT_EQ(m_tester->usage()->total, 500ULL);
    EXPECT_EQ(m_tester->getUsageTotalDelta(), 1ULL);
}

// B10 缺文件场景：read_stats 早退后 update 不崩溃（m_usage 预初始化守卫）
TEST_F(UT_CPUSet, Update_StatFileMissing_DoesNotCrash)
{
    // Arrange: fopen 失败 + QProcess 空输出
    stub.set(fopen, stub_fopen_null);
    install_qproc_stubs(stub);
    // Act
    m_tester->update();
    // Assert: 零状态且 delta 回退 1（0<=0）
    EXPECT_EQ(m_tester->usage()->total, 0ULL);
    EXPECT_EQ(m_tester->getUsageTotalDelta(), 1ULL);
    EXPECT_TRUE(m_tester->cpuLogicName().isEmpty());
}

// ---------- 拷贝构造 / operator=（写时分离语义） ----------

// B11 拷贝构造共享 d：读值一致且引用同一私有数据
TEST_F(UT_CPUSet, CopyCtor_BeforeWrite_SharesPrivateData)
{
    // Arrange: 原对象装载真实统计数据
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0", dir);
    stub.set(fopen, stub_fopen_preopened);
    m_tester->read_stats();
    // Act
    CPUSet copy(*m_tester);
    // Assert: 经 const 引用访问（非 const data() 会触发 detach）确认引用共享
    const CPUSet &copyRef = copy;
    const CPUSet &origRef = *m_tester;
    EXPECT_EQ(copyRef.d.data(), origRef.d.data());
    EXPECT_EQ(copyRef.d.data()->ref.loadRelaxed(), 2);
    EXPECT_EQ(copy.stat()->user, 100ULL);
    EXPECT_EQ(copy.usage()->total, 1010ULL);
}

// B11 拷贝后写入：detach 保证副本与原对象隔离（写时分离边界）
TEST_F(UT_CPUSet, CopyCtor_OnWrite_DetachesFromOriginal)
{
    // Arrange: 原对象总量 1010，副本改读总量 2000 的伪文件
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0", dir);
    stub.set(fopen, stub_fopen_preopened);
    m_tester->read_stats();
    CPUSet copy(*m_tester);
    install_qproc_stubs(stub);
    // Act: 副本以新内容 update（触发 detach）
    prepare_fake_proc_stat(QStringList()
        << "cpu  200 0 400 1400 0 0 0 0 0 0", dir);
    copy.update();
    // Assert: 副本为新值，原对象保持旧值
    EXPECT_EQ(copy.usage()->total, 2000ULL);
    EXPECT_EQ(m_tester->usage()->total, 1010ULL);
    EXPECT_NE(copy.d.data(), m_tester->d.data());
}

// B12 operator=：赋值共享且写时独立（低复杂度单用例）
TEST_F(UT_CPUSet, AssignOperator_CopiesValues_StaysIndependent)
{
    // Arrange: 原对象同时具备统计与 lscpu 信息
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0", dir);
    stub.set(fopen, stub_fopen_preopened);
    g_lscpuStdout = kStdLscpu;
    install_qproc_stubs(stub);
    m_tester->update();
    CPUSet assigned;
    // Act
    assigned = *m_tester;
    // Assert: 赋值共享同一 d 且值一致（const 引用访问避免 data() detach）
    const CPUSet &assignedRef = assigned;
    const CPUSet &sourceRef = *m_tester;
    EXPECT_EQ(assignedRef.d.data(), sourceRef.d.data());
    EXPECT_EQ(assigned.usage()->total, 1010ULL);
    EXPECT_EQ(assigned.cpuCount(), m_tester->cpuCount());
    EXPECT_EQ(assigned.cpuCount(), 8);
}

// 自赋值路径：operator= 提前返回,不重复分配
TEST_F(UT_CPUSet, AssignOperator_SelfAssign_ReturnsEarlyWithoutCopy)
{
    // Arrange: 已加载数据的对象与指向自身的引用
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    prepare_fake_proc_stat(QStringList()
        << "cpu  100 0 200 700 0 0 10 0 0 0", dir);
    stub.set(fopen, stub_fopen_preopened);
    m_tester->update();
    const CPUUsage before = m_tester->usage();
    // Act: 自赋值(this == &rhs → 提前 return)
    CPUSet &selfRef = *m_tester;
    *m_tester = selfRef;
    // Assert: 状态不变(仍为同一 usage 指针,无重建)
    EXPECT_EQ(m_tester->usage(), before);
    EXPECT_EQ(m_tester->usage()->total, 1010ULL);
}

// virtualization 空值分支: lscpu 无 Virtualization 键时回退文案
TEST_F(UT_CPUSet, Virtualization_EmptyKey_FallsBackToNotSupport)
{
    // Arrange: 未装载 lscpu 的默认对象,m_info 无 Virtualization 键
    ASSERT_NE(m_tester, nullptr);
    // Act: 直接读取虚化能力
    const QString value = m_tester->virtualization();
    // Assert: 回退 "Not support" 文案
    EXPECT_FALSE(value.isEmpty());
    EXPECT_EQ(value, QObject::tr("Not support"));
}

// ---------- 默认构造 / 析构 / 缺省键（生命周期与无效等价类） ----------

// B13 默认构造与析构：占位结构非空、缺省键回退默认、虚化能力回退文案
TEST_F(UT_CPUSet, Lifecycle_DefaultConstructed_GettersSafe)
{
    // Arrange: 默认对象（无外部状态），预置期望文案
    const QString notSupportText = QStringLiteral("Not support");
    // Act: 构造后立即析构（覆盖 ~CPUSet），再以默认实例断言缺省状态
    {
        CPUSet transient;
        EXPECT_NE(transient.stat(), nullptr);
    }
    CPUSet fresh;
    // Assert: 缺省键全部回退默认值
    EXPECT_EQ(fresh.cpuCount(), 0);
    EXPECT_EQ(fresh.socketCount(), 0);
    EXPECT_TRUE(fresh.modelName().isEmpty());
    EXPECT_EQ(fresh.virtualization(), notSupportText);
}

// B14 statDB 未知键：返回空共享指针且不崩溃（无效等价类）
TEST_F(UT_CPUSet, StatDb_UnknownKey_ReturnsNullStat)
{
    // Arrange: 默认对象与未知键
    const QByteArray unknownKey("no-such-cpu");
    // Act
    CPUStat stat = m_tester->statDB(unknownKey);
    // Assert: 未知键返回空指针，整机统计不受影响
    EXPECT_EQ(stat, nullptr);
    EXPECT_NE(m_tester->stat(), nullptr);
    EXPECT_EQ(m_tester->stat()->user, 0ULL);
}

// B15 usageDB 未知键：const 访问路径返回空共享指针且不插入（无效等价类）
TEST_F(UT_CPUSet, UsageDb_UnknownKey_ReturnsNullWithoutInsert)
{
    // Arrange: 默认对象与未知键
    const QByteArray unknownKey("cpuX");
    // Act
    CPUUsage usage = m_tester->usageDB(unknownKey);
    // Assert: 返回空指针；const 路径不改变 usageDB（cpuLogicName 不变）
    EXPECT_EQ(usage, nullptr);
    EXPECT_TRUE(m_tester->cpuLogicName().isEmpty());
    EXPECT_NE(m_tester->usage(), nullptr);
}

// ---------- 频率获取器（同质组参数化：formatHz 换算） ----------
using FreqGetter = QString (CPUSet::*)() const;
struct FreqParam
{
    FreqGetter getter;
    const char *expected;
};

class UT_CPUSetFreqParam : public UT_CPUSet,
                           public ::testing::WithParamInterface<FreqParam>
{
};

TEST_P(UT_CPUSetFreqParam, FreqGetters_CannedLscpu_ReturnsScaledString)
{
    // Arrange: canned lscpu 提供 CPU MHz=800.024 / min=400.000 / max=3600.900
    g_lscpuStdout = kStdLscpu;
    install_qproc_stubs(stub);
    m_tester->read_overall_info();
    // Act
    const QString value = (m_tester->*GetParam().getter)();
    // Assert: formatHz 以 MHz 基准换算，>1000 进位 GHz；lscpu 已实际装载
    EXPECT_EQ(value.toStdString(), std::string(GetParam().expected));
    EXPECT_EQ(m_tester->cpuCount(), 8);
}

INSTANTIATE_TEST_SUITE_P(HzUnits, UT_CPUSetFreqParam,
    ::testing::Values(
        FreqParam{&CPUSet::curFreq, "800.00MHz"},   // 800.024 截断为 800
        FreqParam{&CPUSet::minFreq, "400.00MHz"},
        FreqParam{&CPUSet::maxFreq, "3.60GHz"}));   // 3600.9 截断后进位 GHz

// ---------- 缓存获取器（同质组参数化：原样返回 m_info 值） ----------
using CacheGetter = QString (CPUSet::*)() const;
struct CacheParam
{
    CacheGetter getter;
    const char *expected;
};

class UT_CPUSetCacheParam : public UT_CPUSet,
                            public ::testing::WithParamInterface<CacheParam>
{
};

TEST_P(UT_CPUSetCacheParam, CacheGetters_CannedLscpu_ReturnsRawValue)
{
    // Arrange: canned lscpu 提供四级缓存行
    g_lscpuStdout = kStdLscpu;
    install_qproc_stubs(stub);
    m_tester->read_overall_info();
    // Act
    const QString value = (m_tester->*GetParam().getter)();
    // Assert: 原样返回 trim 后的缓存容量；lscpu 已实际装载
    EXPECT_EQ(value.toStdString(), std::string(GetParam().expected));
    EXPECT_EQ(m_tester->cpuCount(), 8);
}

INSTANTIATE_TEST_SUITE_P(CacheLevels, UT_CPUSetCacheParam,
    ::testing::Values(
        CacheParam{&CPUSet::l1dCache, "32K"},
        CacheParam{&CPUSet::l1iCache, "32K"},
        CacheParam{&CPUSet::l2Cache, "256K"},
        CacheParam{&CPUSet::l3Cache, "20480K"}));
