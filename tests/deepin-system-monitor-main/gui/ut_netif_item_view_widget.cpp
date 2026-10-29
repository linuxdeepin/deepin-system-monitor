// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "netif_item_view_widget.h"
#include "chart_view_widget.h"
#include "system/netif.h"
#include "common/common.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//Qt
#include <QSignalSpy>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPaintEvent>

using namespace core::system;
using namespace common::format;

/***************************************STUB begin*********************************************/
// 打桩说明：本类依赖 ChartViewWidget(纯内存绘制)与 formatUnit_net(纯格式化函数)，
// 均为无外部 I/O 的实现，直接使用真实实现；NetifInfo 为纯数据对象，走真实对象。
/***************************************STUB end**********************************************/

class UT_NetifItemViewWidget : public ::testing::Test
{
public:
    UT_NetifItemViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget parent;
        m_tester = new NetifItemViewWidget(&parent, "testmac");
        m_tester->resize(200, 150);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetifItemViewWidget *m_tester;
};

// 说明：清单中 netif_item_view_widget.cpp 的 QWidget(parent) 自由函数节点
// 对应 Qt 基类构造，非本项目可测代码，其行为由下方构造函数用例覆盖。
TEST_F(UT_NetifItemViewWidget, initTest)
{
    // 构造函数：MAC 绑定 + 图表子控件创建 + 默认横向标题模式
    ASSERT_NE(m_tester, nullptr);
    EXPECT_EQ(m_tester->m_mac, QByteArray("testmac"));
    EXPECT_NE(m_tester->m_ChartWidget, nullptr);
    EXPECT_EQ(m_tester->m_ChartWidget->parent(), m_tester);
    EXPECT_EQ(m_tester->m_mode, NetifItemViewWidget::TITLE_HORIZONTAL);
    EXPECT_TRUE(m_tester->m_isActive == false);
    EXPECT_EQ(m_tester->m_ifname, QString("Ethernet"));
    EXPECT_TRUE(m_tester->m_recv_bps.isEmpty());
    EXPECT_TRUE(m_tester->m_sent_bps.isEmpty());
}

TEST_F(UT_NetifItemViewWidget, test_updateData_01)
{
    // 正常数据：速率写入图表 + 速率文案按 formatUnit_net 转换
    auto info = std::make_shared<NetifInfo>();
    info->set_recv_bps(1048576);
    info->set_sent_bps(2048);

    m_tester->updateData(info);

    EXPECT_EQ(m_tester->m_ChartWidget->m_listData1.size(), 1);
    EXPECT_DOUBLE_EQ(m_tester->m_ChartWidget->m_listData1.last().toDouble(), 1048576.0);
    EXPECT_EQ(m_tester->m_ChartWidget->m_listData2.size(), 1);
    EXPECT_DOUBLE_EQ(m_tester->m_ChartWidget->m_listData2.last().toDouble(), 2048.0);
    EXPECT_EQ(m_tester->m_recv_bps, formatUnit_net(1048576 * 8, B, 1, true));
    EXPECT_EQ(m_tester->m_sent_bps, formatUnit_net(2048 * 8, B, 1, true));
    // 默认 NetifInfo 无网卡名 -> 保持构造时的 "Ethernet"
    EXPECT_EQ(m_tester->m_ifname, QString("Ethernet"));
}

TEST_F(UT_NetifItemViewWidget, test_updateData_02_append)
{
    // 连续刷新：图表数据按次序追加
    auto info1 = std::make_shared<NetifInfo>();
    info1->set_recv_bps(1024);
    info1->set_sent_bps(512);
    auto info2 = std::make_shared<NetifInfo>();
    info2->set_recv_bps(2048);
    info2->set_sent_bps(1024);

    m_tester->updateData(info1);
    m_tester->updateData(info2);

    EXPECT_EQ(m_tester->m_ChartWidget->m_listData1.size(), 2);
    EXPECT_DOUBLE_EQ(m_tester->m_ChartWidget->m_listData1.first().toDouble(), 1024.0);
    EXPECT_DOUBLE_EQ(m_tester->m_ChartWidget->m_listData1.last().toDouble(), 2048.0);
    EXPECT_EQ(m_tester->m_recv_bps, formatUnit_net(2048 * 8, B, 1, true));
}

TEST_F(UT_NetifItemViewWidget, test_updateActiveStatus_01)
{
    // 激活状态切换
    m_tester->updateActiveStatus(true);
    EXPECT_TRUE(m_tester->m_isActive);

    m_tester->updateActiveStatus(false);
    EXPECT_FALSE(m_tester->m_isActive);
}

TEST_F(UT_NetifItemViewWidget, test_setMode_01)
{
    // 模式切换
    m_tester->setMode(NetifItemViewWidget::TITLE_VERTICAL);
    EXPECT_EQ(m_tester->m_mode, NetifItemViewWidget::TITLE_VERTICAL);

    m_tester->setMode(NetifItemViewWidget::TITLE_HORIZONTAL);
    EXPECT_EQ(m_tester->m_mode, NetifItemViewWidget::TITLE_HORIZONTAL);
}

TEST_F(UT_NetifItemViewWidget, test_fontChanged_01)
{
    // 字体更新：保存字体并同步到控件
    QFont font;
    font.setBold(true);
    font.setPointSize(16);
    m_tester->fontChanged(font);

    EXPECT_EQ(m_tester->m_font, font);
    EXPECT_EQ(m_tester->font(), font);
}

TEST_F(UT_NetifItemViewWidget, test_fontChanged_02_geometryRecomputed)
{
    // 字体变化 -> 图表几何按字体高度重新布局（纵向模式）
    m_tester->setMode(NetifItemViewWidget::TITLE_VERTICAL);

    QFont font;
    font.setPointSize(20);
    m_tester->fontChanged(font);

    int fontHeight = QFontMetrics(font).height();
    // 纵向模式: setGeometry(margin, fontHeight*2 + TextSpacing, width-2*margin, height - fontHeight*2 - TextSpacing - margin)
    EXPECT_EQ(m_tester->m_ChartWidget->geometry(),
              QRect(6, fontHeight * 2 + 12, 200 - 2 * 6, 150 - fontHeight * 2 - 12 - 6));
}

// 分支说明(updateWidgetGeometry): TITLE_HORIZONTAL->图表铺满去标题区 / else->按字体高度留标题区
TEST_F(UT_NetifItemViewWidget, test_updateWidgetGeometry_01_horizontal)
{
    // 横向模式：图表从 TextSpacing(12) 起，高度去掉标题区与底部边距
    m_tester->updateWidgetGeometry();

    EXPECT_EQ(m_tester->m_ChartWidget->geometry(), QRect(0, 12, 200, 150 - 12 - 6));
}

TEST_F(UT_NetifItemViewWidget, test_updateWidgetGeometry_02_vertical)
{
    // 纵向模式：x 偏移 margin(6)，y 偏移 2*字体高 + TextSpacing(12)
    m_tester->setMode(NetifItemViewWidget::TITLE_VERTICAL);
    QFont font;
    font.setPointSize(12);
    m_tester->m_font = font;
    m_tester->updateWidgetGeometry();

    int fontHeight = QFontMetrics(font).height();
    EXPECT_EQ(m_tester->m_ChartWidget->geometry(),
              QRect(6, fontHeight * 2 + 12, 200 - 12, 150 - fontHeight * 2 - 18));
}

TEST_F(UT_NetifItemViewWidget, test_resizeEvent_01)
{
    // 尺寸变化 -> 图表几何随视图重算
    QResizeEvent event(QSize(200, 150), QSize(300, 200));
    m_tester->resizeEvent(&event);

    EXPECT_EQ(m_tester->m_ChartWidget->geometry(), QRect(0, 12, 200, 150 - 18));
}

TEST_F(UT_NetifItemViewWidget, test_mousePressEvent_01)
{
    // 点击 -> 发出 clicked 信号并携带本条目 MAC
    QSignalSpy spy(m_tester, &NetifItemViewWidget::clicked);

    QMouseEvent press(QEvent::MouseButtonPress, QPointF(10, 10), QPointF(10, 10),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->mousePressEvent(&press);

    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString("testmac"));
}

// 分支说明(paintEvent): m_isActive 高亮->画选中底色+高亮描边 / 非激活->TextTips 描边;
//                      TITLE_HORIZONTAL 横排文案 / TITLE_VERTICAL 纵排文案
TEST_F(UT_NetifItemViewWidget, test_paintEvent_01_inactiveHorizontal)
{
    // 非激活 + 横向模式：绘制不崩溃
    QPaintEvent event(m_tester->rect());
    m_tester->paintEvent(&event);

    EXPECT_FALSE(m_tester->m_isActive);
}

TEST_F(UT_NetifItemViewWidget, test_paintEvent_02_activeVertical)
{
    // 激活 + 纵向模式：走高亮绘制分支
    m_tester->updateActiveStatus(true);
    m_tester->setMode(NetifItemViewWidget::TITLE_VERTICAL);

    QPaintEvent event(m_tester->rect());
    m_tester->paintEvent(&event);

    EXPECT_TRUE(m_tester->m_isActive);
    EXPECT_EQ(m_tester->m_mode, NetifItemViewWidget::TITLE_VERTICAL);
}

TEST_F(UT_NetifItemViewWidget, test_paintEvent_03_afterDataUpdate)
{
    // 携带速率文案绘制：确认刷新后的文案被使用
    auto info = std::make_shared<NetifInfo>();
    info->set_recv_bps(1048576);
    info->set_sent_bps(2048);
    m_tester->updateData(info);

    QPaintEvent event(m_tester->rect());
    m_tester->paintEvent(&event);

    EXPECT_EQ(m_tester->m_recv_bps, formatUnit_net(1048576 * 8, B, 1, true));
    EXPECT_EQ(m_tester->m_sent_bps, formatUnit_net(2048 * 8, B, 1, true));
}
