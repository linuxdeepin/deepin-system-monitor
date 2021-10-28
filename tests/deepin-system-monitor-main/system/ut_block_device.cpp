/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
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
#include "system/block_device.h"
#include <QString>
#include <QProcess>

#include "ut_block_device.h"

Ut_BlockDevice::Ut_BlockDevice()
{
}

void stub_ReadDeviceInfo()
{
    core::system::BlockDevice blockDevice;
}

TEST(UT_BlockDevice_setDeviceName, UT_BlockDevice_setDeviceName)
{
    core::system::BlockDevice blockDevice;
    QString strFileName = "abc";
    blockDevice.setDeviceName(strFileName.toLocal8Bit());
    QString retName = QString::fromLatin1(blockDevice.deviceName());
//    qInfo()<<"retName:"<<retName;
    EXPECT_EQ("abc", retName);
}

//TEST(UT_BlockDevice_readDeviceInfo, UT_BlockDevice_readDeviceInfo)
//{
//    core::system::BlockDevice blockDevice;
//    Stub stub;
//    stub.set(ADDR(core::system::BlockDevice, readDeviceInfo), stub_ReadDeviceInfo);

//    EXPECT_EQ();
//}
