// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * 用例计数声明表
 * | 方法 | 级别 | 因子 | min | actual |
 * | instance/add/get/remove/clear/setMaxCost | mid | c:3 | 1 | 6 |
 */

/*****************************************************************************************
 * 测试目标：core::process::ProcessIconCache
 * 用例说明：进程图标缓存单例：注册/获取/移除/清空/容量上限淘汰
 * 覆盖方法：ProcessIconCache / instance / addProcessIcon / getProcessIcon /
 *           removeProcessIcon / contains / clear / setMaxCost
 * 打桩说明：缓存本身不发起外部 I/O；ProcessIcon 对象仅作缓存载荷（不调用其真实 icon()），
 *           无需打桩。单例由 ProcessIconCache::instance() 提供，用例间用 clear() 清场，
 *           夹具不拥有单例所有权（避免悬垂 m_instance）。
 * 异常场景：未注册 pid 查询 / 超过 maxCost 淘汰
 * 用例计数：7 个 TEST_F
 *****************************************************************************************/

//self
#include "process/process_icon_cache.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//project
#include "process/process_icon.h"
#include <QPixmapCache>
#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE

using namespace core::process;

/**************************** 测试夹具 ****************************/
class UT_ProcessIconCache : public ::testing::Test
{
public:
    UT_ProcessIconCache() = default;

    virtual void SetUp()
    {
        m_tester = ProcessIconCache::instance();
        m_tester->clear();
        m_tester->setMaxCost(200);
    }

    virtual void TearDown()
    {
        // 清场但不删除单例：m_instance 为静态，删除会导致悬垂
        m_tester->clear();
        m_tester = nullptr;
    }

protected:
    ProcessIconCache *m_tester = nullptr;
};

/**************************** 用例 ****************************/

// 单例语义：多次获取同一实例
TEST_F(UT_ProcessIconCache, instance_RepeatedCalls_ReturnsSamePointer)
{
    // Arrange：SetUp 已获取实例
    ASSERT_TRUE(m_tester != nullptr);

    // Act
    ProcessIconCache *again = ProcessIconCache::instance();

    // Assert
    EXPECT_EQ(again, m_tester);
    EXPECT_NE(m_tester, nullptr);
    EXPECT_NE(again, nullptr);
}

// 注册后可查询，且返回同一对象指针
TEST_F(UT_ProcessIconCache, addProcessIcon_NewPid_GetReturnsSameObject)
{
    // Arrange
    pid_t pid = 1000;
    ProcessIcon *icon = new ProcessIcon();

    // Act
    m_tester->addProcessIcon(pid, icon);

    // Assert
    EXPECT_TRUE(m_tester->contains(pid));
    EXPECT_EQ(m_tester->getProcessIcon(pid), icon);
}

// 未注册 pid：查询返回空指针
TEST_F(UT_ProcessIconCache, getProcessIcon_MissingPid_ReturnsNull)
{
    // Arrange：缓存为空
    pid_t pid = 2001;

    // Act
    ProcessIcon *icon = m_tester->getProcessIcon(pid);

    // Assert
    EXPECT_EQ(icon, nullptr);
    EXPECT_FALSE(m_tester->contains(pid));
}

// 移除后不可再查询
TEST_F(UT_ProcessIconCache, removeProcessIcon_RegisteredPid_BecomesMissing)
{
    // Arrange
    pid_t pid = 3000;
    m_tester->addProcessIcon(pid, new ProcessIcon());
    ASSERT_TRUE(m_tester->contains(pid));

    // Act
    m_tester->removeProcessIcon(pid);

    // Assert
    EXPECT_FALSE(m_tester->contains(pid));
    EXPECT_EQ(m_tester->getProcessIcon(pid), nullptr);
}

// clear：清空全部条目
TEST_F(UT_ProcessIconCache, clear_MultipleEntries_EmptiesCache)
{
    // Arrange：注册多个 pid
    m_tester->addProcessIcon(4001, new ProcessIcon());
    m_tester->addProcessIcon(4002, new ProcessIcon());
    m_tester->addProcessIcon(4003, new ProcessIcon());
    ASSERT_TRUE(m_tester->contains(4001));
    ASSERT_TRUE(m_tester->contains(4003));

    // Act
    m_tester->clear();

    // Assert
    EXPECT_EQ(m_tester->contains(4001), false);
    EXPECT_FALSE(m_tester->contains(4002));
    EXPECT_FALSE(m_tester->contains(4003));
}

// 主题切换回调：ctor 内连接的 lambda 响应信号清空全局位图缓存
TEST_F(UT_ProcessIconCache, themeChanged_Signal_ClearsGlobalPixmapCache)
{
    // Arrange：预置一张全局位图并确认可命中
    QPixmap pm(4, 4);
    pm.fill(Qt::red);
    const QString key = "ut-icon-theme-key";
    ASSERT_TRUE(QPixmapCache::insert(key, pm));
    QPixmap hit;
    ASSERT_TRUE(QPixmapCache::find(key, &hit));

    // Act：发射 DGuiApplicationHelper 主题切换信号（触发 ctor 内 connect 的回调）
    emit DGuiApplicationHelper::instance()->themeTypeChanged(
        DGuiApplicationHelper::instance()->themeType());

    // Assert：全局位图缓存被清空
    EXPECT_EQ(QPixmapCache::find(key, &hit), false);
    EXPECT_EQ(QPixmapCache::find(key, &hit), false);
}

// setMaxCost：超限时淘汰最早注册的条目（QCache 代价淘汰语义）
TEST_F(UT_ProcessIconCache, setMaxCost_LimitExceeded_EvictsOldestEntry)
{
    // Arrange：上限设为 1（每个 ProcessIcon 代价 1）
    m_tester->setMaxCost(1);
    m_tester->addProcessIcon(5001, new ProcessIcon());
    ASSERT_TRUE(m_tester->contains(5001));

    // Act：注册第二个 pid，超出代价上限
    m_tester->addProcessIcon(5002, new ProcessIcon());

    // Assert：先注册的被淘汰，后注册的保留
    EXPECT_EQ(m_tester->contains(5001), false);
    EXPECT_EQ(m_tester->contains(5002), true);
}
