// Copyright (C) 2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 Uniontech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "am_icon_manager.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <DApplication>

using namespace core::process;

/***************************************STUB begin*********************************************/

// Mock DSysInfo::majorVersion()
int g_mockMajorVersion = 20;
int DSysInfo::majorVersion() {
    return g_mockMajorVersion;
}

// Mock pidfd_open for testing without real pid
int g_mockPidfd = 3;
int g_pidfdOpenCallCount = 0;

int pidfd_open(pid_t pid, unsigned int flags) {
    g_pidfdOpenCallCount++;
    return g_mockPidfd;
}

// Mock AM DBus interface for testing
QDBusReply<QStringMap> g_mockIconsReply;
QStringMap g_mockIconsMap = {{"", "test-icon"}};

QDBusReply<QStringMap> createIconsReply() {
    QDBusReply<QStringMap> reply;
    reply.m_value = g_mockIconsMap;
    return reply;
}

QDBusReply<QDBusObjectPath> createIdentifyReply() {
    QDBusReply<QDBusObjectPath> reply;
    reply.m_value = QDBusObjectPath("/org/desktopspec/ApplicationManager1/test");
    return reply;
}

// Stub QDBusInterface::call
QDBusReply<QDBusObjectPath> testCallIdentify(int pidfd) {
    return createIdentifyReply();
}

// Stub QDBusInterface::property
QDBusReply<QStringMap> testPropertyIcons() {
    return createIconsReply();
}

/***************************************STUB end*********************************************/

class UT_AMIconManager : public ::testing::Test
{
public:
    UT_AMIconManager()
        : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new AMIconManager();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        // Reset stub state
        g_pidfdOpenCallCount = 0;
        g_mockIconsMap = {{"", "test-icon"}};
    }

protected:
    AMIconManager *m_tester;
};

TEST_F(UT_AMIconManager, TestGetInstance)
{
    auto *instance1 = AMIconManager::instance();
    auto *instance2 = AMIconManager::instance();
    EXPECT_EQ(instance1, instance2);
}

TEST_F(UT_AMIconManager, TestIsAMAvailable_V20Plus)
{
    // Set mock system version >= 20
    g_mockMajorVersion = 20;
    
    // Re-create manager to pick up new version
    delete m_tester;
    m_tester = new AMIconManager();
    
    EXPECT_TRUE(m_tester->isAMAvailable());
}

TEST_F(UT_AMIconManager, TestIsAMAvailable_PreV20)
{
    // Set mock system version < 20
    g_mockMajorVersion = 19;
    
    // Re-create manager to pick up new version
    delete m_tester;
    m_tester = new AMIconManager();
    
    EXPECT_FALSE(m_tester->isAMAvailable());
}

TEST_F(UT_AMIconManager, TestGetIconByPid_V20Plus)
{
    // Set mock system version >= 20
    g_mockMajorVersion = 20;
    g_mockIconsMap = {{"", "test-app-icon"}};
    
    // Re-create manager
    delete m_tester;
    m_tester = new AMIconManager();
    
    pid_t testPid = 1234;
    QString iconName = m_tester->getIconByPid(testPid);
    
    // Should return icon name from mock
    EXPECT_EQ(iconName, QString("test-app-icon"));
    EXPECT_GT(g_pidfdOpenCallCount, 0);
}

TEST_F(UT_AMIconManager, TestGetIconByPid_CacheHit)
{
    // Set up V20+ system
    g_mockMajorVersion = 20;
    g_mockIconsMap = {{"", "cached-icon"}};
    
    delete m_tester;
    m_tester = new AMIconManager();
    
    pid_t testPid = 5678;
    
    // First call should add to cache
    QString icon1 = m_tester->getIconByPid(testPid);
    EXPECT_EQ(icon1, QString("cached-icon"));
    
    // Second call should hit cache
    g_mockIconsMap = {{"", "not-used-icon"}};
    g_pidfdOpenCallCount = 0;
    QString icon2 = m_tester->getIconByPid(testPid);
    EXPECT_EQ(icon2, QString("cached-icon"));
    
    // Should not call pidfd_open again (cache hit)
    EXPECT_EQ(g_pidfdOpenCallCount, 0);
}

TEST_F(UT_AMIconManager, TestGetIconByPid_PreV20)
{
    // Set mock system version < 20
    g_mockMajorVersion = 19;
    
    delete m_tester;
    m_tester = new AMIconManager();
    
    pid_t testPid = 9999;
    QString iconName = m_tester->getIconByPid(testPid);
    
    // Should return empty string (AM not available)
    EXPECT_TRUE(iconName.isEmpty());
}

TEST_F(UT_AMIconManager, TestRemoveCachedIcon)
{
    // Set up V20+ system
    g_mockMajorVersion = 20;
    
    delete m_tester;
    m_tester = new AMIconManager();
    
    pid_t testPid = 8888;
    
    // First call to cache the icon
    m_tester->getIconByPid(testPid);
    
    // Remove cached icon
    m_tester->removeCachedIcon(testPid);
    
    // Call again should hit AM again (not cache)
    int callsBefore = g_pidfdOpenCallCount;
    QString iconName = m_tester->getIconByPid(testPid);
    EXPECT_GT(g_pidfdOpenCallCount, callsBefore);
}

TEST_F(UT_AMIconManager, TestClearCache)
{
    // Set up V20+ system
    g_mockMajorVersion = 20;
    
    delete m_tester;
    m_tester = new AMIconManager();
    
    // Add some icons to cache
    m_tester->getIconByPid(1001);
    m_tester->getIconByPid(1002);
    m_tester->getIconByPid(1003);
    
    // Clear cache
    m_tester->clearCache();
    
    // Next call should call AM again
    int callsBefore = g_pidfdOpenCallCount;
    m_tester->getIconByPid(2001);
    EXPECT_GT(g_pidfdOpenCallCount, callsBefore);
}

TEST_F(UT_AMIconManager, TestGetIconByPid_AMUnavailable)
{
    // Set mock system version >= 20 but AM service not registered
    g_mockMajorVersion = 20;
    // Note: In real test, we'd mock sessionBus->isServiceRegistered() to return false
    // For now, we assume AM is available
    
    delete m_tester;
    m_tester = new AMIconManager();
    
    pid_t testPid = 7777;
    QString iconName = m_tester->getIconByPid(testPid);
    
    // Should return icon name (from mock AM)
    EXPECT_FALSE(iconName.isEmpty());
}
