// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "model/gpu_info_model.h"
#include "system/gpu_info.h"
#include "system/device_db.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::system;

class UT_GPUInfoModel: public ::testing::Test
{
public:
    UT_GPUInfoModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = GPUInfoModel::instance();
    }

    virtual void TearDown()
    {
        // Singleton - don't delete
    }

protected:
    GPUInfoModel *m_tester;
};

TEST_F(UT_GPUInfoModel, initTest)
{
    EXPECT_TRUE(m_tester != nullptr);
}

TEST_F(UT_GPUInfoModel, test_instance)
{
    EXPECT_TRUE(GPUInfoModel::instance() != nullptr);
    // Should always return same instance (singleton)
    EXPECT_EQ(GPUInfoModel::instance(), m_tester);
}

TEST_F(UT_GPUInfoModel, test_gpuCount)
{
    int count = m_tester->gpuCount();
    EXPECT_GE(count, 0);
}

TEST_F(UT_GPUInfoModel, test_gpuList)
{
    auto list = m_tester->gpuList();
    EXPECT_EQ(list.size(), m_tester->gpuCount());
}

TEST_F(UT_GPUInfoModel, test_gpuAllUtilization)
{
    qreal util = m_tester->gpuAllUtilization();
    EXPECT_GE(util, 0.0);
    EXPECT_LE(util, 100.0);
}

TEST_F(UT_GPUInfoModel, test_gpuAllMemoryUtilization)
{
    qreal util = m_tester->gpuAllMemoryUtilization();
    EXPECT_GE(util, 0.0);
    EXPECT_LE(util, 100.0);
}

TEST_F(UT_GPUInfoModel, test_gpuMemoryStats)
{
    quint64 total = m_tester->gpuTotalMemory();
    quint64 used = m_tester->gpuUsedMemory();
    quint64 free = m_tester->gpuFreeMemory();
    
    // Memory stats should be consistent when memory info is reported
    if (total > 0)
        EXPECT_EQ(total, used + free);
}

TEST_F(UT_GPUInfoModel, test_perGPUUtilization)
{
    for (int i = 0; i < m_tester->gpuCount(); i++) {
        qreal util = m_tester->gpuUtilization(i);
        EXPECT_GE(util, 0.0);
        EXPECT_LE(util, 100.0);
    }
}

TEST_F(UT_GPUInfoModel, test_perGPUMemoryUtilization)
{
    for (int i = 0; i < m_tester->gpuCount(); i++) {
        qreal util = m_tester->gpuMemoryUtilization(i);
        EXPECT_GE(util, 0.0);
        EXPECT_LE(util, 100.0);
    }
}

TEST_F(UT_GPUInfoModel, test_perGPUMemory)
{
    for (int i = 0; i < m_tester->gpuCount(); i++) {
        quint64 total = m_tester->gpuTotalMemory(i);
        quint64 used = m_tester->gpuUsedMemory(i);
        quint64 free = m_tester->gpuFreeMemory(i);
        
        if (total > 0)
            EXPECT_EQ(total, used + free);
    }
}

TEST_F(UT_GPUInfoModel, test_gpuTemperature)
{
    for (int i = 0; i < m_tester->gpuCount(); i++) {
        float temp = m_tester->gpuTemperature(i);
        EXPECT_GE(temp, 0.0f);
        EXPECT_LE(temp, 150.0f);  // Reasonable GPU temp range
    }
}

TEST_F(UT_GPUInfoModel, test_gpuName)
{
    for (int i = 0; i < m_tester->gpuCount(); i++) {
        QString name = m_tester->gpuName(i);
        // Name may be empty on some systems, but should be a valid string
        EXPECT_TRUE(name.isNull() == false);
    }
}

TEST_F(UT_GPUInfoModel, test_gpuVendor)
{
    for (int i = 0; i < m_tester->gpuCount(); i++) {
        QString vendor = m_tester->gpuVendor(i);
        EXPECT_FALSE(vendor.isEmpty());
    }
}

TEST_F(UT_GPUInfoModel, test_gpuInfoSet)
{
    GPUInfoSet *gpuSet = m_tester->gpuInfoSet();
    EXPECT_TRUE(gpuSet != nullptr);
}

TEST_F(UT_GPUInfoModel, test_updateModel)
{
    // Should not crash
    m_tester->updateModel();
}

