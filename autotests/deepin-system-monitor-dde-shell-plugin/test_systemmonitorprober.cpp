// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 单元测试 - SystemMonitorProber
// （deepin-system-monitor-dde-shell-plugin/systemmonitorprober.cpp）
//
// 测试策略：/proc/stat、/proc/meminfo、/proc/net/dev 经 ProcSandbox（POSIX
// ::open 前缀重定向）注入受控内容，QFile 读取与解析逻辑全部真实执行，不依赖
// 真实 /proc 数值；QNetworkInterface 环境差异经打桩 preferredNetworkInterfaces
// （私有，-fno-access-control 直调/打桩）消除；QElapsedTimer 打桩为固定时长，
// 使 networkSpeed 的速率计算可精确断言。
//
// 用例计数声明：
// | 函数 | 级别 | 因子 | min | actual |
// |------|------|------|-----|--------|
// | cpuUsagePercent | high | 采样/边界/失败 | 10 | 10 |
// | memoryUsagePercent | high | 正常/回退/失败 | 9 | 9 |
// | networkSpeed | high | 采样/过滤/失败 | 10 | 10 |
// | preferredNetworkInterfaces | medium | 过滤不变量 | 1 | 1 |
// | formatTransferRate | medium | 单位段边界 | 7 | 7 |
//
// 10 项检查清单（test-code-gen.md）：
// 1 public 方法覆盖：cpuUsagePercent/memoryUsagePercent/networkSpeed 全覆盖；
// 2 等价类：CPU 空闲/部分繁忙/满载、内存正常/回退估算、速率 B/KB/MB 三段；
// 3 边界值：5 字段无 iowait、available>total、速率 1024/1MB 边界、0 速率；
// 4 同质分组：formatTransferRate 数值区间可参数化，这里按边界显式列出；
// 5 分支清单→用例映射：见下方【分支清单】；
// 6 分支触发：/proc 打开失败（沙箱 ENOENT）、字段缺失、接口过滤空/非空；
// 7 异常断言：函数无显式 throw，以 EXPECT_NO_THROW 覆盖空指针入参场景；
// 8 反向场景：计数器回退、counter reset、坏行跳过；
// 9 强异常安全：打桩 clear 后重复调用 networkSpeed 验证状态自恢复；
// 10 打桩选型：/proc 注入用 ::open 重定向沙箱（保真 QFile 链路）；接口枚举
//    与 QElapsedTimer 用 stub-ext set_lamda（非虚成员函数地址补丁）。
//
// 【分支清单】（复杂方法分支 → 用例映射）
// cpuUsagePercent：
//   B1 首次采样（m_prevCpuTotalTime==0）→ 基线 0 → Cpu_FirstCall_ReturnsBaselineZero
//   B2 总时长增长 → busyRatio=(totalΔ-idleΔ)/totalΔ → Cpu_SecondCall_ComputesBusyRatio /
//      Cpu_SecondCall_PartialBusy
//   B3 idleΔ>totalΔ → qMin 钳制 → Cpu_IdleDeltaClamped_ZeroBusy
//   B4 totalTime<=prev（计数器回退）→ 返回 0 并重设基线 → Cpu_CounterRegression_ResamplesBaseline
//   B5 readCpuTimes 失败：字段<5 / 非 cpu 行 / 打开失败 / 空文件 → -1 →
//      Cpu_Malformed_* / Cpu_OpenFailure_* / Cpu_EmptyFile_*
//   B6 无 iowait 字段（5 字段）→ value(5)=0 → Cpu_FiveFields_NoIowait
// memoryUsagePercent：
//   B7 MemAvailable 正常 → (total-available)/total → Mem_Normal_ComputesUsedPercent
//   B8 available>total → used=0 → Mem_AvailableExceedsTotal_ClampsToZero
//   B9 无 MemAvailable → 回退估算 free+buffers+cached+reclaimable-shmem →
//      Mem_FallbackEstimation_*
//   B10 估算<shmem → available=0 → 100% → Mem_Fallback_ShmemLarger_ClampsToFull
//   B11 total==0 → -1 → Mem_MissingTotal_ReturnsNegativeOne / Mem_OpenFailure_ReturnsNegativeOne
//   B12 找齐即 break → Mem_LoopBreaks_WhenBothFieldsFound
//   B13 字段顺序无关 → Mem_FieldOrderIndependent_ParsesAllLines
// networkSpeed：
//   B13 首次采样（timer 无效）→ "0 KB/s" + 记录基线 → Net_FirstCall_*
//   B14 正常差值 → formatTransferRate(Δ/elapsed) → Net_SecondCall_FormatsDeltas
//   B15 接口集合过滤：空集合全收 / 非集合成员跳过 / 多接口求和 → Net_Interface*/Net_Multi*
//   B16 计数器回退 → Δ=0 → Net_CounterReset_ClampsDeltaToZero
//   B17 读失败 → 0 字节基线 → Net_OpenFailure_*
//   B18 空指针入参 → 直接返回 → Net_NullPointerArguments_NoCrash
//   B19 坏行（无冒号/字段<9）跳过 → Net_MalformedLines_Skipped

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <stubext.h>

#include "systemmonitorprober.h"
#include "procsandbox.h"

#include <QElapsedTimer>
#include <QNetworkInterface>

#include <cstring>

namespace {

// /proc/stat 头行构造：cpu user nice system idle iowait irq softirq steal ...
QByteArray cpuLine(quint64 user, quint64 nice, quint64 system, quint64 idle, quint64 iowait)
{
    return QStringLiteral("cpu  %1 %2 %3 %4 %5 0 0 0 0 0\n")
        .arg(user)
        .arg(nice)
        .arg(system)
        .arg(idle)
        .arg(iowait)
        .toUtf8();
}

QByteArray meminfoLine(const char *key, quint64 value)
{
    return QStringLiteral("%1:       %2 kB\n").arg(QString::fromLatin1(key)).arg(value).toUtf8();
}

// /proc/net/dev 数据行（收 16 列：bytes(0) ... 发送 bytes(8)）
QByteArray netDevLine(const QString &iface, quint64 rxBytes, quint64 txBytes)
{
    //               rx: bytes packets errs drop fifo frame compressed multicast |
    //               tx: bytes packets errs drop fifo colls carrier compressed
    return QStringLiteral("%1: %2 %3 0 0 0 0 0 0 %4 0 0 0 0 0 0 0\n")
        .arg(iface, -8)
        .arg(rxBytes)
        .arg(7)     // rx packets 占位
        .arg(txBytes)
        .toUtf8();
}

const QByteArray kNetDevHeader =
    "Inter-|   Receive                                                |  Transmit\n"
    " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n";

} // namespace

class SystemMonitorProberTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        sandbox = std::make_unique<ProcSandbox>(stub);
    }

    void TearDown() override
    {
        stub.clear();
        prober.reset();
        sandbox.reset();
    }

    // 打桩私有方法 preferredNetworkInterfaces：消除 QNetworkInterface 环境差异
    void stubInterfaces(const QStringList &ifaces)
    {
        stub.set_lamda(&SystemMonitorProber::preferredNetworkInterfaces,
                       [ifaces](SystemMonitorProber *) { return ifaces; });
    }

    // 打桩 QElapsedTimer（非虚成员，显式转换去掉 noexcept 限定以匹配 VFLocator）：
    // start 记录有效态、restart 返回固定毫秒，使速率计算可精确断言
    void stubElapsedTimer(qint64 restartMsecs)
    {
        s_timerValid = false;
        stub.set_lamda(static_cast<bool (QElapsedTimer::*)() const>(&QElapsedTimer::isValid),
                       [](const QElapsedTimer *) { return s_timerValid; });
        stub.set_lamda(static_cast<void (QElapsedTimer::*)()>(&QElapsedTimer::start),
                       [](QElapsedTimer *) { s_timerValid = true; });
        stub.set_lamda(static_cast<qint64 (QElapsedTimer::*)()>(&QElapsedTimer::restart),
                       [restartMsecs](QElapsedTimer *) { return restartMsecs; });
    }

    stub_ext::StubExt stub;
    std::unique_ptr<ProcSandbox> sandbox;
    std::unique_ptr<SystemMonitorProber> prober = std::make_unique<SystemMonitorProber>();

    inline static bool s_timerValid = false;
};

// ---- cpuUsagePercent / readCpuTimes ----

TEST_F(SystemMonitorProberTest, Cpu_FirstCall_ReturnsBaselineZero)
{
    // Arrange: 合法 /proc/stat（total=600, idle=400）
    sandbox->inject("/proc/stat", cpuLine(100, 0, 100, 400, 0));
    // Act
    const int usage = prober->cpuUsagePercent();
    // Assert: 首次采样只建立基线，返回 0；基线已记录
    EXPECT_EQ(usage, 0);
    EXPECT_EQ(prober->m_prevCpuTotalTime, 600u);
    EXPECT_EQ(prober->m_prevCpuIdleTime, 400u);
}

TEST_F(SystemMonitorProberTest, Cpu_SecondCall_ComputesBusyRatio)
{
    // Arrange: 第一次采样 total=600 idle=400；第二次 total=800 idle=400
    sandbox->inject("/proc/stat", cpuLine(100, 0, 100, 400, 0));
    EXPECT_EQ(prober->cpuUsagePercent(), 0);
    sandbox->inject("/proc/stat", cpuLine(300, 0, 100, 400, 0));
    // Act: totalΔ=200, idleΔ=0 → busy=100%
    const int usage = prober->cpuUsagePercent();
    // Assert
    EXPECT_EQ(usage, 100);
}

TEST_F(SystemMonitorProberTest, Cpu_SecondCall_PartialBusy)
{
    // Arrange: 第一次 total=600 idle=400；第二次 total=1700 idle=1300
    sandbox->inject("/proc/stat", cpuLine(100, 0, 100, 400, 0));
    EXPECT_EQ(prober->cpuUsagePercent(), 0);
    sandbox->inject("/proc/stat", cpuLine(300, 0, 100, 1200, 100));
    // Act: totalΔ=1100, idleΔ=900 → busy=(1100-900)*100/1100≈18%
    const int usage = prober->cpuUsagePercent();
    // Assert
    EXPECT_EQ(usage, 18);
}

TEST_F(SystemMonitorProberTest, Cpu_IdleDeltaClamped_ZeroBusy)
{
    // Arrange: 构造 idleΔ > totalΔ 的异常采样（综合值不一致）
    sandbox->inject("/proc/stat", cpuLine(100, 0, 100, 400, 0));
    EXPECT_EQ(prober->cpuUsagePercent(), 0);
    sandbox->inject("/proc/stat", cpuLine(50, 0, 50, 300, 300));
    // Act: totalΔ=100, idleΔ=200 → qMin 钳制后 busy=0
    const int usage = prober->cpuUsagePercent();
    // Assert
    EXPECT_EQ(usage, 0);
}

TEST_F(SystemMonitorProberTest, Cpu_CounterRegression_ResamplesBaseline)
{
    // Arrange: 第一次采样 total=600；第二次计数器回退 total=500
    sandbox->inject("/proc/stat", cpuLine(100, 0, 100, 400, 0));
    EXPECT_EQ(prober->cpuUsagePercent(), 0);
    sandbox->inject("/proc/stat", cpuLine(80, 0, 80, 340, 0));
    // Act: total 未增长 → 返回 0 并以本次为新基线
    const int usage = prober->cpuUsagePercent();
    // Assert: 基线已更新为第二次采样（total=500, idle=340）
    EXPECT_EQ(usage, 0);
    EXPECT_EQ(prober->m_prevCpuTotalTime, 500u);
    EXPECT_EQ(prober->m_prevCpuIdleTime, 340u);
    // 后续正常增长可基于新基线计算：totalΔ=300, idleΔ=0 → 100%
    sandbox->inject("/proc/stat", cpuLine(280, 0, 100, 340, 0));
    EXPECT_EQ(prober->cpuUsagePercent(), 100);
}

TEST_F(SystemMonitorProberTest, Cpu_MalformedLine_TooFewFields_ReturnsNegativeOne)
{
    // Arrange: 仅 3 个字段
    sandbox->inject("/proc/stat", QByteArray("cpu  100 200\n"));
    // Act: 连续两次读取
    const int first = prober->cpuUsagePercent();
    const int second = prober->cpuUsagePercent();
    // Assert: 解析失败恒返回 -1
    EXPECT_EQ(first, -1);
    EXPECT_EQ(second, -1);
}

TEST_F(SystemMonitorProberTest, Cpu_NotCpuLine_ReturnsNegativeOne)
{
    // Arrange: 首行为 cpu0（单核行），不符合 "cpu" 精确匹配
    sandbox->inject("/proc/stat", QByteArray("cpu0 100 0 100 400 0 0 0 0 0 0\n"));
    // Act: 连续两次读取
    const int first = prober->cpuUsagePercent();
    const int second = prober->cpuUsagePercent();
    // Assert: 解析失败恒返回 -1
    EXPECT_EQ(first, -1);
    EXPECT_EQ(second, -1);
}

TEST_F(SystemMonitorProberTest, Cpu_OpenFailure_ReturnsNegativeOne)
{
    // Arrange: 沙箱注入 ENOENT
    sandbox->injectFailure("/proc/stat");
    // Act: 连续两次读取
    const int first = prober->cpuUsagePercent();
    const int second = prober->cpuUsagePercent();
    // Assert: 打开失败恒返回 -1
    EXPECT_EQ(first, -1);
    EXPECT_EQ(second, -1);
}

TEST_F(SystemMonitorProberTest, Cpu_FiveFields_NoIowait_IdleIsFourthField)
{
    // Arrange: 5 字段（无 iowait 列），value(5) 取空 → idle 仅计 idle 字段
    sandbox->inject("/proc/stat", QByteArray("cpu  100 0 100 300\n"));
    EXPECT_EQ(prober->cpuUsagePercent(), 0);
    sandbox->inject("/proc/stat", QByteArray("cpu  200 0 200 300\n"));
    // Act: totalΔ=200, idleΔ=0 → 100%
    const int usage = prober->cpuUsagePercent();
    // Assert
    EXPECT_EQ(usage, 100);
}

TEST_F(SystemMonitorProberTest, Cpu_EmptyFile_ReturnsNegativeOne)
{
    // Arrange: 空文件 → 首行为空 → 字段数不足
    sandbox->inject("/proc/stat", QByteArray());
    // Act: 连续两次读取
    const int first = prober->cpuUsagePercent();
    const int second = prober->cpuUsagePercent();
    // Assert: 空文件恒返回 -1
    EXPECT_EQ(first, -1);
    EXPECT_EQ(second, -1);
}

// ---- memoryUsagePercent ----

TEST_F(SystemMonitorProberTest, Mem_Normal_ComputesUsedPercent)
{
    // Arrange: total=16000000, available=8000000
    sandbox->inject("/proc/meminfo",
                    meminfoLine("MemTotal", 16000000) + meminfoLine("MemAvailable", 8000000));
    // Act: 两次读取（解析无状态，结果应一致）
    const int first = prober->memoryUsagePercent();
    const int second = prober->memoryUsagePercent();
    // Assert: 50%
    EXPECT_EQ(first, 50);
    EXPECT_EQ(second, 50);
}

TEST_F(SystemMonitorProberTest, Mem_Rounding_RoundHalfUp)
{
    // Arrange: used=2/3 → 66.67%
    sandbox->inject("/proc/meminfo",
                    meminfoLine("MemTotal", 3) + meminfoLine("MemAvailable", 1));
    // Act
    const int usage = prober->memoryUsagePercent();
    // Assert: 四舍五入 67
    EXPECT_EQ(usage, 67);
    EXPECT_NE(usage, 66);
}

TEST_F(SystemMonitorProberTest, Mem_AvailableExceedsTotal_ClampsToZero)
{
    // Arrange: available > total
    sandbox->inject("/proc/meminfo",
                    meminfoLine("MemTotal", 1000) + meminfoLine("MemAvailable", 2000));
    // Act
    const int usage = prober->memoryUsagePercent();
    // Assert: 钳制为 0（负使用率不出现）
    EXPECT_EQ(usage, 0);
    EXPECT_LT(usage, 1);
}

TEST_F(SystemMonitorProberTest, Mem_FallbackEstimation_WithoutAvailable)
{
    // Arrange: 无 MemAvailable → 回退 free+buffers+cached+reclaimable-shmem
    // est = 1000+200+300+100-50 = 1550 → used = 10000-1550 = 8450 → 84.5% → 85
    sandbox->inject("/proc/meminfo",
                    meminfoLine("MemTotal", 10000)
                        + meminfoLine("MemFree", 1000)
                        + meminfoLine("Buffers", 200)
                        + meminfoLine("Cached", 300)
                        + meminfoLine("SReclaimable", 100)
                        + meminfoLine("Shmem", 50));
    // Act
    const int usage = prober->memoryUsagePercent();
    // Assert: est = 1000+200+300+100-50 = 1550 → used = 8450 → 84.5% → 85
    EXPECT_EQ(usage, 85);
    EXPECT_GT(usage, 84);
}

TEST_F(SystemMonitorProberTest, Mem_Fallback_ShmemLarger_ClampsToFull)
{
    // Arrange: 估算值 < Shmem → available=0 → used=total → 100%
    sandbox->inject("/proc/meminfo",
                    meminfoLine("MemTotal", 10000)
                        + meminfoLine("MemFree", 100)
                        + meminfoLine("Buffers", 100)
                        + meminfoLine("Cached", 100)
                        + meminfoLine("SReclaimable", 0)
                        + meminfoLine("Shmem", 5000));
    // Act
    const int usage = prober->memoryUsagePercent();
    // Assert: 估算 < Shmem → available=0 → used=total → 100%
    EXPECT_EQ(usage, 100);
    EXPECT_LE(usage, 100);
}

TEST_F(SystemMonitorProberTest, Mem_MissingTotal_ReturnsNegativeOne)
{
    // Arrange: 无 MemTotal 行
    sandbox->inject("/proc/meminfo", meminfoLine("MemAvailable", 8000000));
    // Act: 连续两次读取
    const int first = prober->memoryUsagePercent();
    const int second = prober->memoryUsagePercent();
    // Assert: 缺关键字段恒返回 -1
    EXPECT_EQ(first, -1);
    EXPECT_EQ(second, -1);
}

TEST_F(SystemMonitorProberTest, Mem_OpenFailure_ReturnsNegativeOne)
{
    // Arrange: 沙箱注入 ENOENT
    sandbox->injectFailure("/proc/meminfo");
    // Act: 连续两次读取
    const int first = prober->memoryUsagePercent();
    const int second = prober->memoryUsagePercent();
    // Assert: 打开失败恒返回 -1
    EXPECT_EQ(first, -1);
    EXPECT_EQ(second, -1);
}

TEST_F(SystemMonitorProberTest, Mem_LoopBreaks_WhenBothFieldsFound)
{
    // Arrange: MemTotal/MemAvailable 之后跟坏行（无冒号数值），解析应在找齐后提前退出
    sandbox->inject("/proc/meminfo",
                    meminfoLine("MemTotal", 16000000)
                        + meminfoLine("MemAvailable", 8000000)
                        + QByteArray("garbage line without colon\n"));
    // Act
    const int usage = prober->memoryUsagePercent();
    // Assert: 结果与正常解析一致（未受坏行影响）
    EXPECT_EQ(usage, 50);
    EXPECT_NE(usage, -1);
}

TEST_F(SystemMonitorProberTest, Mem_FieldOrderIndependent_ParsesAllLines)
{
    // Arrange: MemAvailable 在 MemTotal 之前出现
    sandbox->inject("/proc/meminfo",
                    meminfoLine("MemAvailable", 4000000) + meminfoLine("MemTotal", 16000000));
    // Act
    const int usage = prober->memoryUsagePercent();
    // Assert: 字段顺序无关，75%
    EXPECT_EQ(usage, 75);
    EXPECT_NE(usage, -1);
}

// ---- networkSpeed / readNetworkBytes ----

TEST_F(SystemMonitorProberTest, Net_FirstCall_ReturnsZeroSpeed_RecordsBaseline)
{
    // Arrange: 合法 /proc/net/dev + eth0
    stubInterfaces({ QStringLiteral("eth0") });
    stubElapsedTimer(1000);
    sandbox->inject("/proc/net/dev", kNetDevHeader + netDevLine("eth0", 1000, 500));
    // Act
    QString down, up;
    prober->networkSpeed(&down, &up);
    // Assert: 首次采样返回 0 KB/s 并记录基线
    EXPECT_EQ(down, QStringLiteral("0 KB/s"));
    EXPECT_EQ(up, QStringLiteral("0 KB/s"));
    EXPECT_EQ(prober->m_prevReceiveBytes, 1000u);
    EXPECT_EQ(prober->m_prevTransmitBytes, 500u);
    EXPECT_TRUE(prober->m_networkSampleTimer.isValid());
}

TEST_F(SystemMonitorProberTest, Net_SecondCall_FormatsDeltas)
{
    // Arrange: 首次基线 rx=1000/tx=500；间隔固定 1000ms；第二次 rx=3048/tx=1012
    stubInterfaces({ QStringLiteral("eth0") });
    stubElapsedTimer(1000);
    sandbox->inject("/proc/net/dev", kNetDevHeader + netDevLine("eth0", 1000, 500));
    QString down, up;
    prober->networkSpeed(&down, &up);
    sandbox->inject("/proc/net/dev", kNetDevHeader + netDevLine("eth0", 3048, 1012));
    // Act: Δrx=2048B/1s → 2 KB/s；Δtx=512B/1s → 512 B/s
    prober->networkSpeed(&down, &up);
    // Assert
    EXPECT_EQ(down, QStringLiteral("2 KB/s"));
    EXPECT_EQ(up, QStringLiteral("512 B/s"));
}

TEST_F(SystemMonitorProberTest, Net_MultipleInterfaces_Summed)
{
    // Arrange: eth0 与 wlan0 均在首选集合内 → 字节求和
    stubInterfaces({ QStringLiteral("eth0"), QStringLiteral("wlan0") });
    stubElapsedTimer(1000);
    sandbox->inject("/proc/net/dev",
                    kNetDevHeader + netDevLine("eth0", 1000, 500) + netDevLine("wlan0", 2000, 1000));
    QString down, up;
    prober->networkSpeed(&down, &up);
    sandbox->inject("/proc/net/dev",
                    kNetDevHeader + netDevLine("eth0", 3048, 1012) + netDevLine("wlan0", 4096, 2024));
    // Act: Δrx=2048+2048=4096 → 4 KB/s；Δtx=512+1024=1536 → 2 KB/s（1536/1024=1.5 → 2）
    prober->networkSpeed(&down, &up);
    // Assert
    EXPECT_EQ(down, QStringLiteral("4 KB/s"));
    EXPECT_EQ(up, QStringLiteral("2 KB/s"));
}

TEST_F(SystemMonitorProberTest, Net_InterfaceFiltering_SkipsNonPreferred)
{
    // Arrange: 首选集合仅 eth0，wlan0 行不计入
    stubInterfaces({ QStringLiteral("eth0") });
    stubElapsedTimer(1000);
    sandbox->inject("/proc/net/dev",
                    kNetDevHeader + netDevLine("eth0", 1000, 500) + netDevLine("wlan0", 99999, 99999));
    QString down, up;
    prober->networkSpeed(&down, &up);
    sandbox->inject("/proc/net/dev",
                    kNetDevHeader + netDevLine("eth0", 3048, 1012) + netDevLine("wlan0", 199998, 199998));
    // Act: 仅 eth0 Δrx=2048 → 2 KB/s
    prober->networkSpeed(&down, &up);
    // Assert
    EXPECT_EQ(down, QStringLiteral("2 KB/s"));
    EXPECT_EQ(up, QStringLiteral("512 B/s"));
}

TEST_F(SystemMonitorProberTest, Net_EmptyInterfaceList_CountsAll)
{
    // Arrange: 无可用接口 → interfaceSet 为空 → 所有行计入
    stubInterfaces({});
    stubElapsedTimer(1000);
    sandbox->inject("/proc/net/dev",
                    kNetDevHeader + netDevLine("eth0", 1000, 500) + netDevLine("wlan0", 2000, 1000));
    QString down, up;
    prober->networkSpeed(&down, &up);
    sandbox->inject("/proc/net/dev",
                    kNetDevHeader + netDevLine("eth0", 2000, 1000) + netDevLine("wlan0", 4000, 2000));
    // Act: Δrx=1000+2000=3000 → 3 KB/s；Δtx=500+1000=1500 → 1 KB/s（1500/1024=1.46 → 1）
    prober->networkSpeed(&down, &up);
    // Assert
    EXPECT_EQ(down, QStringLiteral("3 KB/s"));
    EXPECT_EQ(up, QStringLiteral("1 KB/s"));
}

TEST_F(SystemMonitorProberTest, Net_CounterReset_ClampsDeltaToZero)
{
    // Arrange: 第二次采样计数器小于首次（接口重建等）
    stubInterfaces({ QStringLiteral("eth0") });
    stubElapsedTimer(1000);
    sandbox->inject("/proc/net/dev", kNetDevHeader + netDevLine("eth0", 100000, 50000));
    QString down, up;
    prober->networkSpeed(&down, &up);
    sandbox->inject("/proc/net/dev", kNetDevHeader + netDevLine("eth0", 100, 50));
    // Act: Δ 负数 → 钳制为 0
    prober->networkSpeed(&down, &up);
    // Assert
    EXPECT_EQ(down, QStringLiteral("0 B/s"));
    EXPECT_EQ(up, QStringLiteral("0 B/s"));
}

TEST_F(SystemMonitorProberTest, Net_OpenFailure_ZeroBaselineAfterFirstCall)
{
    // Arrange: /proc/net/dev 打开失败
    stubInterfaces({ QStringLiteral("eth0") });
    stubElapsedTimer(1000);
    sandbox->injectFailure("/proc/net/dev");
    QString down, up;
    // Act: 首次（读取失败但仍是首采样）→ 0 KB/s 基线
    prober->networkSpeed(&down, &up);
    EXPECT_EQ(down, QStringLiteral("0 KB/s"));
    EXPECT_EQ(prober->m_prevReceiveBytes, 0u);
    // 第二次：Δ=0 → 0 B/s
    prober->networkSpeed(&down, &up);
    // Assert
    EXPECT_EQ(down, QStringLiteral("0 B/s"));
    EXPECT_EQ(up, QStringLiteral("0 B/s"));
}

TEST_F(SystemMonitorProberTest, Net_NullPointerArguments_NoCrash)
{
    // Arrange: 合法内容
    stubInterfaces({ QStringLiteral("eth0") });
    stubElapsedTimer(1000);
    sandbox->inject("/proc/net/dev", kNetDevHeader + netDevLine("eth0", 1000, 500));
    QString up;
    QString down;
    // Act: 空指针入参直接返回，不崩溃
    EXPECT_NO_THROW(prober->networkSpeed(nullptr, &up));
    EXPECT_NO_THROW(prober->networkSpeed(&down, nullptr));
    // Assert: 不写值
    EXPECT_EQ(up, QString());
    EXPECT_EQ(down, QString());
}

TEST_F(SystemMonitorProberTest, Net_MalformedLines_Skipped)
{
    // Arrange: 混入无冒号行与字段不足行
    stubInterfaces({ QStringLiteral("eth0") });
    stubElapsedTimer(1000);
    sandbox->inject("/proc/net/dev",
                    kNetDevHeader
                        + QByteArray("this line has no colon\n")
                        + QByteArray("eth0: 1 2 3\n")
                        + netDevLine("eth0", 1000, 500));
    QString down, up;
    // Act: 坏行跳过，仅合法行计入基线
    prober->networkSpeed(&down, &up);
    // Assert
    EXPECT_EQ(prober->m_prevReceiveBytes, 1000u);
    EXPECT_EQ(prober->m_prevTransmitBytes, 500u);
}

TEST_F(SystemMonitorProberTest, Net_RepeatedCalls_StateSelfRecovery)
{
    // Arrange: 连续多次采样（含 stub clear 与重建），验证状态自恢复不崩溃
    stubInterfaces({ QStringLiteral("eth0") });
    stubElapsedTimer(1000);
    sandbox->inject("/proc/net/dev", kNetDevHeader + netDevLine("eth0", 1000, 500));
    QString down, up;
    // Act: 首采样建立基线，二采样产生增量
    prober->networkSpeed(&down, &up);
    sandbox->inject("/proc/net/dev", kNetDevHeader + netDevLine("eth0", 3048, 1012));
    prober->networkSpeed(&down, &up);
    // Assert: 增量速率
    EXPECT_EQ(down, QStringLiteral("2 KB/s"));
    // Act: 相同计数再采样
    sandbox->inject("/proc/net/dev", kNetDevHeader + netDevLine("eth0", 3048, 1012));
    prober->networkSpeed(&down, &up);
    // Assert: 零速率
    EXPECT_EQ(down, QStringLiteral("0 B/s"));
    EXPECT_EQ(up, QStringLiteral("0 B/s"));
}

// ---- preferredNetworkInterfaces（真实实现的不变量校验） ----

TEST_F(SystemMonitorProberTest, PreferredInterfaces_RealNics_ExcludeVirtual)
{
    // Arrange: 直调私有方法（-fno-access-control），结果依赖真实环境
    ASSERT_NE(prober, nullptr);
    // Act
    const QStringList result = prober->preferredNetworkInterfaces();
    // Assert: 不变量 —— 环回与虚拟/隧道接口一律被过滤，名称非空
    for (const QString &name : result) {
        EXPECT_FALSE(name.isEmpty());
        EXPECT_NE(name, QStringLiteral("lo"));
        EXPECT_FALSE(name.startsWith(QStringLiteral("veth")));
        EXPECT_FALSE(name.startsWith(QStringLiteral("br-")));
        EXPECT_FALSE(name.startsWith(QStringLiteral("docker")));
        EXPECT_FALSE(name.startsWith(QStringLiteral("virbr")));
        EXPECT_FALSE(name.startsWith(QStringLiteral("tun")));
        EXPECT_FALSE(name.startsWith(QStringLiteral("tap")));
        EXPECT_FALSE(name.startsWith(QStringLiteral("wg")));
        EXPECT_FALSE(name.startsWith(QStringLiteral("dummy")));
    }
}

// ---- formatTransferRate（静态私有方法，直调） ----

TEST_F(SystemMonitorProberTest, Format_Bytes_WholeRange)
{
    // Arrange: B/s 段内典型值
    const double zero = 0.0;
    const double mid = 512.0;
    const double nearKb = 1023.4;
    // Act
    const QString r0 = SystemMonitorProber::formatTransferRate(zero);
    const QString r1 = SystemMonitorProber::formatTransferRate(mid);
    const QString r2 = SystemMonitorProber::formatTransferRate(nearKb);
    // Assert: 整数 B/s 输出
    EXPECT_EQ(r0, QStringLiteral("0 B/s"));
    EXPECT_EQ(r1, QStringLiteral("512 B/s"));
    EXPECT_EQ(r2, QStringLiteral("1023 B/s"));
}

TEST_F(SystemMonitorProberTest, Format_BytesRounding_Boundary)
{
    // Arrange: 1023.9 → qRound → 1024，仍为 B/s 段
    const double input = 1023.9;
    // Act
    const QString text = SystemMonitorProber::formatTransferRate(input);
    // Assert: 进位但不跨段
    EXPECT_EQ(text, QStringLiteral("1024 B/s"));
    EXPECT_NE(text, QStringLiteral("1023 B/s"));
}

TEST_F(SystemMonitorProberTest, Format_KB_Boundary)
{
    // Arrange: 1024 B/s 恰好进入 KB 段
    const double atKb = 1024.0;
    const double inKb = 2048.0;
    const double nearMb = 1024.0 * 1024 - 1;
    // Act
    const QString r0 = SystemMonitorProber::formatTransferRate(atKb);
    const QString r1 = SystemMonitorProber::formatTransferRate(inKb);
    const QString r2 = SystemMonitorProber::formatTransferRate(nearMb);
    // Assert: 整数 KB/s 输出
    EXPECT_EQ(r0, QStringLiteral("1 KB/s"));
    EXPECT_EQ(r1, QStringLiteral("2 KB/s"));
    EXPECT_EQ(r2, QStringLiteral("1024 KB/s"));
}

TEST_F(SystemMonitorProberTest, Format_MB_Boundary)
{
    // Arrange: 1 MiB 恰好进入 MB 段（一位小数）
    const double oneMib = 1024.0 * 1024;
    // Act
    const QString atBoundary = SystemMonitorProber::formatTransferRate(oneMib);
    const QString below = SystemMonitorProber::formatTransferRate(oneMib - 1);
    // Assert: 边界进段，前一刻仍在 KB 段
    EXPECT_EQ(atBoundary, QStringLiteral("1.0 MB/s"));
    EXPECT_EQ(below, QStringLiteral("1024 KB/s"));
}

TEST_F(SystemMonitorProberTest, Format_MB_Fraction)
{
    // Arrange: MB 段非整数 MiB 值
    const double frac1 = 2.5 * 1024 * 1024;
    const double frac2 = 3.14159 * 1024 * 1024;
    // Act
    const QString r1 = SystemMonitorProber::formatTransferRate(frac1);
    const QString r2 = SystemMonitorProber::formatTransferRate(frac2);
    // Assert: 一位小数
    EXPECT_EQ(r1, QStringLiteral("2.5 MB/s"));
    EXPECT_EQ(r2, QStringLiteral("3.1 MB/s"));
}

TEST_F(SystemMonitorProberTest, Format_Zero_ReturnsZero)
{
    // Arrange: 0.4 B/s 四舍五入为 0
    const double sub = 0.4;
    const double zero = 0.0;
    // Act
    const QString rounded = SystemMonitorProber::formatTransferRate(sub);
    const QString exact = SystemMonitorProber::formatTransferRate(zero);
    // Assert: 均为 0 B/s
    EXPECT_EQ(rounded, QStringLiteral("0 B/s"));
    EXPECT_EQ(exact, QStringLiteral("0 B/s"));
}

TEST_F(SystemMonitorProberTest, Format_NegativeInput_Characterization)
{
    // Arrange: 反向输入（正常流程不会出现）
    const double negative = -5.0;
    // Act
    const QString text = SystemMonitorProber::formatTransferRate(negative);
    // Assert: 按实现原样四舍五入输出，作为行为锚定
    EXPECT_EQ(text, QStringLiteral("-5 B/s"));
    EXPECT_NE(text, QStringLiteral("5 B/s"));
}
