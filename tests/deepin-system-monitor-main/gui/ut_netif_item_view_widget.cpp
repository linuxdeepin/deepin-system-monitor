// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
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
#include <QImage>

using namespace core::system;
using namespace common::format;

/***************************************STUB begin*********************************************/
// 打桩说明：本类依赖 ChartViewWidget(纯内存绘制)与 formatUnit_net(纯格式化函数)，
// 均为无外部 I/O 的实现，直接使用真实实现；NetifInfo 为纯数据对象，走真实对象。
/***************************************STUB end**********************************************/

namespace {
// 统计非纯白像素数：paint 类用例用它证明确实绘制了内容
int nonWhitePixels(const QImage &image)
{
    int drawn = 0;
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (image.pixelColor(x, y) != Qt::white)
                ++drawn;
    return drawn;
}
}  // namespace

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

    // 离屏渲染当前控件到白色画布
    QImage renderWidget()
    {
        QImage image(m_tester->size(), QImage::Format_ARGB32);
        image.fill(Qt::white);
        m_tester->render(&image);
        return image;
    }

protected:
    NetifItemViewWidget *m_tester;
};

// 说明：清单中 netif_item_view_widget.cpp 的 QWidget(parent) 自由函数节点
// 对应 Qt 基类构造，非本项目可测代码，其行为由下方构造函数用例覆盖。
TEST_F(UT_NetifItemViewWidget, Ctor_DefaultState_MacBoundAndChartCreated)
{
    // Arrange：构造参数 mac="testmac"（SetUp 完成）

    // Act：默认构造（SetUp 完成）
    ASSERT_NE(m_tester, nullptr);

    // Assert：MAC 绑定 + 图表子控件创建 + 默认横向标题模式
    EXPECT_EQ(m_tester->m_mac, QByteArray("testmac"));
    EXPECT_NE(m_tester->m_ChartWidget, nullptr);
    EXPECT_EQ(m_tester->m_ChartWidget->parent(), m_tester);
    EXPECT_EQ(m_tester->m_mode, NetifItemViewWidget::TITLE_HORIZONTAL);
    EXPECT_EQ(m_tester->m_isActive, false);
    EXPECT_EQ(m_tester->m_ifname, QString("Ethernet"));
    EXPECT_TRUE(m_tester->m_recv_bps.isEmpty());
    EXPECT_TRUE(m_tester->m_sent_bps.isEmpty());
}

TEST_F(UT_NetifItemViewWidget, test_updateData_01)
{
    // Arrange：正常速率数据
    auto info = std::make_shared<NetifInfo>();
    info->set_recv_bps(1048576);
    info->set_sent_bps(2048);

    // Act：刷新数据
    m_tester->updateData(info);

    // Assert：速率写入图表 + 速率文案按 formatUnit_net 转换
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
    // Arrange：两组速率数据
    auto info1 = std::make_shared<NetifInfo>();
    info1->set_recv_bps(1024);
    info1->set_sent_bps(512);
    auto info2 = std::make_shared<NetifInfo>();
    info2->set_recv_bps(2048);
    info2->set_sent_bps(1024);

    // Act：连续刷新两次
    m_tester->updateData(info1);
    m_tester->updateData(info2);

    // Assert：图表数据按次序追加
    EXPECT_EQ(m_tester->m_ChartWidget->m_listData1.size(), 2);
    EXPECT_DOUBLE_EQ(m_tester->m_ChartWidget->m_listData1.first().toDouble(), 1024.0);
    EXPECT_DOUBLE_EQ(m_tester->m_ChartWidget->m_listData1.last().toDouble(), 2048.0);
    EXPECT_EQ(m_tester->m_recv_bps, formatUnit_net(2048 * 8, B, 1, true));
}

TEST_F(UT_NetifItemViewWidget, test_updateActiveStatus_01)
{
    // Arrange：初始非激活 + 离屏渲染基线
    ASSERT_EQ(m_tester->m_isActive, false);
    const QImage inactiveImage = renderWidget();

    // Act：切换为激活后渲染，再切回非激活后渲染
    m_tester->updateActiveStatus(true);
    const QImage activeImage = renderWidget();
    const bool activeState = m_tester->m_isActive;
    m_tester->updateActiveStatus(false);
    const QImage inactiveAgain = renderWidget();

    // Assert：激活状态切换生效；高亮底色改变绘制产物；切回后与基线一致
    EXPECT_EQ(activeState, true);
    EXPECT_FALSE(activeImage == inactiveImage);
    EXPECT_TRUE(inactiveAgain == inactiveImage);
}

TEST_F(UT_NetifItemViewWidget, test_setMode_01)
{
    // Arrange：初始横向模式
    ASSERT_EQ(m_tester->m_mode, NetifItemViewWidget::TITLE_HORIZONTAL);

    // Act：切换到纵向再切回横向
    m_tester->setMode(NetifItemViewWidget::TITLE_VERTICAL);
    const int afterVertical = m_tester->m_mode;
    m_tester->setMode(NetifItemViewWidget::TITLE_HORIZONTAL);

    // Assert：模式切换生效
    EXPECT_EQ(afterVertical, NetifItemViewWidget::TITLE_VERTICAL);
    EXPECT_EQ(m_tester->m_mode, NetifItemViewWidget::TITLE_HORIZONTAL);
}

TEST_F(UT_NetifItemViewWidget, test_fontChanged_01)
{
    // Arrange：目标字体
    QFont font;
    font.setBold(true);
    font.setPointSize(16);

    // Act：字体更新回调
    m_tester->fontChanged(font);

    // Assert：保存字体并同步到控件
    EXPECT_EQ(m_tester->m_font, font);
    EXPECT_EQ(m_tester->font(), font);
    EXPECT_EQ(m_tester->m_font.pointSize(), 16);
}

TEST_F(UT_NetifItemViewWidget, test_fontChanged_02_geometryRecomputed)
{
    // Arrange：纵向模式 + 目标字体
    m_tester->setMode(NetifItemViewWidget::TITLE_VERTICAL);
    QFont font;
    font.setPointSize(20);

    // Act：字体变化回调
    m_tester->fontChanged(font);

    // Assert：字体保存；图表几何按字体高度重新布局
    // 纵向模式: setGeometry(margin, fontHeight*2 + TextSpacing, width-2*margin, height - fontHeight*2 - TextSpacing - margin)
    EXPECT_EQ(m_tester->m_font, font);
    int fontHeight = QFontMetrics(font).height();
    EXPECT_EQ(m_tester->m_ChartWidget->geometry(),
              QRect(6, fontHeight * 2 + 12, 200 - 2 * 6, 150 - fontHeight * 2 - 12 - 6));
}

// 分支说明(updateWidgetGeometry): TITLE_HORIZONTAL->图表铺满去标题区 / else->按字体高度留标题区
TEST_F(UT_NetifItemViewWidget, test_updateWidgetGeometry_01_horizontal)
{
    // Arrange：默认横向模式（SetUp 完成）

    // Act：重算几何
    m_tester->updateWidgetGeometry();

    // Assert：图表从 TextSpacing(12) 起，宽度铺满，高度去掉标题区与底部边距
    const QRect geom = m_tester->m_ChartWidget->geometry();
    EXPECT_EQ(geom.x(), 0);
    EXPECT_EQ(geom.y(), 12);
    EXPECT_EQ(geom.width(), 200);
    EXPECT_EQ(geom.height(), 150 - 12 - 6);
}

TEST_F(UT_NetifItemViewWidget, test_updateWidgetGeometry_02_vertical)
{
    // Arrange：纵向模式 + 12pt 字体
    m_tester->setMode(NetifItemViewWidget::TITLE_VERTICAL);
    QFont font;
    font.setPointSize(12);
    m_tester->m_font = font;

    // Act：重算几何
    m_tester->updateWidgetGeometry();

    // Assert：x 偏移 margin(6)，y 偏移 2*字体高 + TextSpacing(12)
    int fontHeight = QFontMetrics(font).height();
    const QRect geom = m_tester->m_ChartWidget->geometry();
    EXPECT_EQ(geom.x(), 6);
    EXPECT_EQ(geom.y(), fontHeight * 2 + 12);
    EXPECT_EQ(geom.width(), 200 - 12);
    EXPECT_EQ(geom.height(), 150 - fontHeight * 2 - 18);
}

TEST_F(UT_NetifItemViewWidget, test_resizeEvent_01)
{
    // Arrange：尺寸从 300x200 变为 200x150
    QResizeEvent event(QSize(200, 150), QSize(300, 200));

    // Act：触发尺寸变化回调
    m_tester->resizeEvent(&event);

    // Assert：图表几何随视图重算（横向模式规则）
    const QRect geom = m_tester->m_ChartWidget->geometry();
    EXPECT_EQ(geom.x(), 0);
    EXPECT_EQ(geom.y(), 12);
    EXPECT_EQ(geom.width(), 200);
    EXPECT_EQ(geom.height(), 150 - 18);
}

TEST_F(UT_NetifItemViewWidget, test_mousePressEvent_01)
{
    // Arrange：监听 clicked 信号
    QSignalSpy spy(m_tester, &NetifItemViewWidget::clicked);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(10, 10), QPointF(10, 10),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act：鼠标左键点击
    m_tester->mousePressEvent(&press);

    // Assert：发出 clicked 信号并携带本条目 MAC
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString("testmac"));
}

// 分支说明(paintEvent): m_isActive 高亮->画选中底色+高亮描边 / 非激活->TextTips 描边;
//                      TITLE_HORIZONTAL 横排文案 / TITLE_VERTICAL 纵排文案
TEST_F(UT_NetifItemViewWidget, test_paintEvent_01_inactiveHorizontal)
{
    // Arrange：非激活 + 横向模式（SetUp 完成）

    // Act：离屏渲染（paintEvent 经完整绘制链执行）
    const QImage image = renderWidget();

    // Assert：绘制产物非空；非激活状态未改变
    EXPECT_GT(nonWhitePixels(image), 0);
    EXPECT_EQ(m_tester->m_isActive, false);
}

TEST_F(UT_NetifItemViewWidget, test_paintEvent_02_activeVertical)
{
    // Arrange：非激活基线渲染
    const QImage inactiveImage = renderWidget();

    // Act：激活 + 纵向模式后渲染（走高亮绘制分支）
    m_tester->updateActiveStatus(true);
    m_tester->setMode(NetifItemViewWidget::TITLE_VERTICAL);
    const QImage activeImage = renderWidget();

    // Assert：高亮分支产物与基线不同；状态生效
    EXPECT_GT(nonWhitePixels(activeImage), 0);
    EXPECT_FALSE(activeImage == inactiveImage);
    EXPECT_EQ(m_tester->m_mode, NetifItemViewWidget::TITLE_VERTICAL);
}

TEST_F(UT_NetifItemViewWidget, test_paintEvent_03_afterDataUpdate)
{
    // Arrange：携带速率文案（updateData 后）
    auto info = std::make_shared<NetifInfo>();
    info->set_recv_bps(1048576);
    info->set_sent_bps(2048);
    m_tester->updateData(info);

    // Act：离屏渲染
    const QImage image = renderWidget();

    // Assert：刷新后的文案被用于绘制，产物非空
    EXPECT_GT(nonWhitePixels(image), 0);
    EXPECT_EQ(m_tester->m_recv_bps, formatUnit_net(1048576 * 8, B, 1, true));
    EXPECT_EQ(m_tester->m_sent_bps, formatUnit_net(2048 * 8, B, 1, true));
}
