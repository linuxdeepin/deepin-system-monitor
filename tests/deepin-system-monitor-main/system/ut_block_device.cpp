// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/block_device.h"
#include "system/private/block_device_p.h"
#include "system/sys_info.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QString>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QDateTime>

//c
#include <sys/time.h>

using namespace core::system;

/***************************************STUB begin*********************************************/

// QFile::open(bool(OpenMode)) —— 控制被测代码能否打开 /proc、/sys 下的真实文件
static bool g_openResult = false;
static int g_openCalls = 0;
typedef bool (*QFileOpenPtr)(QFile *, QIODevice::OpenMode);
static bool stub_QFile_open(QFile *, QIODevice::OpenMode)
{
    ++g_openCalls;
    return g_openResult;
}

// QTextStream::readLine(qint64) —— 提供受控的 /proc/diskstats 行数据
static QStringList g_tsLines;
static int g_tsIdx = 0;
static QString stub_QTextStream_readLine(qint64)
{
    if (g_tsIdx >= g_tsLines.size())
        return QString();   // isNull() == true，结束被测 do-while 循环
    return g_tsLines[g_tsIdx++];
}

// QIODevice::readLine(qint64) —— 提供受控的 /sys/block/%1/{size,device/model} 单行数据
static QByteArray g_ioLine;
static int g_ioLineCalls = 0;
static QByteArray stub_QIODevice_readLine(qint64)
{
    ++g_ioLineCalls;
    return g_ioLine;
}

// SysInfo::uptime —— 固定时钟，保证速率计算确定性
static timeval g_uptime = { 0, 0 };
static timeval stub_SysInfo_uptime()
{
    return g_uptime;
}

// BlockDevice::readDeviceSize —— 隔离 readDiskInfo 对 /sys 的读取
static quint64 g_devSize = 0;
static int g_readSizeCalls = 0;
static quint64 stub_BlockDevice_readDeviceSize(const QString &)
{
    ++g_readSizeCalls;
    return g_devSize;
}

// BlockDevice::readDeviceModel —— 隔离 readDeviceInfo 内部的 model 读取
static int g_readModelCalls = 0;
static void stub_BlockDevice_readDeviceModel()
{
    ++g_readModelCalls;
}

/***************************************STUB end***********************************************/

class UT_BlockDevice : public ::testing::Test
{
public:
    UT_BlockDevice() : m_tester(nullptr) {}

    virtual void SetUp()
    {
        g_openResult = false;
        g_openCalls = 0;
        g_tsLines.clear();
        g_tsIdx = 0;
        g_ioLine.clear();
        g_ioLineCalls = 0;
        g_uptime = timeval { 0, 0 };
        g_devSize = 0;
        g_readSizeCalls = 0;
        g_readModelCalls = 0;
        m_tester = new BlockDevice();
    }

    virtual void TearDown()
    {
        delete m_tester;
        m_tester = nullptr;
    }

    // 构造一条 17 列的 diskstats 行：[2]=name [3]=reads [4]=read_merged [5]=read_sectors
    // [7]=writes [8]=write_merged [9]=write_sectors [16]=discard_sectors
    static QString diskstatsLine(const char *name, quint64 reads, quint64 readMerged,
                                 quint64 readSectors, quint64 writes, quint64 writeMerged,
                                 quint64 writeSectors, quint64 discardSectors)
    {
        QStringList fields;
        fields << "8" << "0" << name
               << QString::number(reads) << QString::number(readMerged) << QString::number(readSectors) << "0"
               << QString::number(writes) << QString::number(writeMerged) << QString::number(writeSectors)
               << "0" << "0" << "0" << "0" << "0" << "0"
               << QString::number(discardSectors);
        return "   8        0 " + fields.mid(2).join(" ");
    }

protected:
    BlockDevice *m_tester;
};

/**
 * @brief 默认构造状态检查
 * 覆盖分支: BlockDevice 默认构造 → 各计数字段零值
 */
TEST_F(UT_BlockDevice, initTest)
{
    // Arrange: SetUp 已创建默认对象
    // Act & Assert: 默认状态全部为零值/空值
    EXPECT_TRUE(m_tester->deviceName().isEmpty());
    EXPECT_EQ(m_tester->capacity(), 0u);
    EXPECT_TRUE(m_tester->model().isEmpty());
    EXPECT_EQ(m_tester->readSpeed(), 0u);
    EXPECT_EQ(m_tester->writeSpeed(), 0u);
    EXPECT_EQ(m_tester->blocksRead(), 0u);
    EXPECT_EQ(m_tester->blocksWritten(), 0u);
    EXPECT_EQ(m_tester->bytesRead(), 0u);
    EXPECT_EQ(m_tester->bytesWritten(), 0u);
    EXPECT_EQ(m_tester->percentUtilization(), 0.0);
}

/**
 * @brief 拷贝构造：共享数据 + 写时分离
 * 覆盖分支: BlockDevice(const BlockDevice&) → d(other.d)；写时拷贝分离
 */
TEST_F(UT_BlockDevice, test_copy_ctor)
{
    // Arrange
    {
        Stub stub;
        stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);   // open 失败，readDeviceInfo 早退
        g_openResult = false;
        m_tester->setDeviceName("sda");
    }
    // Act
    BlockDevice copy(*m_tester);
    {
        Stub stub;
        stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
        m_tester->setDeviceName("sdb");                 // 修改原对象，触发写时分离
    }
    // Assert: 副本保留旧值，原对象更新为新值
    EXPECT_EQ(copy.deviceName(), QByteArray("sda"));
    EXPECT_EQ(m_tester->deviceName(), QByteArray("sdb"));
}

/**
 * @brief 赋值运算符：与拷贝构造相同的共享语义
 * 覆盖分支: BlockDevice::operator= → d = other.d；写时拷贝分离
 */
TEST_F(UT_BlockDevice, test_assign_operator)
{
    // Arrange
    {
        Stub stub;
        stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
        m_tester->setDeviceName("sda");
    }
    BlockDevice other;
    // Act
    other = *m_tester;
    {
        Stub stub;
        stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
        m_tester->setDeviceName("sdc");
    }
    // Assert
    EXPECT_EQ(other.deviceName(), QByteArray("sda"));
    EXPECT_EQ(m_tester->deviceName(), QByteArray("sdc"));
}

/**
 * @brief readDeviceSize 正常读取：size 文件内容 × SECTOR_SIZE
 * 覆盖分支: readDeviceSize: [QFile::open success → size = toULongLong * SECTOR_SIZE]
 */
TEST_F(UT_BlockDevice, test_readDeviceSize_ok)
{
    Stub stub;
    // Arrange
    g_openResult = true;
    g_ioLine = "125\n";
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    stub.set((QByteArray(QIODevice::*)(qint64))ADDR(QIODevice, readLine), stub_QIODevice_readLine);
    // Act
    quint64 size = m_tester->readDeviceSize("sda");
    // Assert: 125 扇区 × 512 字节
    EXPECT_EQ(size, 125u * 512);
    EXPECT_EQ(g_openCalls, 1);
    EXPECT_EQ(g_ioLineCalls, 1);
}

/**
 * @brief readDeviceSize 打不开文件返回 0
 * 覆盖分支: readDeviceSize: [QFile::open fail → return 0]
 */
TEST_F(UT_BlockDevice, test_readDeviceSize_openFail)
{
    Stub stub;
    // Arrange
    g_openResult = false;
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    // Act
    quint64 size = m_tester->readDeviceSize("sda");
    // Assert
    EXPECT_EQ(size, 0u);
    EXPECT_EQ(g_openCalls, 1);
}

/**
 * @brief readDeviceSize 非数字内容返回 0
 * 覆盖分支: readDeviceSize: [open success → toULongLong 转换失败 → 0 * SECTOR_SIZE]
 */
TEST_F(UT_BlockDevice, test_readDeviceSize_garbageContent)
{
    Stub stub;
    // Arrange
    g_openResult = true;
    g_ioLine = "not-a-number\n";
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    stub.set((QByteArray(QIODevice::*)(qint64))ADDR(QIODevice, readLine), stub_QIODevice_readLine);
    // Act
    quint64 size = m_tester->readDeviceSize("sda");
    // Assert
    EXPECT_EQ(size, 0u);
}

/**
 * @brief readDeviceModel 正常读取：去除换行并 trim
 * 覆盖分支: readDeviceModel: [QFile::open success → model = readLine().replace("\n","").trimmed()]
 */
TEST_F(UT_BlockDevice, test_readDeviceModel_ok)
{
    Stub stub;
    // Arrange
    m_tester->d->name = "sda";
    g_openResult = true;
    g_ioLine = "  QEMU HARDDISK \n";
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    stub.set((QByteArray(QIODevice::*)(qint64))ADDR(QIODevice, readLine), stub_QIODevice_readLine);
    // Act
    m_tester->readDeviceModel();
    // Assert
    EXPECT_EQ(m_tester->model(), QString("QEMU HARDDISK"));
}

/**
 * @brief readDeviceModel 打不开文件：model 保持为空，不崩溃
 * 覆盖分支: readDeviceModel: [QFile::open fail → return（model 不写入）]
 */
TEST_F(UT_BlockDevice, test_readDeviceModel_openFail)
{
    Stub stub;
    // Arrange
    m_tester->d->name = "sda";
    g_openResult = false;
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    // Act
    m_tester->readDeviceModel();
    // Assert
    EXPECT_TRUE(m_tester->model().isEmpty());
}

/**
 * @brief readDeviceInfo 解析 diskstats 并计算速率（两次采样）
 * 覆盖分支: readDeviceInfo:
 *   [open fail → return] 不在本用例;
 *   [line contains "loop" → 跳过] 用 loop 行过滤;
 *   [deviceInfo.size()>16 且 [2]==name → 命中] 两次采样;
 *   [read_iss/blk_read/blk_wrtn/read_merged/write_com/write_merged != 0 → 差分速率];
 *   [[3]!=0 → p_rrqm; [7]!=0 → p_wrqm（已知缺陷：整数除法恒 0）];
 *   [calcDiskIoStates interval=0 → 取 1]
 */
TEST_F(UT_BlockDevice, test_readDeviceInfo_parses_and_rates)
{
    Stub stub;
    // Arrange
    m_tester->d->name = "sda";
    g_openResult = true;
    g_uptime = timeval { 0, 0 };          // uptime 恒 0 → calcDiskIoStates interval 回退 1
    g_devSize = 512;                      // readDeviceSize 返回最终容量（stub 直接作为 capacity）
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    stub.set((QString(QTextStream::*)(qint64))ADDR(QTextStream, readLine), stub_QTextStream_readLine);
    stub.set(ADDR(SysInfo, uptime), stub_SysInfo_uptime);
    stub.set(ADDR(BlockDevice, readDeviceModel), stub_BlockDevice_readDeviceModel);
    stub.set(ADDR(BlockDevice, readDeviceSize), stub_BlockDevice_readDeviceSize);

    // Act: 第一次采样（种子）
    g_tsLines << diskstatsLine("sda", 100, 10, 200, 50, 20, 100, 5)
              << "7       0 loop0 1 0 2 0 3 0 4 0 0 0 0 0 0 0"   // 含 loop，应被过滤
              << "";
    m_tester->readDeviceInfo();

    // Assert: 第一次采样的累计值与百分比
    EXPECT_EQ(g_readModelCalls, 1);       // readDeviceInfo 委托 readDeviceModel
    EXPECT_EQ(m_tester->capacity(), 512u);
    EXPECT_EQ(m_tester->readIssuer(), 100u);
    EXPECT_EQ(m_tester->readMerged(), 10u);
    EXPECT_EQ(m_tester->blocksRead(), 200u);
    EXPECT_EQ(m_tester->bytesRead(), 200u * 512);
    EXPECT_EQ(m_tester->writeComplete(), 50u);
    EXPECT_EQ(m_tester->writeMerged(), 20u);
    EXPECT_EQ(m_tester->blocksWritten(), 100u);
    EXPECT_EQ(m_tester->bytesWritten(), 100u * 512);
    EXPECT_EQ(m_tester->transferPerSecond(), 150u);   // tps = reads + writes
    EXPECT_DOUBLE_EQ(m_tester->readRequestMergedPercent(), 10.0);  // 10/100*100
    // 已知缺陷（Mode 5 已登记）：p_wrqm 用整数除法 20/50=0 → 恒 0，期望值应为 40.0
    EXPECT_DOUBLE_EQ(m_tester->writeRequestMergedPercent(), 0.0);
    EXPECT_EQ(m_tester->percentUtilization(), 0.0);

    // Act: 第二次采样（差分），interval 恒为 1
    g_tsIdx = 0;
    g_tsLines.clear();
    g_tsLines << diskstatsLine("sda", 100, 10, 300, 50, 20, 150, 5) << "";
    m_tester->readDeviceInfo();

    // Assert: 第二次采样的差分速率（interval=1）
    EXPECT_EQ(m_tester->readIssuer(), 100u);          // reads 不变
    EXPECT_EQ(m_tester->blocksRead(), 300u);
    EXPECT_EQ(m_tester->bytesRead(), 300u * 512);
    EXPECT_DOUBLE_EQ(m_tester->sectorsReadPerSecond(), 100.0);    // (300-200)/1
    EXPECT_DOUBLE_EQ(m_tester->sectorsWrittenPerSecond(), 50.0);  // (150-100)/1
    EXPECT_DOUBLE_EQ(m_tester->readRequestIssuedPerSecond(), 0.0);   // reads 差值 0
    EXPECT_DOUBLE_EQ(m_tester->writeRequestIssuedPerSecond(), 0.0);  // writes 差值 0
    EXPECT_EQ(m_tester->readSpeed(), 100u * 512);     // calcDiskIoStates: rdiff=100, interval=1
    EXPECT_EQ(m_tester->writeSpeed(), 50u * 512);     // wdiff=50, ddiff=0
    EXPECT_EQ(g_readSizeCalls, 2);
}

/**
 * @brief readDeviceInfo 打不开 /proc/diskstats：各计数保持 0
 * 覆盖分支: readDeviceInfo: [QFile::open fail → return（不解析）]
 */
TEST_F(UT_BlockDevice, test_readDeviceInfo_openFail)
{
    Stub stub;
    // Arrange
    m_tester->d->name = "sda";
    g_openResult = false;
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    // Act
    m_tester->readDeviceInfo();
    // Assert
    EXPECT_EQ(m_tester->blocksRead(), 0u);
    EXPECT_EQ(m_tester->blocksWritten(), 0u);
    EXPECT_EQ(m_tester->readIssuer(), 0u);
    EXPECT_EQ(m_tester->capacity(), 0u);
}

/**
 * @brief diskstats 中不存在目标设备：不更新任何计数
 * 覆盖分支: readDeviceInfo: [deviceInfo[2] != d->name → 循环不命中]
 */
TEST_F(UT_BlockDevice, test_readDeviceInfo_noMatch)
{
    Stub stub;
    // Arrange
    m_tester->d->name = "sda";
    g_openResult = true;
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    stub.set((QString(QTextStream::*)(qint64))ADDR(QTextStream, readLine), stub_QTextStream_readLine);
    stub.set(ADDR(SysInfo, uptime), stub_SysInfo_uptime);
    stub.set(ADDR(BlockDevice, readDeviceModel), stub_BlockDevice_readDeviceModel);
    stub.set(ADDR(BlockDevice, readDeviceSize), stub_BlockDevice_readDeviceSize);
    g_tsLines << diskstatsLine("sdb", 100, 10, 200, 50, 20, 100, 5) << "";
    // Act
    m_tester->readDeviceInfo();
    // Assert
    EXPECT_EQ(m_tester->blocksRead(), 0u);
    EXPECT_EQ(m_tester->blocksWritten(), 0u);
    EXPECT_EQ(g_readSizeCalls, 0);        // 未命中即不读取 size
}

/**
 * @brief 列数不足 16 的行被跳过
 * 覆盖分支: readDeviceInfo: [deviceInfo.size() <= 16 → 不处理]
 */
TEST_F(UT_BlockDevice, test_readDeviceInfo_shortLineSkipped)
{
    Stub stub;
    // Arrange
    m_tester->d->name = "sda";
    g_openResult = true;
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    stub.set((QString(QTextStream::*)(qint64))ADDR(QTextStream, readLine), stub_QTextStream_readLine);
    stub.set(ADDR(SysInfo, uptime), stub_SysInfo_uptime);
    stub.set(ADDR(BlockDevice, readDeviceModel), stub_BlockDevice_readDeviceModel);
    stub.set(ADDR(BlockDevice, readDeviceSize), stub_BlockDevice_readDeviceSize);
    g_tsLines << "   8        0 sda 1 2 3" << "";
    // Act
    m_tester->readDeviceInfo();
    // Assert
    EXPECT_EQ(m_tester->blocksRead(), 0u);
    EXPECT_EQ(m_tester->capacity(), 0u);
}

/**
 * @brief 私有 calcDiskIoStates：读/写/丢弃扇区差值 → 字节速率
 * 覆盖分支: calcDiskIoStates:
 *   [curr_read > blk_read → rdiff = 差值]; [curr_write > blk_wrtn → wdiff];
 *   [curr_discard > discard_sector → ddiff]; [rtime > ltime → interval = 2];
 *   wsize = (wdiff + ddiff) * SECTOR_SIZE
 */
TEST_F(UT_BlockDevice, test_calcDiskIoStates)
{
    // Arrange
    m_tester->d->blk_read = 100;
    m_tester->d->blk_wrtn = 200;
    m_tester->d->discard_sector = 50;
    m_tester->timevalList[0] = timeval { 100, 0 };   // prev
    m_tester->timevalList[1] = timeval { 102, 0 };   // curr
    QStringList diskInfo;
    diskInfo << "8" << "0" << "sda" << "0" << "0" << "200" << "0" << "0" << "0" << "350"
             << "0" << "0" << "0" << "0" << "0" << "0" << "60";
    // Act
    m_tester->calcDiskIoStates(diskInfo);
    // Assert: rdiff=100 → 100*512/2；wdiff=150,ddiff=10 → (150+10)*512/2
    EXPECT_EQ(m_tester->readSpeed(), 100u * 512 / 2);
    EXPECT_EQ(m_tester->writeSpeed(), 160u * 512 / 2);
}

/**
 * @brief calcDiskIoStates 计数器回绕（当前值小于上次）按 0 增量处理
 * 覆盖分支: calcDiskIoStates: [curr_read <= blk_read → rdiff = 0] 等三个回绕分支
 */
TEST_F(UT_BlockDevice, test_calcDiskIoStates_counterRollover)
{
    // Arrange
    m_tester->d->blk_read = 1000;
    m_tester->d->blk_wrtn = 2000;
    m_tester->d->discard_sector = 3000;
    m_tester->timevalList[0] = timeval { 100, 0 };
    m_tester->timevalList[1] = timeval { 102, 0 };
    QStringList diskInfo;
    diskInfo << "8" << "0" << "sda" << "0" << "0" << "100" << "0" << "0" << "0" << "200"
             << "0" << "0" << "0" << "0" << "0" << "0" << "40";
    // Act
    m_tester->calcDiskIoStates(diskInfo);
    // Assert: 全部差值回绕为 0 → 速率为 0
    EXPECT_EQ(m_tester->readSpeed(), 0u);
    EXPECT_EQ(m_tester->writeSpeed(), 0u);
}

/**
 * @brief setDeviceName 记录设备名（readDeviceInfo 打不开文件时早退）
 * 覆盖分支: setDeviceName: [d->name 赋值 → readDeviceInfo（open fail 早退）]
 */
TEST_F(UT_BlockDevice, test_setDeviceName)
{
    Stub stub;
    // Arrange
    g_openResult = false;
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    // Act
    m_tester->setDeviceName("nvme0n1");
    // Assert
    EXPECT_EQ(m_tester->deviceName(), QByteArray("nvme0n1"));
    EXPECT_EQ(g_openCalls, 1);            // setDeviceName 委托 readDeviceInfo
}

/**
 * @brief 析构不崩溃（隐式共享数据释放）
 * 覆盖分支: ~BlockDevice: [delete d]
 */
TEST_F(UT_BlockDevice, test_destructor)
{
    // Arrange
    BlockDevice *dev = new BlockDevice();
    {
        Stub stub;
        stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
        dev->setDeviceName("sda");
    }
    // Act & Assert: 正常释放，无崩溃
    delete dev;
    SUCCEED();
}
