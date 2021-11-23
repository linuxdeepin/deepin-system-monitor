/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      baohaifeng <baohaifeng@uniontech.com>
* Maintainer:  baohaifeng <baohaifeng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

TEST_F(UT_BlockDevice, test_readDeviceModel)
{
//    core::system::BlockDevice blockDevice;
//    blockDevice.readDeviceModel();
//    if (blockDevice.model().isEmpty())
//        qInfo("isEmpty");
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



