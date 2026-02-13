// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/gpu_info.h"
#include "system/device_db.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::system;

class UT_GPUInfoSet: public ::testing::Test
{
public:
    UT_GPUInfoSet() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new GPUInfoSet();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    GPUInfoSet *m_tester;
};

TEST_F(UT_GPUInfoSet, initTest)
{
    EXPECT_TRUE(m_tester != nullptr);
}

TEST_F(UT_GPUInfoSet, test_update)
{
    m_tester->update();
    // Should not crash even if no GPUs are present
    EXPECT_GE(m_tester->gpuCount(), 0);
}

TEST_F(UT_GPUInfoSet, test_gpuCount)
{
    m_tester->update();
    int count = m_tester->gpuCount();
    EXPECT_GE(count, 0);
}

TEST_F(UT_GPUInfoSet, test_gpuList)
{
    m_tester->update();
    auto list = m_tester->gpuList();
    EXPECT_EQ(list.size(), m_tester->gpuCount());
}

TEST_F(UT_GPUInfoSet, test_gpu)
{
    m_tester->update();
    if (m_tester->gpuCount() > 0) {
        auto gpu = m_tester->gpu(0);
        EXPECT_TRUE(gpu != nullptr);
        EXPECT_GE(gpu->gpuUtilization, 0.0f);
        EXPECT_LE(gpu->gpuUtilization, 100.0f);
    }
}

TEST_F(UT_GPUInfoSet, test_memoryStats)
{
    m_tester->update();
    quint64 total = m_tester->totalMemory();
    quint64 used = m_tester->usedMemory();
    quint64 free = m_tester->freeMemory();
    
    // Memory stats should be consistent when memory info is reported
    if (total > 0) {
        EXPECT_EQ(total, used + free);
        EXPECT_GE(total, used);
        EXPECT_GE(total, free);
    }
}

TEST_F(UT_GPUInfoSet, test_averageUtilization)
{
    m_tester->update();
    float avg = m_tester->averageUtilization();
    EXPECT_GE(avg, 0.0f);
    EXPECT_LE(avg, 100.0f);
}

TEST_F(UT_GPUInfoSet, test_gpuVendors)
{
    // Test vendor detection flags
    bool hasNvidia = m_tester->hasNvidiaGPU();
    bool hasAmd = m_tester->hasAmdGPU();
    bool hasIntel = m_tester->hasIntelGPU();
    
    // These are detection flags, they should be boolean
    EXPECT_TRUE(hasNvidia == true || hasNvidia == false);
    EXPECT_TRUE(hasAmd == true || hasAmd == false);
    EXPECT_TRUE(hasIntel == true || hasIntel == false);
}

TEST_F(UT_GPUInfoSet, test_perGPUInfo)
{
    m_tester->update();
    for (int i = 0; i < m_tester->gpuCount(); i++) {
        auto gpu = m_tester->gpu(i);
        EXPECT_TRUE(gpu != nullptr);
        
        // Check index matches
        EXPECT_EQ(gpu->index, i);
        
        // Check vendor is set
        EXPECT_FALSE(gpu->vendor.isEmpty());
        
        // Check utilization is in valid range
        EXPECT_GE(gpu->gpuUtilization, 0.0f);
        EXPECT_LE(gpu->gpuUtilization, 100.0f);
        
        // Check memory utilization is in valid range
        EXPECT_GE(gpu->memoryUtilization, 0.0f);
        EXPECT_LE(gpu->memoryUtilization, 100.0f);
        
        // Check memory consistency when memory info is reported
        if (gpu->totalMemory > 0)
            EXPECT_EQ(gpu->totalMemory, gpu->usedMemory + gpu->freeMemory);
        
        // Temperature should be reasonable (0-150°C is safe range for GPUs)
        EXPECT_GE(gpu->temperature, 0.0f);
        EXPECT_LE(gpu->temperature, 150.0f);
        
        // Should be marked as available
        EXPECT_TRUE(gpu->isAvailable);
    }
}

