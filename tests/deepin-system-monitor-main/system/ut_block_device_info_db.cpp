// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/block_device_info_db.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

using namespace core::system;

/***************************************STUB begin*********************************************/

// QDir::exists —— 控制 /sys/block 是否"存在"
static bool g_dirExists = true;
static bool stub_QDir_exists()
{
    return g_dirExists;
}

// QDir::entryInfoList —— 返回受控目录项（QTemporaryDir 中真实创建的目录/符号链接）
static QFileInfoList g_entries;
static QFileInfoList stub_QDir_entryInfoList(QDir::Filters, QDir::SortFlags)
{
    return g_entries;
}

// BlockDevice::readDeviceSize —— 隔离 readDiskInfo 对 /sys 的读取
static quint64 g_devSize = 0;
static int g_readSizeCalls = 0;
static quint64 stub_BlockDevice_readDeviceSize(const QString &)
{
    ++g_readSizeCalls;
    return g_devSize;
}

// QFile::open —— 截断 setDeviceName → readDeviceInfo 对 /proc 的真实读取
typedef bool (*QFileOpenPtr)(QFile *, QIODevice::OpenMode);
static bool stub_QFile_open(QFile *, QIODevice::OpenMode)
{
    return false;
}

/***************************************STUB end***********************************************/

class UT_BlockDeviceInfoDB : public ::testing::Test
{
public:
    UT_BlockDeviceInfoDB() : m_tester(nullptr) {}

    virtual void SetUp()
    {
        g_dirExists = true;
        g_entries.clear();
        g_devSize = 0;
        g_readSizeCalls = 0;
        m_tester = new BlockDeviceInfoDB();
    }

    virtual void TearDown()
    {
        delete m_tester;
        m_tester = nullptr;
    }

    // 在临时目录创建一个符号链接目录项并加入受控 entryInfoList
    void addLinkEntry(const QString &name, const QString &target)
    {
        const QString path = m_tmpDir.path() + "/" + name;
        QFile::link(target, path);
        g_entries << QFileInfo(path);
    }

    // 在临时目录创建普通目录项并加入受控 entryInfoList（如 ram0/loop0）
    void addDirEntry(const QString &name)
    {
        const QString path = m_tmpDir.path() + "/" + name;
        QDir(m_tmpDir.path()).mkpath(name);
        g_entries << QFileInfo(path);
    }

    // 预置一台已存在的设备（setDeviceName 触发的 readDeviceInfo 由 stub_QFile_open 截断）
    void seedDevice(const QByteArray &name)
    {
        BlockDevice bd;
        bd.setDeviceName(name);
        m_tester->m_deviceList.append(bd);
    }

protected:
    BlockDeviceInfoDB *m_tester;
    QTemporaryDir m_tmpDir;
};

/**
 * @brief 默认构造：设备列表为空
 * 覆盖分支: BlockDeviceInfoDB 构造 → m_deviceList 空初始化
 */
TEST_F(UT_BlockDeviceInfoDB, initTest)
{
    // Arrange: SetUp 已创建对象
    // Act & Assert
    EXPECT_TRUE(m_tester->deviceList().isEmpty());
    EXPECT_EQ(m_tester->deviceList().size(), 0);
}

/**
 * @brief readDiskInfo 同时收录物理盘与虚拟盘
 * 覆盖分支: readDiskInfo:
 *   [dir.exists → true]; [fileName 过滤 . / .. / ram / loop];
 *   物理轮次: [t_link 不含 "virtual" 且 readDeviceSize>0 → 加入];
 *   虚拟轮次: [t_link 含 "virtual" 且 readDeviceSize>0 → 加入]
 */
TEST_F(UT_BlockDeviceInfoDB, test_readDiskInfo_physical_and_virtual)
{
    Stub stub;
    // Arrange: sda 为物理盘（链接目标不含 virtual），vdb 为虚拟盘
    addLinkEntry("sda", "../devices/pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block/sda");
    addLinkEntry("vdb", "../devices/virtual/block/vdb");
    g_devSize = 1;   // 1 扇区 > 0，准入
    stub.set((bool (QDir::*)() const)ADDR(QDir, exists), stub_QDir_exists);
    stub.set((QFileInfoList(QDir::*)(QDir::Filters, QDir::SortFlags) const)ADDR(QDir, entryInfoList), stub_QDir_entryInfoList);
    stub.set(ADDR(BlockDevice, readDeviceSize), stub_BlockDevice_readDeviceSize);
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);   // 截断 setDeviceName 链路
    // Act
    m_tester->readDiskInfo();
    // Assert: 两台设备均已收录，名称正确
    QList<BlockDevice> list = m_tester->deviceList();
    ASSERT_EQ(list.size(), 2);
    EXPECT_EQ(list.at(0).deviceName(), QByteArray("sda"));   // 物理轮次先加入
    EXPECT_EQ(list.at(1).deviceName(), QByteArray("vdb"));   // 虚拟轮次后加入
    EXPECT_EQ(g_readSizeCalls, 2);
}

/**
 * @brief readDeviceSize 返回 0 的设备被拒绝收录
 * 覆盖分支: readDiskInfo: [readDeviceSize(name) <= 0 → 不加入 m_deviceList]
 */
TEST_F(UT_BlockDeviceInfoDB, test_readDiskInfo_zeroSizeRejected)
{
    Stub stub;
    // Arrange
    addLinkEntry("sda", "../devices/pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block/sda");
    g_devSize = 0;
    stub.set((bool (QDir::*)() const)ADDR(QDir, exists), stub_QDir_exists);
    stub.set((QFileInfoList(QDir::*)(QDir::Filters, QDir::SortFlags) const)ADDR(QDir, entryInfoList), stub_QDir_entryInfoList);
    stub.set(ADDR(BlockDevice, readDeviceSize), stub_BlockDevice_readDeviceSize);
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    // Act
    m_tester->readDiskInfo();
    // Assert
    EXPECT_TRUE(m_tester->deviceList().isEmpty());
    EXPECT_EQ(g_readSizeCalls, 1);   // 仅物理轮询问；虚拟轮因 t_link 不含 virtual 跳过
}

/**
 * @brief ram/loop 设备按名称过滤，不进入任何一轮收录
 * 覆盖分支: readDiskInfo: [fileName().contains("ram") → 跳过]; [contains("loop") → 跳过]
 */
TEST_F(UT_BlockDeviceInfoDB, test_readDiskInfo_filters_ram_and_loop)
{
    Stub stub;
    // Arrange
    addDirEntry("ram0");
    addDirEntry("loop0");
    addLinkEntry("sda", "../devices/pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block/sda");
    g_devSize = 1;
    stub.set((bool (QDir::*)() const)ADDR(QDir, exists), stub_QDir_exists);
    stub.set((QFileInfoList(QDir::*)(QDir::Filters, QDir::SortFlags) const)ADDR(QDir, entryInfoList), stub_QDir_entryInfoList);
    stub.set(ADDR(BlockDevice, readDeviceSize), stub_BlockDevice_readDeviceSize);
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    // Act
    m_tester->readDiskInfo();
    // Assert: 仅 sda 被收录
    QList<BlockDevice> list = m_tester->deviceList();
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list.at(0).deviceName(), QByteArray("sda"));
}

/**
 * @brief 已存在设备走"更新"路径，不会重复收录
 * 覆盖分支: readDiskInfo: [m_deviceList 中已存在同名设备 → index != -1 → setDeviceName 更新]
 */
TEST_F(UT_BlockDeviceInfoDB, test_readDiskInfo_updatesExisting)
{
    Stub stub;
    // Arrange: 预置 sda，且目录项中 sda 仍存在
    seedDevice("sda");
    addLinkEntry("sda", "../devices/pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block/sda");
    g_devSize = 1;
    stub.set((bool (QDir::*)() const)ADDR(QDir, exists), stub_QDir_exists);
    stub.set((QFileInfoList(QDir::*)(QDir::Filters, QDir::SortFlags) const)ADDR(QDir, entryInfoList), stub_QDir_entryInfoList);
    stub.set(ADDR(BlockDevice, readDeviceSize), stub_BlockDevice_readDeviceSize);
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    // Act
    m_tester->readDiskInfo();
    // Assert: 不重复追加
    QList<BlockDevice> list = m_tester->deviceList();
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list.at(0).deviceName(), QByteArray("sda"));
}

/**
 * @brief 目录中消失的设备被移除（单个缺失场景正常移除）
 * 覆盖分支: readDiskInfo: [遍历 m_deviceList，未在目录项中找到 → removeAt(i)]
 */
TEST_F(UT_BlockDeviceInfoDB, test_readDiskInfo_removesMissingDevice)
{
    Stub stub;
    // Arrange: 仅 sda 在"目录"中，gone 设备应被移除
    seedDevice("sda");
    seedDevice("gone");
    addLinkEntry("sda", "../devices/pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block/sda");
    g_devSize = 1;
    stub.set((bool (QDir::*)() const)ADDR(QDir, exists), stub_QDir_exists);
    stub.set((QFileInfoList(QDir::*)(QDir::Filters, QDir::SortFlags) const)ADDR(QDir, entryInfoList), stub_QDir_entryInfoList);
    stub.set(ADDR(BlockDevice, readDeviceSize), stub_BlockDevice_readDeviceSize);
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    // Act
    m_tester->readDiskInfo();
    // Assert
    QList<BlockDevice> list = m_tester->deviceList();
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list.at(0).deviceName(), QByteArray("sda"));
}

/**
 * @brief 已知缺陷证据：连续两台设备缺失时，removeAt(i) 边删边走会漏检第二台
 * 覆盖分支: readDiskInfo: [removeAt(i) 后 i++ 跳过下一个元素 → gone2 残留]
 * 注: 该用例锁定当前实际行为；缺陷已在 Mode 5 缺陷清单登记，修复后应改为断言 size==1。
 */
TEST_F(UT_BlockDeviceInfoDB, test_readDiskInfo_twoMissingDevices_skipDefect)
{
    Stub stub;
    // Arrange: sda 存在，gone1/gone2 均不在目录中
    seedDevice("sda");
    seedDevice("gone1");
    seedDevice("gone2");
    addLinkEntry("sda", "../devices/pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block/sda");
    g_devSize = 1;
    stub.set((bool (QDir::*)() const)ADDR(QDir, exists), stub_QDir_exists);
    stub.set((QFileInfoList(QDir::*)(QDir::Filters, QDir::SortFlags) const)ADDR(QDir, entryInfoList), stub_QDir_entryInfoList);
    stub.set(ADDR(BlockDevice, readDeviceSize), stub_BlockDevice_readDeviceSize);
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    // Act
    m_tester->readDiskInfo();
    // Assert（当前实际行为）: gone1 被移除，gone2 因跳过而残留
    QList<BlockDevice> list = m_tester->deviceList();
    ASSERT_EQ(list.size(), 2);
    EXPECT_EQ(list.at(0).deviceName(), QByteArray("sda"));
    EXPECT_EQ(list.at(1).deviceName(), QByteArray("gone2"));
}

/**
 * @brief /sys/block 目录不存在：直接返回，列表保持原状
 * 覆盖分支: readDiskInfo: [dir.exists() == false → return]
 */
TEST_F(UT_BlockDeviceInfoDB, test_readDiskInfo_dirMissing)
{
    Stub stub;
    // Arrange
    g_dirExists = false;
    stub.set((bool (QDir::*)() const)ADDR(QDir, exists), stub_QDir_exists);
    // Act
    m_tester->readDiskInfo();
    // Assert
    EXPECT_TRUE(m_tester->deviceList().isEmpty());
    EXPECT_EQ(g_readSizeCalls, 0);
}

/**
 * @brief update() 委托 readDiskInfo 完成设备收录
 * 覆盖分支: update: [readDiskInfo() 委托 → 收录结果落库]
 */
TEST_F(UT_BlockDeviceInfoDB, test_update_delegatesToReadDiskInfo)
{
    Stub stub;
    // Arrange
    addLinkEntry("sda", "../devices/pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block/sda");
    g_devSize = 1;
    stub.set((bool (QDir::*)() const)ADDR(QDir, exists), stub_QDir_exists);
    stub.set((QFileInfoList(QDir::*)(QDir::Filters, QDir::SortFlags) const)ADDR(QDir, entryInfoList), stub_QDir_entryInfoList);
    stub.set(ADDR(BlockDevice, readDeviceSize), stub_BlockDevice_readDeviceSize);
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    // Act
    m_tester->update();
    // Assert: update 的效果与 readDiskInfo 一致
    QList<BlockDevice> list = m_tester->deviceList();
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list.at(0).deviceName(), QByteArray("sda"));
}

/**
 * @brief deviceList() 返回副本：外部修改不影响内部状态
 * 覆盖分支: deviceList: [QReadLocker + 值返回副本]
 */
TEST_F(UT_BlockDeviceInfoDB, test_deviceList_returnsCopy)
{
    Stub stub;
    // Arrange
    stub.set((QFileOpenPtr)((bool(QFile::*)(QIODevice::OpenMode))ADDR(QFile, open)), stub_QFile_open);
    seedDevice("sda");
    // Act
    QList<BlockDevice> copy = m_tester->deviceList();
    copy.clear();
    // Assert: 原始列表不受副本修改影响
    EXPECT_EQ(m_tester->deviceList().size(), 1);
    EXPECT_EQ(m_tester->deviceList().at(0).deviceName(), QByteArray("sda"));
}
