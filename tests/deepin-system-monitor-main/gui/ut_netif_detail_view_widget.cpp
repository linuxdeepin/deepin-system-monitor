// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "netif_detail_view_widget.h"
#include "netif_stat_view_widget.h"
#include "netif_summary_view_widget.h"
#include "netif_item_view_widget.h"
#include "system/device_db.h"
#include "system/netif.h"
#include "system/netif_info_db.h"

//gtest
#include <gtest/gtest.h>

//Qt
#include <QFont>

using namespace core::system;

/***************************************FIXTURE begin*****************************************/
// 数据注入说明：测试构建为 -O2，NetifInfoDB::infoDB() 在头文件内联定义且被生产代码
// 全部内联（目标文件中无符号引用），首字节补丁式打桩无法生效。因此借助测试构建的
// -fno-access-control 直接向 DeviceDB 单例的 m_infoDB 写入合成数据，隔离真实系统数据源
// （不触发 Netlink//proc 读取），各测试结束时通过 clearInfoDB() 还原。
static const QMap<QByteArray, NetifInfoPtr> stub_infoDB_one()
{
    QMap<QByteArray, NetifInfoPtr> map;
    auto info = std::make_shared<NetifInfo>();
    info->set_recv_bps(1024);
    info->set_sent_bps(512);
    map.insert("aa", info);
    return map;
}

static const QMap<QByteArray, NetifInfoPtr> stub_infoDB_empty()
{
    return QMap<QByteArray, NetifInfoPtr>();
}

static void injectInfoDB(const QMap<QByteArray, NetifInfoPtr> &map)
{
    DeviceDB::instance()->netifInfoDB()->m_infoDB = map;
}

static void clearInfoDB()
{
    DeviceDB::instance()->netifInfoDB()->m_infoDB.clear();
}
/***************************************FIXTURE end**********************************************/

class UT_NetifDetailViewWidget : public ::testing::Test
{
public:
    UT_NetifDetailViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        clearInfoDB();
        static QWidget parent;
        m_tester = new NetifDetailViewWidget(&parent);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        clearInfoDB();
    }

protected:
    NetifDetailViewWidget *m_tester;
};

TEST_F(UT_NetifDetailViewWidget, initTest)
{
    // 构造函数：网络概览 + 网络详情两块子视图装配，标题翻译
    ASSERT_NE(m_tester, nullptr);
    EXPECT_NE(m_tester->m_netifstatWIdget, nullptr);
    EXPECT_NE(m_tester->m_netifsummaryWidget, nullptr);
    EXPECT_EQ(m_tester->m_netifstatWIdget->parent(), m_tester);
    EXPECT_EQ(m_tester->m_netifsummaryWidget->parent(), m_tester);
    EXPECT_EQ(m_tester->objectName(), QString("NetifDetailViewWidget"));
}

// 分支说明(updateData): 转发 onModelUpdate -> 按当前数据源刷新统计/详情子视图
TEST_F(UT_NetifDetailViewWidget, test_updateData_01)
{
    // 空数据源：子视图保持初始状态
    m_tester->updateData();

    EXPECT_FALSE(m_tester->m_netifstatWIdget->m_initStatus);
    EXPECT_TRUE(m_tester->m_netifstatWIdget->m_mapItemView.isEmpty());
}

TEST_F(UT_NetifDetailViewWidget, test_updateData_02_withData)
{
    // 有数据源：统计子视图按 MAC 创建条目
    injectInfoDB(stub_infoDB_one());

    m_tester->updateData();

    EXPECT_TRUE(m_tester->m_netifstatWIdget->m_initStatus);
    EXPECT_EQ(m_tester->m_netifstatWIdget->m_mapItemView.size(), 1);
    EXPECT_TRUE(m_tester->m_netifstatWIdget->m_mapItemView.contains("aa"));
    // 概览子视图联动（信号 netifItemClicked -> onNetifItemClicked）：
    // 无 IPv4/IPv6 地址 -> 连接类型+MAC+带宽+RX(6)+TX(6) = 15 行
    EXPECT_EQ(m_tester->m_netifsummaryWidget->m_netInfoModel->rowCount(QModelIndex()), 15);
}

// 分支说明(detailFontChanged): 基类标题字体 + 统计/详情子视图字体同步
TEST_F(UT_NetifDetailViewWidget, test_detailFontChanged_01)
{
    // 先注入数据并创建条目，便于验证字体转发
    injectInfoDB(stub_infoDB_one());
    m_tester->updateData();
    ASSERT_EQ(m_tester->m_netifstatWIdget->m_mapItemView.size(), 1);

    QFont font;
    font.setBold(true);
    font.setPointSize(15);
    m_tester->detailFontChanged(font);

    // 统计子视图：字体转发到条目（fontChanged 不改自身字体）
    EXPECT_EQ(m_tester->m_netifstatWIdget->m_mapItemView.value("aa")->font(), font);
    // 详情子视图委托字体已同步
    EXPECT_EQ(m_tester->m_netifsummaryWidget->m_netInfoDetailItemDelegate->m_font, font);
    // 表格高度固定 260
    EXPECT_EQ(m_tester->m_netifsummaryWidget->minimumHeight(), 260);
}
