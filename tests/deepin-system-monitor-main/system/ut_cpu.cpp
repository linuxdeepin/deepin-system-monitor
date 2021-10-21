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
#include "system/cpu.h"
#include <QString>
#include <QProcess>

#include "ut_cpu.h"

Ut_CPU::Ut_CPU()
{
}


TEST(UT_CPU_logicalIndex, UT_CPU_logicalIndex_001)
{
    QList<core::system::CPUInfo> infos;
    QProcess process;
    process.start("cat /proc/cpuinfo");
    process.waitForFinished(3000);
    QString cpuinfo = process.readAllStandardOutput();
    QStringList processors = cpuinfo.split("\n\n", QString::SkipEmptyParts);

    for (int i = 0; i < 1; ++i) {
        core::system::CPUInfo info;
        QStringList list = processors[i].split("\n", QString::SkipEmptyParts);
        for (QString text : list) {
            if (text.startsWith("processor")) {
                info.setIndex(text.split(":").value(1).toInt());
            } else if (text.startsWith("vendor_id")) {
                info.setVendorId(text.split(":").value(1));
            } else if (text.startsWith("cpu MHz")) {
                info.setCpuFreq(text.split(":").value(1));
            } else if (text.startsWith("core id")) {
                info.setCoreId(text.split(":").value(1).toUInt());
            } else if (text.startsWith("model name")) {
                info.setModelName(text.split(":").value(1));
            } else if (text.startsWith("cache size")) {
                info.setCacheSize(text.split(":").value(1));
            }
        }

        infos.append(info);

//        qInfo()<<"logicalIndex:"<<info.logicalIndex();
        EXPECT_EQ(0, info.logicalIndex());
    }
}

TEST(UT_CPU_coreID, UT_CPU_coreID)
{
    core::system::CPUInfo info;
    info.setCoreId(10);
    qInfo()<<"coreID:"<<info.coreID();
    EXPECT_EQ("10", info.coreID());
}

TEST(UT_CPU_modelName, UT_CPU_modelName)
{
    core::system::CPUInfo info;
    info.setModelName("abc");
    EXPECT_EQ("abc", info.modelName());
}

TEST(UT_CPU_vendorId, UT_CPU_vendorId)
{
    core::system::CPUInfo info;
    info.setVendorId("abc");
    EXPECT_EQ("abc", info.vendorId());
}


TEST(UT_CPU_cpuFreq, UT_CPU_cpuFreq_001)
{
    core::system::CPUInfo info;
    info.setCpuFreq("abc");
    QString retString =  info.cpuFreq();
//    qInfo()<<"cpuFreq:"<<retString.length();
    EXPECT_NE(3, retString.length());
}

TEST(UT_CPU_cacheSize, UT_CPU_cacheSize)
{
    core::system::CPUInfo info;
    info.setCacheSize("10");
    qInfo()<<"cacheSize:"<<info.cacheSize();
    EXPECT_EQ("10", info.cacheSize());
}

TEST(UT_CPU_setIndex, UT_CPU_setIndex)
{
    core::system::CPUInfo info;
    info.setIndex(10);
    EXPECT_EQ(10, info.logicalIndex());
}

TEST(UT_CPU_setCoreId, UT_CPU_setCoreId)
{
    core::system::CPUInfo info;
    info.setCoreId(10);
    EXPECT_EQ("10", info.coreID());
}

TEST(UT_CPU_setModelName, UT_CPU_setModelName)
{
    core::system::CPUInfo info;
    info.setModelName("abc");
    EXPECT_EQ("abc", info.modelName());
}

TEST(UT_CPU_setVendorId, UT_CPU_setVendorId)
{
    core::system::CPUInfo info;
    info.setVendorId("abc");
    EXPECT_EQ("abc", info.vendorId());
}

TEST(UT_CPU_setCpuFreq, UT_CPU_setCpuFreq)
{
    core::system::CPUInfo info;
    info.setCpuFreq("abcde");
    QString retString =  info.cpuFreq();
    EXPECT_NE(5, retString.length());
}

TEST(UT_CPU_setCacheSize, UT_CPU_setCacheSize)
{
    core::system::CPUInfo info;
    info.setCacheSize("10");
    qInfo()<<"cacheSize:"<<info.cacheSize();
    EXPECT_EQ("10", info.cacheSize());
}

