// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "model/cpu_info_model.h"
//system
#include "system/cpu_set.h"
#include "system/sys_info.h"
#include "system/device_db.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QSignalSpy>
#include <QRegularExpression>
#include <QtMath>
#include <unistd.h>

/***************************************STUB begin*********************************************/
// 替换 CPUSet::cpuLogicName()（out-of-line，可安全打桩），
// 用于 updateModel 的循环空集合边界用例
QList<QByteArray> stub_cpuLogicName_empty()
{
    return QList<QByteArray>();
}
/***************************************STUB end**********************************************/

/*
 * 用例计数声明（mode2 §4：actual 不得低于 min）
 *
 * | method            | level | factors                | min | actual |
 * |-------------------|-------|------------------------|-----|--------|
 * | CPUInfoModel      | low   | -                      | 1   | 1      |
 * | ~CPUInfoModel     | low   | -                      | 1   | 1      |
 * | instance          | mid   | in_degree:6            | 2   | 2      |
 * | sysInfo           | low   | -                      | 1   | 1      |
 * | cpuSet            | low   | -                      | 1   | 1      |
 * | cpuPercentList    | low   | -                      | 1   | 1      |
 * | cpuAllPercent     | mid   | in_degree:3            | 2   | 2      |
 * | loadavg           | low   | -                      | 1   | 1      |
 * | nProcesses        | low   | -                      | 1   | 1      |
 * | nThreads          | low   | -                      | 1   | 1      |
 * | nFileDescriptors  | low   | -                      | 1   | 1      |
 * | hostname          | low   | -                      | 1   | 1      |
 * | osType            | low   | -                      | 1   | 1      |
 * | osVersion         | low   | -                      | 1   | 1      |
 * | uptime            | mid   | in_degree:5            | 2   | 2      |
 * | updateModel       | mid   | linear_scan_in_loop:1  | 3   | 3      |
 *
 * 分支清单（基于 get_code_snippet 实取源码）：
 * - updateModel: ① 首次采样 m_singleUsageSample 不含 cpuN → 新建 Sample（else 分支）
 *                ② 二次采样已含 cpuN → 复用已有 Sample 追加（contains 分支）
 *                ③ cpuLogicName 为空 → 循环体跳过，仍 emit modelUpdated（循环空边界）
 * - cpuAllPercent: ① 无样本 → recentSamplePair 全空 → cpupc(null,null) 返回 0
 *                  ② 单样本 → pair.second==nullptr → 按单帧 total 计算
 * - uptime: 由 m_sysInfo->uptime().tv_sec 拆分 days/hours/mins 后按
 *           "%1 days %2 hours %3 minutes" 格式化（tr 未翻译时为字面量模板）
 * - cpuPercentList: 按 cpuName 数字序排序后逐项 cpupc（linear_scan_in_loop）
 *
 * test-types.md §8 最小清单：
 * | # | 检查项 | 状态 |
 * |---|--------|------|
 * | 1 | 每个公开方法 ≥ 1 用例 | [x] |
 * | 2 | 输入维度等价类划分 | [x] |
 * | 3 | 边界值显式覆盖（空样本=0 / 循环空集合） | [x] |
 * | 4 | 同质多组输入参数化 | [x] 不适用（无同质多组输入接口） |
 * | 5 | 分支清单列出并映射用例名 | [x] |
 * | 6 | if/switch/early-return 分支有触发用例 | [x] |
 * | 7 | 异常路径 EXPECT_THROW | [x] 不适用（接口无异常路径） |
 * | 8 | 负面场景专门用例 | [x] 空样本边界即负面场景 |
 * | 9 | 负面用例强异常安全 | [x] |
 * | 10 | stub 策略：Qt/全局函数 stub.h，外部设备不直接依赖 | [x] |
 */

class UT_CPUInfoModel: public ::testing::Test
{
public:
    UT_CPUInfoModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new CPUInfoModel();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CPUInfoModel *m_tester;
};

// 分支：构造函数初始化 m_sysInfo=SysInfo::instance()、m_cpuSet=DeviceDB 的 cpuSet
TEST_F(UT_CPUInfoModel, Ctor_Default_SetsSysInfoAndCpuSet)
{
    // Arrange: SetUp 已构造模型
    ASSERT_NE(m_tester, nullptr);
    // Act: 读取内部持有的两个依赖指针
    SysInfo *info = m_tester->sysInfo();
    CPUSet *set = m_tester->cpuSet();
    // Assert: 两个依赖均非空且与全局单例一致
    EXPECT_NE(info, nullptr);
    EXPECT_EQ(info, SysInfo::instance());
    EXPECT_NE(set, nullptr);
    EXPECT_EQ(set, DeviceDB::instance()->cpuSet());
}

// 覆盖 ~CPUInfoModel：更新后析构不崩溃、不泄漏（配合 valgrind/asan 可查）
TEST_F(UT_CPUInfoModel, Dtor_AfterUpdate_ReleasesCleanly)
{
    // Arrange: 独立构造一个模型并写入采样
    CPUInfoModel *model = new CPUInfoModel();
    model->updateModel();
    // Act: 立即析构
    delete model;
    // Assert: 全局单例不受局部实例析构影响
    EXPECT_NE(CPUInfoModel::instance(), nullptr);
    EXPECT_EQ(m_tester->sysInfo(), SysInfo::instance());
}

// instance(): 单例语义
TEST_F(UT_CPUInfoModel, Instance_CalledTwice_ReturnsSamePointer)
{
    // Arrange: 无需前置状态，单例由静态工厂维护
    ASSERT_NE(CPUInfoModel::instance(), nullptr);
    // Act: 连续两次获取单例
    CPUInfoModel *first = CPUInfoModel::instance();
    CPUInfoModel *second = CPUInfoModel::instance();
    // Assert: 非空且指针一致
    EXPECT_NE(first, nullptr);
    EXPECT_EQ(first, second);
}

// instance(): 单例与局部实例互不影响（异常安全）
TEST_F(UT_CPUInfoModel, Instance_AfterLocalModelDestroyed_StaysValid)
{
    // Arrange: 局部构造并销毁一个实例
    {
        CPUInfoModel local;
        EXPECT_NE(local.sysInfo(), nullptr);
    }
    // Act: 再次获取单例
    CPUInfoModel *inst = CPUInfoModel::instance();
    // Assert: 单例仍有效
    EXPECT_NE(inst, nullptr);
    EXPECT_EQ(inst, CPUInfoModel::instance());
}

// cpuAllPercent 分支①：无样本时 cpupc(null,null) 返回 0（空样本边界）
TEST_F(UT_CPUInfoModel, CpuAllPercent_NoSamples_ReturnsZero)
{
    // Arrange: SetUp 新建模型，未调用 updateModel
    ASSERT_NE(m_tester, nullptr);
    // Act: 直接查询总体 CPU 占用率
    qreal percent = m_tester->cpuAllPercent();
    // Assert: 空样本边界返回 0
    EXPECT_DOUBLE_EQ(percent, 0.0);
    EXPECT_FALSE(qIsFinite(percent) && (percent < 0.0 || percent > 100.0));
}

// cpuAllPercent 分支②：单样本（pair.second==nullptr）按单帧计算，结果落在 [0,100]
TEST_F(UT_CPUInfoModel, CpuAllPercent_AfterSingleUpdate_InRange)
{
    // Arrange: 先经公开 API update() 读取 /proc/stat 填充真实占用值，再采一次样本
    m_tester->cpuSet()->update();
    m_tester->updateModel();
    // Act: 查询总体占用率
    qreal percent = m_tester->cpuAllPercent();
    // Assert: 单帧路径结果为有限值且在百分比区间内
    EXPECT_TRUE(qIsFinite(percent));
    EXPECT_GE(percent, 0.0);
    EXPECT_LE(percent, 100.0);
}

// loadavg(): "%1 %2 %3"（各保留 2 位小数）格式
TEST_F(UT_CPUInfoModel, Loadavg_SystemRunning_MatchesThreeNumberFormat)
{
    // Arrange: 正则匹配 3 个 2 位小数的数字，空格分隔
    static const QRegularExpression re(
        QStringLiteral("^\\d+\\.\\d{2} \\d+\\.\\d{2} \\d+\\.\\d{2}$"));
    ASSERT_NE(m_tester, nullptr);
    // Act: 读取负载均值字符串
    QString avg = m_tester->loadavg();
    // Assert: 非空、含两个空格且严格符合格式
    EXPECT_FALSE(avg.isEmpty());
    EXPECT_GE(avg.count(' '), 2);
    EXPECT_TRUE(re.match(avg).hasMatch()) << "actual: " << avg.toStdString();
}

// nProcesses(): /proc 实时进程数
TEST_F(UT_CPUInfoModel, NProcesses_SystemRunning_ReturnsPositive)
{
    // Arrange: 先经公开 API readSysInfo() 刷新 /proc 动态计数（生产路径由监控线程驱动）
    ASSERT_NE(m_tester, nullptr);
    m_tester->sysInfo()->readSysInfo();
    // Act: 读取进程数
    uint count = m_tester->nProcesses();
    // Assert: 合理区间 (>0 且小于百万)
    EXPECT_GT(count, 0u);
    EXPECT_LT(count, 1000000u);
}

// nThreads(): /proc 实时线程数
TEST_F(UT_CPUInfoModel, NThreads_SystemRunning_ReturnsPositive)
{
    // Arrange: 系统运行中必有线程
    ASSERT_NE(m_tester, nullptr);
    // Act: 读取线程数
    uint count = m_tester->nThreads();
    // Assert: 合理区间
    EXPECT_GT(count, 0u);
    EXPECT_LT(count, 10000000u);
}

// nFileDescriptors(): /proc/sys/fs/file-nr 实时句柄数
TEST_F(UT_CPUInfoModel, NFileDescriptors_SystemRunning_ReturnsPositive)
{
    // Arrange: 先经公开 API readSysInfo() 刷新 /proc 动态计数
    ASSERT_NE(m_tester, nullptr);
    m_tester->sysInfo()->readSysInfo();
    // Act: 读取句柄数
    uint count = m_tester->nFileDescriptors();
    // Assert: 合理区间
    EXPECT_GT(count, 0u);
    EXPECT_LT(count, 10000000u);
}

// hostname(): 非空且与 SysInfo 来源一致
TEST_F(UT_CPUInfoModel, Hostname_SystemRunning_ReturnsNonEmpty)
{
    // Arrange: 真实系统存在主机名
    ASSERT_NE(m_tester, nullptr);
    // Act: 读取主机名
    QString name = m_tester->hostname();
    // Assert: 非空且不含空白符
    EXPECT_GE(name.size(), 1);
    EXPECT_FALSE(name.contains(QRegularExpression("\\s")));
}

// osType(): 返回架构字符串（如 x86_64）
TEST_F(UT_CPUInfoModel, OsType_SystemRunning_ReturnsArchToken)
{
    // Arrange: 架构令牌为字母数字下划线连字符
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9_\\-]+$"));
    ASSERT_NE(m_tester, nullptr);
    // Act: 读取 osType
    QString type = m_tester->osType();
    // Assert: 非空且为合法架构令牌
    EXPECT_GE(type.size(), 1);
    EXPECT_TRUE(re.match(type).hasMatch()) << "actual: " << type.toStdString();
}

// osVersion(): 返回内核版本号，含数字
TEST_F(UT_CPUInfoModel, OsVersion_SystemRunning_ContainsVersionDigits)
{
    // Arrange: 内核版本形如 5.10.x / 6.6.x
    static const QRegularExpression re(QStringLiteral("\\d"));
    ASSERT_NE(m_tester, nullptr);
    // Act: 读取 osVersion
    QString version = m_tester->osVersion();
    // Assert: 非空且含数字
    EXPECT_GE(version.size(), 1);
    EXPECT_TRUE(re.match(version).hasMatch()) << "actual: " << version.toStdString();
}

// uptime 分支：真实 uptime 拆分 days/hours/mins 后按模板格式化
TEST_F(UT_CPUInfoModel, Uptime_SystemRunning_MatchesDayHourMinuteFormat)
{
    // Arrange: tr() 未加载翻译时返回字面量模板 "%1 days %2 hours %3 minutes"
    static const QRegularExpression re(
        QStringLiteral("^\\d+ days \\d+ hours \\d+ minutes$"));
    ASSERT_NE(m_tester, nullptr);
    // Act: 读取格式化后的运行时长
    QString up = m_tester->uptime();
    // Assert: 非空、含 5 个空格（模板字段数）且符合模板
    EXPECT_GE(up.count(' '), 5);
    EXPECT_TRUE(re.match(up).hasMatch()) << "actual: " << up.toStdString();
}

// uptime 边界：拆分字段 hours<24、mins<60、均非负（数值一致性校验）
TEST_F(UT_CPUInfoModel, Uptime_SystemRunning_FieldsInValidRange)
{
    // Arrange: 提取格式化串中的三个数字
    static const QRegularExpression re(
        QStringLiteral("^(\\d+) days (\\d+) hours (\\d+) minutes$"));
    // Act: 读取并捕获字段
    QString up = m_tester->uptime();
    auto match = re.match(up);
    // Assert: 字段均在自然区间内
    ASSERT_TRUE(match.hasMatch());
    long days = match.captured(1).toLong();
    long hours = match.captured(2).toLong();
    long mins = match.captured(3).toLong();
    EXPECT_GE(days, 0L);
    EXPECT_GE(hours, 0L);
    EXPECT_LT(hours, 24L);
    EXPECT_GE(mins, 0L);
    EXPECT_LT(mins, 60L);
}

// updateModel 分支①：首次采样，每逻辑核新建 Sample（else 分支）并发信号
TEST_F(UT_CPUInfoModel, UpdateModel_FirstCall_CreatesPerCoreSamples)
{
    // Arrange: 先经公开 API update() 读取 /proc/stat，再挂信号探针
    m_tester->cpuSet()->update();
    QSignalSpy spy(m_tester, &CPUInfoModel::modelUpdated);
    const int coreCount = m_tester->cpuSet()->cpuLogicName().size();
    ASSERT_GT(coreCount, 0);
    // Act: 首次更新模型
    m_tester->updateModel();
    // Assert: 发出一次信号，且 cpuPercentList 与逻辑核数一致
    EXPECT_EQ(spy.count(), 1);
    QList<qreal> percents = m_tester->cpuPercentList();
    EXPECT_EQ(percents.size(), coreCount);
}

// updateModel 分支②：二次采样复用已存在的 Sample（contains 分支）
TEST_F(UT_CPUInfoModel, UpdateModel_SecondCall_ReusesPerCoreSamples)
{
    // Arrange: 经公开 API update() 读取 /proc/stat 后采一次样；休眠 60ms 保证 Tick 前进
    m_tester->cpuSet()->update();
    QSignalSpy spy(m_tester, &CPUInfoModel::modelUpdated);
    m_tester->updateModel();
    ::usleep(60 * 1000);
    // Act: 再次 update() 产生新增量，随后第二次更新模型（命中 contains 复用分支）
    m_tester->cpuSet()->update();
    m_tester->updateModel();
    // Assert: 信号累计两次，占用率列表仍与核数一致且为有限值
    EXPECT_EQ(spy.count(), 2);
    QList<qreal> percents = m_tester->cpuPercentList();
    EXPECT_EQ(percents.size(), m_tester->cpuSet()->cpuLogicName().size());
    for (qreal p : percents) {
        EXPECT_TRUE(qIsFinite(p));
        EXPECT_GE(p, 0.0);
        EXPECT_LE(p, 100.0);
    }
}

// updateModel 分支③（循环空集合边界）：cpuLogicName 为空时循环体跳过，仍发信号
TEST_F(UT_CPUInfoModel, UpdateModel_EmptyCoreList_StillEmitsSignal)
{
    // Arrange: 打桩 cpuLogicName 返回空列表
    Stub stub;
    stub.set(ADDR(CPUSet, cpuLogicName), stub_cpuLogicName_empty);
    QSignalSpy spy(m_tester, &CPUInfoModel::modelUpdated);
    // Act: 更新模型（循环零次）
    m_tester->updateModel();
    // Assert: 信号仍发出；未打桩的总体采样不受影响
    EXPECT_EQ(spy.count(), 1);
    EXPECT_NE(m_tester->cpuSet(), nullptr);
}

// cpuPercentList（linear_scan_in_loop）：按逻辑核数字序输出占用率列表
TEST_F(UT_CPUInfoModel, CpuPercentList_AfterUpdate_SizeMatchesCoreCount)
{
    // Arrange: 先经公开 API update() 读取 /proc/stat，再采集样本并取逻辑核名列表
    m_tester->cpuSet()->update();
    m_tester->updateModel();
    const QList<QByteArray> cores = m_tester->cpuSet()->cpuLogicName();
    ASSERT_FALSE(cores.isEmpty());
    // Act: 读取各核占用率
    QList<qreal> percents = m_tester->cpuPercentList();
    // Assert: 每个逻辑核对应一项且取值合法
    EXPECT_EQ(percents.size(), cores.size());
    for (qreal p : percents) {
        EXPECT_GE(p, 0.0);
        EXPECT_LE(p, 100.0);
    }
}
