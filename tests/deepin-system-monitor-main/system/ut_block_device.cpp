// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/block_device.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QString>
#include <QProcess>
#include <QFile>
#include <QIODevice>
#include <QTextStream>

//c++
#include <limits>

using namespace core::system;

/***************************************STUB begin*********************************************/

void stub_ReadDeviceInfo()
{
    core::system::BlockDevice blockDevice;
}

bool stub_modPath_readDeviceSize(void*, QFile::OpenMode)
{
    return false;
}

bool stub_contains(const QStringRef &s, Qt::CaseSensitivity cs)
{
    return true;
}

/***************************************STUB end**********************************************/

class UT_BlockDevice: public ::testing::Test
{
public:
    UT_BlockDevice() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new BlockDevice();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    BlockDevice *m_tester;
};

TEST_F(UT_BlockDevice, initTest)
{
}

TEST_F(UT_BlockDevice, test_copy)
{
    BlockDevice* rhs = m_tester;
    m_tester->operator= (*rhs);
}

TEST_F(UT_BlockDevice, test_setDeviceName)
{
    QString strFileName = "abc";
    m_tester->setDeviceName(strFileName.toLocal8Bit());
    QString retName = QString::fromLatin1(m_tester->deviceName());
//    qInfo()<<"retName:"<<retName;
    EXPECT_EQ("abc", retName);
}

TEST_F(UT_BlockDevice, test_readDeviceInfo)
{
    m_tester->readDeviceInfo();
    EXPECT_TRUE((m_tester->capacity() > 0) || (m_tester->d->_time_Sec>0));
}

//TEST_F(UT_BlockDevice, test_readDeviceInfo_02)
//{
//    Stub stub;
//    stub.set((bool(QString::*)(QStringRef &))ADDR(QString, contains), stub_contains);
//    m_tester->readDeviceInfo();
//    EXPECT_TRUE((m_tester->capacity() > 0) || (m_tester->d->_time_Sec>0));
//}

TEST_F(UT_BlockDevice, test_readDeviceModel)
{
    core::system::BlockDevice blockDevice;
    blockDevice.readDeviceModel();
    if (blockDevice.model().isEmpty())
        qInfo("isEmpty");
//    EXPECT_FALSE(blockDevice.model().isEmpty());
}

TEST_F(UT_BlockDevice, test_readDeviceSize_01)
{
    EXPECT_EQ(m_tester->readDeviceSize(m_tester->d->name), 0);
}

TEST_F(UT_BlockDevice, test_readDeviceSize_02)
{
    typedef bool (*fptr)(QFile*, QIODevice::OpenMode);
    fptr A_foo = (fptr)((bool(QFile::*)(QIODevice::OpenMode))&QFile::open);

    Stub stub;
    stub.set(A_foo, stub_modPath_readDeviceSize);
    m_tester->readDeviceSize("101");
}

TEST_F(UT_BlockDevice, test_calcDiskIoStates)
{
    QFile file(PROC_PATH_DISK);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QList<QStringList> strList;
    QTextStream in(&file);

    QString line;
    do{
        line = in.readLine();
        if (!line.contains("loop")){
            strList << line.split(" ", QString::SkipEmptyParts);
        }
    }while (!line.isNull());
    file.close();

    if (strList.size()){
        QStringList deviceInfo = strList[0];
        if (deviceInfo.size() > 16 && deviceInfo[2] == m_tester->d->name) {
            m_tester->calcDiskIoStates(deviceInfo);
            EXPECT_TRUE(m_tester->readSpeed() != 0);
        }
     }
}

TEST_F(UT_BlockDevice, test_calcDiskIoStates_subSecondInterval)
{
    QStringList diskInfo { "8", "0", "sda", "10", "0", "100", "0", "20", "0", "200", "0", "0", "0", "0", "0", "0", "5" };

    m_tester->d->blk_read = 50;
    m_tester->d->blk_wrtn = 100;
    m_tester->d->discard_sector = 1;
    m_tester->timevalList[0] = timeval { 10, 900000 };
    m_tester->timevalList[1] = timeval { 11, 100000 };

    m_tester->calcDiskIoStates(diskInfo);

    // 采样间隔 10.9s -> 11.1s，即 0.2 秒；亚秒间隔必须保留小数精度。
    // 读：(100-50) 扇区 * 512 字节 / 0.2s = 128000 B/s
    // 写：((200-100)+(5-1)) 扇区 * 512 字节 / 0.2s = 266240 B/s
    EXPECT_EQ(static_cast<quint64>(128000), m_tester->readSpeed());
    EXPECT_EQ(static_cast<quint64>(266240), m_tester->writeSpeed());
}

TEST_F(UT_BlockDevice, test_calcDiskIoStates_truncatedInput)
{
    m_tester->d->blk_read = 50;
    m_tester->d->blk_wrtn = 100;
    m_tester->d->discard_sector = 1;
    m_tester->timevalList[0] = timeval { 10, 900000 };
    m_tester->timevalList[1] = timeval { 11, 100000 };

    // 字段不足 17 个时不能越界读取 discard 扇区数
    m_tester->calcDiskIoStates(QStringList {});
    m_tester->calcDiskIoStates(QStringList { "8", "0", "sda" });

    QStringList sixteenFields;
    for (int i = 0; i < 16; ++i)
        sixteenFields << "0";
    m_tester->calcDiskIoStates(sixteenFields);

    EXPECT_EQ(0ULL, m_tester->readSpeed());
    EXPECT_EQ(0ULL, m_tester->writeSpeed());
}

TEST_F(UT_BlockDevice, test_calcDiskIoStates_tinyIntervalSaturates)
{
    QStringList diskInfo { "8", "0", "sda", "10", "0", "1099511627776", "0", "20", "0",
                           "1099511627776", "0", "0", "0", "0", "0", "0", "1099511627776" };

    m_tester->d->blk_read = 0;
    m_tester->d->blk_wrtn = 0;
    m_tester->d->discard_sector = 0;
    // 1us 间隔配合巨大增量，速率超出 quint64 范围时饱和到上限
    m_tester->timevalList[0] = timeval { 10, 999999 };
    m_tester->timevalList[1] = timeval { 11, 0 };

    m_tester->calcDiskIoStates(diskInfo);

    EXPECT_EQ(std::numeric_limits<quint64>::max(), m_tester->readSpeed());
    EXPECT_EQ(std::numeric_limits<quint64>::max(), m_tester->writeSpeed());
}

TEST_F(UT_BlockDevice, test_calcDiskIoStates_timeRollback)
{
    QStringList diskInfo { "8", "0", "sda", "10", "0", "100", "0", "20", "0", "200", "0", "0", "0", "0", "0", "0", "5" };

    m_tester->d->blk_read = 50;
    m_tester->d->blk_wrtn = 100;
    m_tester->d->discard_sector = 1;
    // 时间戳倒退时退化为 1 秒间隔，不应除零
    m_tester->timevalList[0] = timeval { 20, 0 };
    m_tester->timevalList[1] = timeval { 10, 0 };

    m_tester->calcDiskIoStates(diskInfo);

    // 间隔固定回退为 1 秒：读增量 100-50=50 扇区，写增量 (200-100)+(5-1)=104 扇区
    EXPECT_EQ(static_cast<quint64>(50 * 512), m_tester->readSpeed());
    EXPECT_EQ(static_cast<quint64>(104 * 512), m_tester->writeSpeed());
}

TEST_F(UT_BlockDevice, test_calcDiskIoStates_truncatedInputClearsStaleRates)
{
    QStringList valid { "8", "0", "sda", "10", "0", "100", "0", "20", "0", "200", "0", "0", "0", "0", "0", "0", "5" };

    m_tester->d->blk_read = 50;
    m_tester->d->blk_wrtn = 100;
    m_tester->d->discard_sector = 1;
    m_tester->timevalList[0] = timeval { 10, 0 };
    m_tester->timevalList[1] = timeval { 11, 0 };

    // 先完成一次有效采样，速率非零
    m_tester->calcDiskIoStates(valid);
    ASSERT_GT(m_tester->readSpeed(), 0ULL);
    ASSERT_GT(m_tester->writeSpeed(), 0ULL);

    // 后续采样字段不足时，旧速率必须清零，不能继续展示
    m_tester->calcDiskIoStates(QStringList { "8", "0", "sda" });

    EXPECT_EQ(0ULL, m_tester->readSpeed());
    EXPECT_EQ(0ULL, m_tester->writeSpeed());
}

TEST_F(UT_BlockDevice, test_calcDiskIoStates_largeCounterNoWrap)
{
    // 扇区增量足够大，乘以 512 后超出 quint64 范围；换算在 double 域完成，不能回绕成小值。
    // 取 2^55 而非 2^55+1，是因为 quint64 转为 double 后超过 2^53 的整数会丢精度。
    const quint64 hugeSectors = 1ULL << 55;
    QStringList diskInfo { "8", "0", "sda", "10", "0", QString::number(hugeSectors), "0", "20", "0",
                           QString::number(hugeSectors), "0", "0", "0", "0", "0", "0", "0" };

    m_tester->d->blk_read = 0;
    m_tester->d->blk_wrtn = 0;
    m_tester->d->discard_sector = 0;
    m_tester->timevalList[0] = timeval { 10, 0 };
    m_tester->timevalList[1] = timeval { 11, 0 };

    m_tester->calcDiskIoStates(diskInfo);

    // 回绕会得到 512 这类小值，正确结果应饱和到上限
    EXPECT_EQ(std::numeric_limits<quint64>::max(), m_tester->readSpeed());
    EXPECT_EQ(std::numeric_limits<quint64>::max(), m_tester->writeSpeed());
}

TEST_F(UT_BlockDevice, test_readDeviceInfo_counterRollback)
{
    QFile file(PROC_PATH_DISK);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QStringList deviceInfo;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.contains("loop")) {
            continue;
        }

        const QStringList fields = line.split(" ", QString::SkipEmptyParts);
        if (fields.size() > 16) {
            deviceInfo = fields;
            break;
        }
    }
    file.close();

    if (deviceInfo.isEmpty()) {
        return;
    }

    m_tester->d->name = deviceInfo[2].toLocal8Bit();
    m_tester->d->read_iss = deviceInfo[3].toULongLong() + 1;
    m_tester->d->read_merged = deviceInfo[4].toULongLong() + 1;
    m_tester->d->blk_read = deviceInfo[5].toULongLong() + 1;
    m_tester->d->write_com = deviceInfo[7].toULongLong() + 1;
    m_tester->d->write_merged = deviceInfo[8].toULongLong() + 1;
    m_tester->d->blk_wrtn = deviceInfo[9].toULongLong() + 1;

    m_tester->readDeviceInfo();

    EXPECT_EQ(0, m_tester->readRequestIssuedPerSecond());
    EXPECT_EQ(0, m_tester->readRequestMergedPerSecond());
    EXPECT_EQ(0, m_tester->sectorsReadPerSecond());
    EXPECT_EQ(0, m_tester->writeRequestIssuedPerSecond());
    EXPECT_EQ(0, m_tester->writeRequestMergedPerSecond());
    EXPECT_EQ(0, m_tester->sectorsWrittenPerSecond());
}

TEST_F(UT_BlockDevice, test_deviceName)
{
    m_tester->deviceName();
}

TEST_F(UT_BlockDevice, test_model)
{
    m_tester->model();
}

TEST_F(UT_BlockDevice, test_capacity)
{
    m_tester->capacity();
}

TEST_F(UT_BlockDevice, test_blocksRead)
{
    m_tester->blocksRead();
}

TEST_F(UT_BlockDevice, test_bytesRead)
{
    m_tester->bytesRead();
}

TEST_F(UT_BlockDevice, test_sectorsReadPerSecond)
{
    m_tester->sectorsReadPerSecond();
}

TEST_F(UT_BlockDevice, test_readRequestIssuedPerSecond)
{
    m_tester->readRequestIssuedPerSecond();
}

TEST_F(UT_BlockDevice, test_readRequestMergedPerSecond)
{
    m_tester->readDeviceInfo();
    m_tester->readRequestMergedPerSecond();
}

TEST_F(UT_BlockDevice, test_readRequestMergedPercent)
{
    m_tester->readDeviceInfo();
    m_tester->readRequestMergedPercent();
}

TEST_F(UT_BlockDevice, test_percentUtilization)
{
    m_tester->readDeviceInfo();
    m_tester->percentUtilization();
}

TEST_F(UT_BlockDevice, test_transferPerSecond)
{
    m_tester->readDeviceInfo();
    m_tester->transferPerSecond();
}

TEST_F(UT_BlockDevice, test_blocksWritten)
{
    m_tester->readDeviceInfo();
    m_tester->blocksWritten();
}

TEST_F(UT_BlockDevice, test_bytesWritten)
{
    m_tester->readDeviceInfo();
    m_tester->bytesWritten();
}

TEST_F(UT_BlockDevice, test_sectorsWrittenPerSecond)
{
    m_tester->readDeviceInfo();
    m_tester->sectorsWrittenPerSecond();
}

TEST_F(UT_BlockDevice, test_writeRequestIssuedPerSecond)
{
    m_tester->readDeviceInfo();
    m_tester->writeRequestIssuedPerSecond();
}

TEST_F(UT_BlockDevice, test_writeRequestMergedPerSecond)
{
    m_tester->readDeviceInfo();
    m_tester->writeRequestMergedPerSecond();
}
TEST_F(UT_BlockDevice, test_writeRequestMergedPercent)
{
    m_tester->readDeviceInfo();
    m_tester->writeRequestMergedPercent();
}

TEST_F(UT_BlockDevice, test_readIssuer)
{
    m_tester->readDeviceInfo();
    m_tester->readIssuer();
}

TEST_F(UT_BlockDevice, test_writeComplete)
{
    m_tester->readDeviceInfo();
    m_tester->writeComplete();
}
TEST_F(UT_BlockDevice, test_readMerged)
{
    m_tester->readDeviceInfo();
    m_tester->readMerged();
}

TEST_F(UT_BlockDevice, test_writeMerged)
{
    m_tester->readDeviceInfo();
    m_tester->writeMerged();
}

TEST_F(UT_BlockDevice, test_readSpeed)
{
    m_tester->readDeviceInfo();
    m_tester->readSpeed();
}
TEST_F(UT_BlockDevice, test_writeSpeed)
{
    m_tester->readDeviceInfo();
    m_tester->writeSpeed();
}


