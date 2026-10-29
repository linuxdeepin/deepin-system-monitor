// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "netif_stat_view_widget.h"
#include "netif_item_view_widget.h"
#include "chart_view_widget.h"
#include "system/device_db.h"
#include "system/netif.h"
#include "system/netif_info_db.h"

//gtest
#include <gtest/gtest.h>

//Qt
#include <QSignalSpy>
#include <QResizeEvent>

using namespace core::system;

/***************************************FIXTURE begin******************************************/
// 数据注入说明：测试构建为 -O2，NetifInfoDB::infoDB() 在头文件内联定义且被生产代码
// 全部内联（目标文件中无符号引用），首字节补丁式打桩无法生效。因此借助测试构建的
// -fno-access-control 直接向 DeviceDB 单例的 m_infoDB 写入合成数据，隔离真实系统数据源
// （不触发 Netlink//proc 读取），TearDown 时清空还原。
static std::shared_ptr<NetifInfo> makeNetifInfo(qreal recvBps, qreal sentBps)
{
    auto info = std::make_shared<NetifInfo>();
    info->set_recv_bps(recvBps);
    info->set_sent_bps(sentBps);
    return info;
}

static QMap<QByteArray, NetifInfoPtr> makeInfoDB(int count)
{
    QMap<QByteArray, NetifInfoPtr> map;
    for (int i = 0; i < count; ++i) {
        map.insert(QString("mac%1").arg(i + 1).toLocal8Bit(),
                   makeNetifInfo(1024 * 1024 * (i + 1), 2048 * (i + 1)));
    }
    return map;
}

static void injectInfoDB(const QMap<QByteArray, NetifInfoPtr> &map)
{
    DeviceDB::instance()->netifInfoDB()->m_infoDB = map;
}

static void clearInfoDB()
{
    DeviceDB::instance()->netifInfoDB()->m_infoDB.clear();
}
/***************************************FIXTURE end********************************************/

class UT_NetifStatViewWidget : public ::testing::Test
{
public:
    UT_NetifStatViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        clearInfoDB();
        static QWidget parent;
        m_tester = new NetifStatViewWidget(&parent);
        m_tester->resize(400, 300);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        clearInfoDB();
    }

    // 向 m_mapItemView 插入测试条目（父对象挂到 m_centralWidget，随 tester 一并销毁）
    NetifItemViewWidget *insertItem(const QByteArray &mac)
    {
        auto *item = new NetifItemViewWidget(m_tester->m_centralWidget, mac);
        m_tester->m_mapItemView.insert(mac, item);
        return item;
    }

protected:
    NetifStatViewWidget *m_tester;
};

TEST_F(UT_NetifStatViewWidget, initTest)
{
    // 构造函数：中央窗口挂载 + DB 指针绑定
    ASSERT_NE(m_tester, nullptr);
    ASSERT_EQ(m_tester->widget(), m_tester->m_centralWidget);
    ASSERT_EQ(m_tester->m_info, DeviceDB::instance()->netifInfoDB());
    EXPECT_FALSE(m_tester->m_initStatus);
    EXPECT_TRUE(m_tester->m_mapItemView.isEmpty());
    EXPECT_TRUE(m_tester->m_currentMac.isEmpty());
}

// 分支说明(onModelUpdate): 新MAC->创建条目并updateData / 旧MAC->仅updateData;
//                         !init&&cnt>0: cnt==1直接emit / cnt>1走onSetItemActiveStatus;
//                         cnt>0且currentMac消失: 单网卡置非激活 / 多网卡置激活后重选
TEST_F(UT_NetifStatViewWidget, test_onModelUpdate_01_createNewItem)
{
    // DB 中出现新 MAC -> 创建条目；单网卡首次初始化直接发出 netifItemClicked
    injectInfoDB(makeInfoDB(1));

    QSignalSpy spy(m_tester, &NetifStatViewWidget::netifItemClicked);
    m_tester->onModelUpdate();

    EXPECT_EQ(m_tester->m_mapItemView.size(), 1);
    EXPECT_TRUE(m_tester->m_mapItemView.contains("mac1"));
    EXPECT_TRUE(m_tester->m_initStatus);
    EXPECT_EQ(m_tester->m_currentMac, QByteArray("mac1"));
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString("mac1"));
    // 条目数据来自注入的 NetifInfo
    EXPECT_EQ(m_tester->m_mapItemView.value("mac1")->m_ChartWidget->m_listData1.size(), 1);
}

TEST_F(UT_NetifStatViewWidget, test_onModelUpdate_02_updateExistingItem)
{
    // 已有条目再次刷新 -> 不重复创建，仅更新数据，且重复刷新不再发出信号
    injectInfoDB(makeInfoDB(2));

    QSignalSpy spy(m_tester, &NetifStatViewWidget::netifItemClicked);
    m_tester->onModelUpdate();
    ASSERT_EQ(m_tester->m_mapItemView.size(), 2);
    ASSERT_EQ(spy.count(), 1);

    NetifItemViewWidget *item1 = m_tester->m_mapItemView.value("mac1");
    NetifItemViewWidget *item2 = m_tester->m_mapItemView.value("mac2");
    ASSERT_NE(item1, nullptr);
    ASSERT_NE(item2, nullptr);

    m_tester->onModelUpdate();

    EXPECT_EQ(m_tester->m_mapItemView.size(), 2);
    EXPECT_EQ(m_tester->m_mapItemView.value("mac1"), item1);
    EXPECT_EQ(m_tester->m_mapItemView.value("mac2"), item2);
    EXPECT_EQ(m_tester->m_currentMac, QByteArray("mac1"));
    // 第二次刷新：init 已完成且 currentMac 仍在 DB -> 无信号
    EXPECT_EQ(spy.count(), 1);
    // onSetItemActiveStatus 分支：netCount>1 且 key 匹配 -> 激活
    EXPECT_TRUE(item1->m_isActive);
    EXPECT_FALSE(item2->m_isActive);
}

TEST_F(UT_NetifStatViewWidget, test_onModelUpdate_03_resetCurrentMacWhenGone)
{
    // 当前激活 MAC 从 DB 消失 -> 重置为第一个，单网卡置为非激活并发出信号
    injectInfoDB(makeInfoDB(1));

    m_tester->m_initStatus = true;
    m_tester->m_currentMac = "gone";
    NetifItemViewWidget *item = insertItem("mac1");
    item->updateActiveStatus(true);

    QSignalSpy spy(m_tester, &NetifStatViewWidget::netifItemClicked);
    m_tester->onModelUpdate();

    EXPECT_EQ(m_tester->m_currentMac, QByteArray("mac1"));
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString("mac1"));
    // netCount == 1 的重置分支：updateActiveStatus(false)
    EXPECT_FALSE(item->m_isActive);
}

TEST_F(UT_NetifStatViewWidget, test_onModelUpdate_05_resetCurrentMacMultiNet)
{
    // 当前激活 MAC 从 DB 消失且 DB 有多块网卡 -> 第一个 item 置为激活并发出信号
    injectInfoDB(makeInfoDB(2));

    m_tester->m_initStatus = true;
    m_tester->m_currentMac = "gone";
    insertItem("mac1");
    insertItem("mac2");

    QSignalSpy spy(m_tester, &NetifStatViewWidget::netifItemClicked);
    m_tester->onModelUpdate();

    EXPECT_EQ(m_tester->m_currentMac, QByteArray("mac1"));
    EXPECT_EQ(spy.count(), 1);
    ASSERT_GT(spy.count(), 0);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString("mac1"));
    // netCount > 1 的重置分支：begin 项 updateActiveStatus(true)
    EXPECT_TRUE(m_tester->m_mapItemView.value(QByteArray("mac1"))->m_isActive);
}

TEST_F(UT_NetifStatViewWidget, test_onModelUpdate_04_emptyDB)
{
    // 空数据源：不创建条目、不触发初始化
    injectInfoDB(makeInfoDB(0));

    QSignalSpy spy(m_tester, &NetifStatViewWidget::netifItemClicked);
    m_tester->onModelUpdate();

    EXPECT_TRUE(m_tester->m_mapItemView.isEmpty());
    EXPECT_FALSE(m_tester->m_initStatus);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(UT_NetifStatViewWidget, test_onSetItemActiveStatus_01_singleNet)
{
    // netCount == 1：所有条目置为非激活，不发出 netifItemClicked
    injectInfoDB(makeInfoDB(1));

    NetifItemViewWidget *item = insertItem("mac1");
    item->updateActiveStatus(true);

    QSignalSpy spy(m_tester, &NetifStatViewWidget::netifItemClicked);
    m_tester->onSetItemActiveStatus("mac1");

    EXPECT_FALSE(item->m_isActive);
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(m_tester->m_currentMac, QByteArray("mac1"));
}

TEST_F(UT_NetifStatViewWidget, test_onSetItemActiveStatus_02_multiNetMatch)
{
    // netCount > 1 且 MAC 匹配：激活匹配项并发出 netifItemClicked
    injectInfoDB(makeInfoDB(2));

    NetifItemViewWidget *item1 = insertItem("mac1");
    NetifItemViewWidget *item2 = insertItem("mac2");
    item2->updateActiveStatus(true);

    QSignalSpy spy(m_tester, &NetifStatViewWidget::netifItemClicked);
    m_tester->onSetItemActiveStatus("mac1");

    EXPECT_TRUE(item1->m_isActive);
    EXPECT_FALSE(item2->m_isActive);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString("mac1"));
    EXPECT_EQ(m_tester->m_currentMac, QByteArray("mac1"));
}

TEST_F(UT_NetifStatViewWidget, test_onSetItemActiveStatus_03_multiNetNoMatch)
{
    // netCount > 1 但 MAC 不在条目集合中：全部置为非激活
    injectInfoDB(makeInfoDB(2));

    NetifItemViewWidget *item1 = insertItem("mac1");
    NetifItemViewWidget *item2 = insertItem("mac2");

    QSignalSpy spy(m_tester, &NetifStatViewWidget::netifItemClicked);
    m_tester->onSetItemActiveStatus("notexist");

    EXPECT_FALSE(item1->m_isActive);
    EXPECT_FALSE(item2->m_isActive);
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(m_tester->m_currentMac, QByteArray("notexist"));
}

TEST_F(UT_NetifStatViewWidget, test_fontChanged_01)
{
    // 字体变化：转发给所有条目
    NetifItemViewWidget *item1 = insertItem("mac1");
    NetifItemViewWidget *item2 = insertItem("mac2");

    QFont font;
    font.setBold(true);
    font.setPointSize(14);
    m_tester->fontChanged(font);

    EXPECT_EQ(item1->font(), font);
    EXPECT_EQ(item2->font(), font);
}

TEST_F(UT_NetifStatViewWidget, test_resizeEvent_01)
{
    // resize 事件 -> updateWidgetGeometry -> 单网卡铺满布局
    injectInfoDB(makeInfoDB(1));
    insertItem("mac1");

    QResizeEvent event(QSize(400, 300), QSize(200, 200));
    m_tester->resizeEvent(&event);

    EXPECT_EQ(m_tester->m_centralWidget->size(), QSize(400, 300));
}

TEST_F(UT_NetifStatViewWidget, test_updateWidgetGeometry_01_single)
{
    // netCount == 1 -> showItemOnlyeOne：横向标题模式
    injectInfoDB(makeInfoDB(1));
    NetifItemViewWidget *item = insertItem("mac1");

    m_tester->updateWidgetGeometry();

    EXPECT_EQ(item->m_mode, NetifItemViewWidget::TITLE_HORIZONTAL);
    EXPECT_EQ(item->geometry(), QRect(0, 0, 400, 300));
    EXPECT_EQ(m_tester->m_centralWidget->size(), QSize(400, 300));
}

TEST_F(UT_NetifStatViewWidget, test_updateWidgetGeometry_02_double)
{
    // netCount == 2 -> showItemDouble：纵向标题模式
    injectInfoDB(makeInfoDB(2));
    NetifItemViewWidget *item1 = insertItem("mac1");
    insertItem("mac2");

    m_tester->updateWidgetGeometry();

    EXPECT_EQ(item1->m_mode, NetifItemViewWidget::TITLE_VERTICAL);
    // 第一项宽度 = (400 - itemSpace(6)) / 2 = 197
    EXPECT_EQ(item1->geometry(), QRect(0, 0, 197, 300));
    EXPECT_EQ(m_tester->m_centralWidget->size(), QSize(400, 300));
}

TEST_F(UT_NetifStatViewWidget, test_updateWidgetGeometry_03_lgDouble)
{
    // netCount > 2 -> showItemLgDouble：分页布局
    injectInfoDB(makeInfoDB(3));
    insertItem("mac1");
    insertItem("mac2");
    NetifItemViewWidget *item3 = insertItem("mac3");

    m_tester->updateWidgetGeometry();

    EXPECT_EQ(item3->m_mode, NetifItemViewWidget::TITLE_VERTICAL);
    // 3 个条目：itemCurrentCnt 0..2，page 恒为 0 -> 中央窗口宽高不变
    EXPECT_EQ(m_tester->m_centralWidget->size(), QSize(400, 300));
}

TEST_F(UT_NetifStatViewWidget, test_updateWidgetGeometry_04_emptyDBNoLayout)
{
    // netCount == 0：三个布局分支都不进入，不做任何布局
    injectInfoDB(makeInfoDB(0));
    NetifItemViewWidget *item = insertItem("mac1");

    m_tester->updateWidgetGeometry();

    // 条目保持初始模式且未铺满视图（未进入 showItemOnlyeOne 分支）
    EXPECT_EQ(item->m_mode, NetifItemViewWidget::TITLE_HORIZONTAL);
    EXPECT_NE(item->geometry(), QRect(0, 0, 400, 300));
    EXPECT_NE(m_tester->m_centralWidget->size(), QSize(400, 300));
}

// 分支说明(showItemOnlyeOne): inDB->show+TITLE_HORIZONTAL+铺满+非激活 / else->hide+非激活
TEST_F(UT_NetifStatViewWidget, test_showItemOnlyeOne_01)
{
    injectInfoDB(makeInfoDB(3));

    NetifItemViewWidget *inDb = insertItem("mac1");
    inDb->hide();
    inDb->updateActiveStatus(true);
    NetifItemViewWidget *notInDb = insertItem("macX");

    m_tester->showItemOnlyeOne();

    // 在 DB 中的条目：显示、横向模式、铺满整个视图
    EXPECT_FALSE(inDb->isHidden());
    EXPECT_EQ(inDb->m_mode, NetifItemViewWidget::TITLE_HORIZONTAL);
    EXPECT_EQ(inDb->geometry(), QRect(0, 0, 400, 300));
    EXPECT_FALSE(inDb->m_isActive);
    // 不在 DB 中的条目：隐藏
    EXPECT_TRUE(notInDb->isHidden());
    EXPECT_EQ(m_tester->m_centralWidget->size(), QSize(400, 300));
}

// 分支说明(showItemDouble): inDB->show+TITLE_VERTICAL+左右并排 / 匹配currentMac->激活 / else->hide
TEST_F(UT_NetifStatViewWidget, test_showItemDouble_01)
{
    injectInfoDB(makeInfoDB(2));
    m_tester->m_currentMac = "mac1";

    NetifItemViewWidget *item1 = insertItem("mac1");
    item1->hide();
    NetifItemViewWidget *item2 = insertItem("mac2");
    NetifItemViewWidget *ghost = insertItem("macX");

    m_tester->showItemDouble();

    EXPECT_FALSE(item1->isHidden());
    EXPECT_FALSE(item2->isHidden());
    EXPECT_EQ(item1->m_mode, NetifItemViewWidget::TITLE_VERTICAL);
    EXPECT_EQ(item2->m_mode, NetifItemViewWidget::TITLE_VERTICAL);
    EXPECT_EQ(item1->geometry(), QRect(0, 0, 197, 300));
    // 第二项 x 偏移 = itemWidth + itemSpace = 197 + 6 = 203
    EXPECT_EQ(item2->geometry(), QRect(203, 0, 197, 300));
    // currentMac 匹配 -> 激活
    EXPECT_TRUE(item1->m_isActive);
    EXPECT_FALSE(item2->m_isActive);
    EXPECT_TRUE(ghost->isHidden());
    EXPECT_EQ(m_tester->m_centralWidget->size(), QSize(400, 300));
}

TEST_F(UT_NetifStatViewWidget, test_showItemDouble_02_noCurrentMac)
{
    // 无 currentMac 匹配：不激活任何条目
    injectInfoDB(makeInfoDB(2));
    m_tester->m_currentMac = "other";

    NetifItemViewWidget *item1 = insertItem("mac1");
    NetifItemViewWidget *item2 = insertItem("mac2");

    m_tester->showItemDouble();

    EXPECT_FALSE(item1->m_isActive);
    EXPECT_FALSE(item2->m_isActive);
}

// 分支说明(showItemLgDouble): inDB->TITLE_VERTICAL+按%4四象限排布+currentMac激活 / else->hide;
//                            5个条目触发翻页 page=1 -> 中央窗口宽度翻倍
TEST_F(UT_NetifStatViewWidget, test_showItemLgDouble_01_quadrantLayout)
{
    injectInfoDB(makeInfoDB(5));
    m_tester->m_currentMac = "mac5";

    // itemWidth = (400 - 6) / 2 = 197, itemHeight = 150
    NetifItemViewWidget *item1 = insertItem("mac1");
    NetifItemViewWidget *item2 = insertItem("mac2");
    NetifItemViewWidget *item3 = insertItem("mac3");
    NetifItemViewWidget *item4 = insertItem("mac4");
    NetifItemViewWidget *item5 = insertItem("mac5");

    m_tester->showItemLgDouble();

    EXPECT_EQ(item1->geometry(), QRect(0, 0, 197, 150));            // %4==0 左上
    EXPECT_EQ(item2->geometry(), QRect(203, 0, 197, 150));          // %4==1 右上
    EXPECT_EQ(item3->geometry(), QRect(0, 150, 197, 150));          // %4==2 左下
    EXPECT_EQ(item4->geometry(), QRect(203, 150, 197, 150));        // %4==3 右下
    EXPECT_EQ(item5->geometry(), QRect(400, 0, 197, 150));          // 第5个翻页
    // 第 5 个条目触发翻页：page == 1 -> 中央窗口宽度 = 2 * 视图宽
    EXPECT_EQ(m_tester->m_centralWidget->size(), QSize(800, 300));
    // currentMac == mac5 -> 激活
    EXPECT_TRUE(item5->m_isActive);
    EXPECT_FALSE(item1->m_isActive);
    EXPECT_FALSE(item2->m_isActive);
    EXPECT_FALSE(item3->m_isActive);
    EXPECT_FALSE(item4->m_isActive);
}

TEST_F(UT_NetifStatViewWidget, test_showItemLgDouble_02_hideNotInDb)
{
    // 不在 DB 中的条目：隐藏并取消激活
    injectInfoDB(makeInfoDB(3));

    NetifItemViewWidget *ghost = insertItem("macX");
    ghost->updateActiveStatus(true);

    m_tester->showItemLgDouble();

    EXPECT_TRUE(ghost->isHidden());
    EXPECT_FALSE(ghost->m_isActive);
    // 无翻页 -> 中央窗口保持视图大小
    EXPECT_EQ(m_tester->m_centralWidget->size(), QSize(400, 300));
}
