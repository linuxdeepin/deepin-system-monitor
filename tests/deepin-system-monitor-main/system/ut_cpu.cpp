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
#include "system/cpu.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QString>
#include <QProcess>
using namespace core::system;

class UT_CPU: public ::testing::Test
{
public:
    UT_CPU() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new CPUInfo();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CPUInfo *m_tester;
};

TEST_F(UT_CPU, initTest)
{
}

TEST_F(UT_CPU, test_logicalIndex)
{
    QList<CPUInfo> infos;

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

TEST_F(UT_CPU, test_coreID)
{
    m_tester->setCoreId(10);
    qInfo()<<"coreID:"<<m_tester->coreID();
    EXPECT_EQ("10", m_tester->coreID());
}

TEST_F(UT_CPU, test_modelName)
{
    m_tester->setModelName("abc");
    EXPECT_EQ("abc", m_tester->modelName());
}

TEST_F(UT_CPU, test_vendorId)
{
    m_tester->setVendorId("abc");
    EXPECT_EQ("abc", m_tester->vendorId());
}


TEST_F(UT_CPU, test_cpuFreq)
{
    m_tester->setCpuFreq("abc");
    QString retString =  m_tester->cpuFreq();
//    qInfo()<<"cpuFreq:"<<retString.length();
    EXPECT_NE(3, retString.length());
}

TEST_F(UT_CPU, test_cacheSize)
{
    m_tester->setCacheSize("10");
    qInfo()<<"cacheSize:"<<m_tester->cacheSize();
    EXPECT_EQ("10", m_tester->cacheSize());
}

TEST_F(UT_CPU, test_setIndex)
{
    m_tester->setIndex(10);
    EXPECT_EQ(10, m_tester->logicalIndex());
}

TEST_F(UT_CPU, test_setCoreId)
{
    m_tester->setCoreId(10);
    EXPECT_EQ("10", m_tester->coreID());
}

TEST_F(UT_CPU, test_setModelName)
{
    m_tester->setModelName("abc");
    EXPECT_EQ("abc", m_tester->modelName());
}

TEST_F(UT_CPU, test_setVendorId)
{
    m_tester->setVendorId("abc");
    EXPECT_EQ("abc", m_tester->vendorId());
}

TEST_F(UT_CPU, test_setCpuFreq)
{
    m_tester->setCpuFreq("abcde");
    QString retString =  m_tester->cpuFreq();
    EXPECT_NE(5, retString.length());
}

TEST_F(UT_CPU, test_setCacheSize)
{
    m_tester->setCacheSize("10");
    qInfo()<<"cacheSize:"<<m_tester->cacheSize();
    EXPECT_EQ("10", m_tester->cacheSize());
}

