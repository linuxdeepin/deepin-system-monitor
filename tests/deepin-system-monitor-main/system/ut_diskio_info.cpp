// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/diskio_info.h"
#include "system/sys_info.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QFile>
#include <QTemporaryDir>

//c
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>

using namespace core::system;

/***************************************STUB begin*********************************************/

// fopen —— 将 /proc/diskstats 重定向到测试临时文件（open+fdopen，避免递归）
static char g_diskstatsPath[512] = { 0 };
static bool g_fopenFail = false;
static FILE *stub_fopen(const char *filename, const char *modes)
{
    if (g_fopenFail || !filename || strcmp(filename, "/proc/diskstats") != 0)
        return nullptr;
    int fd = open(g_diskstatsPath, O_RDONLY);
    if (fd < 0)
        return nullptr;
    return fdopen(fd, modes);
}

// access —— 控制 is_block_dev 对 /sys/block/<dev> 的存在性判断
static bool g_accessDeny = false;
static int stub_access(const char *, int)
{
    return g_accessDeny ? -1 : 0;
}

// SysInfo::uptime —— 固定时钟
static timeval g_uptime = { 0, 0 };
static timeval stub_SysInfo_uptime()
{
    return g_uptime;
}

/***************************************STUB end***********************************************/

class UT_DiskIOInfo : public ::testing::Test
{
public:
    UT_DiskIOInfo() : m_tester(nullptr) {}

    virtual void SetUp()
    {
        g_fopenFail = false;
        g_accessDeny = false;
        g_uptime = timeval { 0, 0 };
        g_diskstatsPath[0] = '\0';
        m_tester = new DiskIOInfo();
    }

    virtual void TearDown()
    {
        delete m_tester;
        m_tester = nullptr;
    }

    // 写入受控的 /proc/diskstats 内容
    void writeDiskstats(const QStringList &lines)
    {
        const QString path = m_tmpDir.path() + "/diskstats";
        QFile f(path);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        for (const QString &line : lines)
            f.write((line + "\n").toUtf8());
        f.close();
        strncpy(g_diskstatsPath, path.toUtf8().constData(), sizeof(g_diskstatsPath) - 1);
    }

    // 构造一条 17 列 diskstats 行
    static QString statsLine(const char *name, quint64 readIos, quint64 readSectors,
                             quint64 writeIos, quint64 writeSectors,
                             quint64 discardIos, quint64 discardSectors)
    {
        return QString("   8        0 %1 %2 5 %3 7 %4 9 %5 0 0 0 0 %6 1 %7")
                .arg(name)
                .arg(readIos)
                .arg(readSectors)
                .arg(writeIos)
                .arg(writeSectors)
                .arg(discardIos)
                .arg(discardSectors);
    }

    static std::shared_ptr<disk_io_stat> makeStat(quint64 readSectors, quint64 writeSectors, quint64 discardSectors)
    {
        auto stat = std::make_shared<disk_io_stat>();
        memset(stat.get(), 0, sizeof(disk_io_stat));
        stat->read_sectors = readSectors;
        stat->write_sectors = writeSectors;
        stat->discard_sectors = discardSectors;
        return stat;
    }

protected:
    DiskIOInfo *m_tester;
    QTemporaryDir m_tmpDir;
};

/**
 * @brief 默认构造：读写速率为 0
 * 覆盖分支: DiskIOInfo 构造 → m_readBps/m_writeBps 零初始化
 */
TEST_F(UT_DiskIOInfo, initTest)
{
    // Arrange: SetUp 已创建对象
    // Act & Assert
    EXPECT_DOUBLE_EQ(m_tester->diskIoReadBps(), 0.0);
    EXPECT_DOUBLE_EQ(m_tester->diskIoWriteBps(), 0.0);
}

/**
 * @brief readDiskIOStats 解析块设备行并落入当前快照
 * 覆盖分支: readDiskIOStats:
 *   [fopen success]; [rc >= 7 → 有效行]; [is_block_dev access==0 → 入表];
 *   [timevalList[DiskIOInfo::kLastStat] ← [DiskIOInfo::kCurrentStat] 快照前移]; [ferror → b 保持 true]
 */
TEST_F(UT_DiskIOInfo, test_readDiskIOStats_parsesBlockDev)
{
    // Arrange
    writeDiskstats(QStringList()
                   << statsLine("sda", 100, 200, 50, 100, 3, 60)
                   << statsLine("sda1", 50, 100, 25, 50, 1, 30));
    g_uptime = timeval { 100, 0 };
    Stub stub;
    stub.set(fopen, stub_fopen);
    stub.set(access, stub_access);
    stub.set(ADDR(SysInfo, uptime), stub_SysInfo_uptime);
    // Act
    m_tester->readDiskIOStats();
    // Assert: 两台设备均入当前快照，字段解析正确
    const QMap<QString, std::shared_ptr<disk_io_stat>> &cur = m_tester->m_diskIoStatMap[DiskIOInfo::kCurrentStat];
    ASSERT_TRUE(cur.contains("sda"));
    ASSERT_TRUE(cur.contains("sda1"));
    EXPECT_EQ(cur["sda"]->read_ios, 100u);
    EXPECT_EQ(cur["sda"]->read_sectors, 200u);
    EXPECT_EQ(cur["sda"]->write_ios, 50u);
    EXPECT_EQ(cur["sda"]->write_sectors, 100u);
    EXPECT_EQ(cur["sda"]->discard_ios, 3u);
    EXPECT_EQ(cur["sda"]->discard_sectors, 60u);
    EXPECT_EQ(m_tester->timevalList[DiskIOInfo::kCurrentStat].tv_sec, 100);
    EXPECT_TRUE(m_tester->m_diskIoStatMap[DiskIOInfo::kLastStat].isEmpty());   // 首轮历史快照为空
}

/**
 * @brief 非块设备（access 拒绝）不入表
 * 覆盖分支: readDiskIOStats: [is_block_dev access != 0 → 跳过该行]; [无有效行 → b=false 告警分支]
 */
TEST_F(UT_DiskIOInfo, test_readDiskIOStats_partitionFiltered)
{
    // Arrange
    writeDiskstats(QStringList() << statsLine("sda1", 50, 100, 25, 50, 1, 30));
    Stub stub;
    stub.set(fopen, stub_fopen);
    stub.set(access, stub_access);
    stub.set(ADDR(SysInfo, uptime), stub_SysInfo_uptime);
    g_accessDeny = true;
    // Act
    m_tester->readDiskIOStats();
    // Assert
    EXPECT_TRUE(m_tester->m_diskIoStatMap[DiskIOInfo::kCurrentStat].isEmpty());
}

/**
 * @brief /proc/diskstats 打不开：直接返回，不崩溃
 * 覆盖分支: readDiskIOStats: [fopen == nullptr → return（strerror 告警）]
 */
TEST_F(UT_DiskIOInfo, test_readDiskIOStats_fopenFail)
{
    // Arrange
    g_fopenFail = true;
    Stub stub;
    stub.set(fopen, stub_fopen);
    stub.set(ADDR(SysInfo, uptime), stub_SysInfo_uptime);
    // Act
    m_tester->readDiskIOStats();
    // Assert
    EXPECT_TRUE(m_tester->m_diskIoStatMap[DiskIOInfo::kCurrentStat].isEmpty());
    EXPECT_TRUE(m_tester->m_diskIoStatMap[DiskIOInfo::kLastStat].isEmpty());
}

/**
 * @brief 连续两次采集：当前快照前移为历史快照
 * 覆盖分支: readDiskIOStats:
 *   [kLastStat ← kCurrentStat]; [m_diskIoStatMap[DiskIOInfo::kLastStat] = 上轮内容]; [kCurrentStat 清空重建]
 */
TEST_F(UT_DiskIOInfo, test_readDiskIOStats_shiftsHistory)
{
    // Arrange
    Stub stub;
    stub.set(fopen, stub_fopen);
    stub.set(access, stub_access);
    stub.set(ADDR(SysInfo, uptime), stub_SysInfo_uptime);
    g_uptime = timeval { 100, 0 };
    writeDiskstats(QStringList() << statsLine("sda", 100, 200, 50, 100, 3, 60));
    // Act: 第一次采集
    m_tester->readDiskIOStats();
    // Assert: 当前快照 200
    EXPECT_EQ(m_tester->m_diskIoStatMap[DiskIOInfo::kCurrentStat]["sda"]->read_sectors, 200u);
    // Act: 第二次采集（内容变为 300）
    g_uptime = timeval { 102, 0 };
    writeDiskstats(QStringList() << statsLine("sda", 120, 300, 60, 150, 4, 70));
    m_tester->readDiskIOStats();
    // Assert: 上轮 200 前移为历史，当前为 300
    EXPECT_EQ(m_tester->m_diskIoStatMap[DiskIOInfo::kLastStat]["sda"]->read_sectors, 200u);
    EXPECT_EQ(m_tester->m_diskIoStatMap[DiskIOInfo::kCurrentStat]["sda"]->read_sectors, 300u);
    EXPECT_EQ(m_tester->timevalList[DiskIOInfo::kLastStat].tv_sec, 100);
    EXPECT_EQ(m_tester->timevalList[DiskIOInfo::kCurrentStat].tv_sec, 102);
}

/**
 * @brief calDiskIoStates 差分速率计算
 * 覆盖分支: calDiskIoStates:
 *   [累计 cur/prev 扇区]; [cur > prev → rdiff/wdiff/ddiff = 差值];
 *   [rtime > ltime → interval = 2]; wsize = (wdiff + ddiff) * SECTOR_SIZE
 */
TEST_F(UT_DiskIOInfo, test_calDiskIoStates_computesRates)
{
    // Arrange
    m_tester->m_diskIoStatMap[DiskIOInfo::kLastStat]["sda"] = makeStat(100, 50, 0);
    m_tester->m_diskIoStatMap[DiskIOInfo::kCurrentStat]["sda"] = makeStat(200, 100, 10);
    m_tester->timevalList[DiskIOInfo::kLastStat] = timeval { 100, 0 };
    m_tester->timevalList[DiskIOInfo::kCurrentStat] = timeval { 102, 0 };
    // Act
    m_tester->calDiskIoStates();
    // Assert: read = (200-100)*512/2 = 25600；write = ((100-50)+(10-0))*512/2 = 15360
    EXPECT_DOUBLE_EQ(m_tester->diskIoReadBps(), 25600.0);
    EXPECT_DOUBLE_EQ(m_tester->diskIoWriteBps(), 15360.0);
}

/**
 * @brief calDiskIoStates 计数器回绕按 0 增量；丢弃扇区计入写方向
 * 覆盖分支: calDiskIoStates: [cur <= prev → 差值取 0]；ddiff 只计入 wsize
 */
TEST_F(UT_DiskIOInfo, test_calDiskIoStates_counterRollover)
{
    // Arrange: 读/写均回绕，仅丢弃扇区有增量
    m_tester->m_diskIoStatMap[DiskIOInfo::kLastStat]["sda"] = makeStat(500, 100, 0);
    m_tester->m_diskIoStatMap[DiskIOInfo::kCurrentStat]["sda"] = makeStat(100, 50, 5);
    m_tester->timevalList[DiskIOInfo::kLastStat] = timeval { 100, 0 };
    m_tester->timevalList[DiskIOInfo::kCurrentStat] = timeval { 102, 0 };
    // Act
    m_tester->calDiskIoStates();
    // Assert: readBps=0；writeBps = (0+5)*512/2 = 1280
    EXPECT_DOUBLE_EQ(m_tester->diskIoReadBps(), 0.0);
    EXPECT_DOUBLE_EQ(m_tester->diskIoWriteBps(), 1280.0);
}

/**
 * @brief calDiskIoStates 时间未推进时 interval 回退为 1（不除零）
 * 覆盖分支: calDiskIoStates: [rtime == ltime → interval = 1]
 */
TEST_F(UT_DiskIOInfo, test_calDiskIoStates_zeroIntervalFallback)
{
    // Arrange
    m_tester->m_diskIoStatMap[DiskIOInfo::kLastStat]["sda"] = makeStat(0, 0, 0);
    m_tester->m_diskIoStatMap[DiskIOInfo::kCurrentStat]["sda"] = makeStat(10, 0, 0);
    m_tester->timevalList[DiskIOInfo::kLastStat] = timeval { 100, 0 };
    m_tester->timevalList[DiskIOInfo::kCurrentStat] = timeval { 100, 0 };   // 与上次相同
    // Act
    m_tester->calDiskIoStates();
    // Assert: readBps = 10*512/1 = 5120
    EXPECT_DOUBLE_EQ(m_tester->diskIoReadBps(), 5120.0);
    EXPECT_DOUBLE_EQ(m_tester->diskIoWriteBps(), 0.0);
}

/**
 * @brief update() = readDiskIOStats + calDiskIoStates 端到端
 * 覆盖分支: update: [readDiskIOStats 委托] + [calDiskIoStates 委托 → 速率落库]
 */
TEST_F(UT_DiskIOInfo, test_update_computesBps)
{
    // Arrange
    writeDiskstats(QStringList() << statsLine("sda", 100, 200, 50, 100, 3, 60));
    Stub stub;
    stub.set(fopen, stub_fopen);
    stub.set(access, stub_access);
    stub.set(ADDR(SysInfo, uptime), stub_SysInfo_uptime);
    // Act: 第一次 update（历史为空 → 全量差分，interval 回退 1）
    m_tester->update();
    // Assert: read = 200*512；write = (100+60)*512
    EXPECT_DOUBLE_EQ(m_tester->diskIoReadBps(), 200.0 * 512);
    EXPECT_DOUBLE_EQ(m_tester->diskIoWriteBps(), 160.0 * 512);
    // Act: 第二次 update（内容不变 → 差分为 0）
    m_tester->update();
    // Assert
    EXPECT_DOUBLE_EQ(m_tester->diskIoReadBps(), 0.0);
    EXPECT_DOUBLE_EQ(m_tester->diskIoWriteBps(), 0.0);
}
