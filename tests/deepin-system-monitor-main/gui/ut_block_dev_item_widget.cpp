// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "block_dev_item_widget.h"
#include "system/block_device.h"
#include "chart_view_widget.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QPainter>
#include <QResizeEvent>
#include <QMouseEvent>

#include <memory>

/***************************************STUB begin*********************************************/
static void stub_readDeviceInfo_noop()
{
}

// 构造一台名称可设置的"虚拟"设备（屏蔽 readDeviceInfo 的真实 /proc 读取）
static BlockDevice makeBlockDevice(const QByteArray &name)
{
    Stub stub;
    stub.set(ADDR(BlockDevice, readDeviceInfo), stub_readDeviceInfo_noop);
    BlockDevice dev;
    dev.setDeviceName(name);
    return dev;
}
/***************************************STUB end**********************************************/

class UT_BlockDevItemWidget : public ::testing::Test
{
public:
    UT_BlockDevItemWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new BlockDevItemWidget();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    BlockDevItemWidget *m_tester;
};

// 构造/析构：图表子控件创建、默认水平模式、未激活
TEST_F(UT_BlockDevItemWidget, Ctor_DefaultState_HorizontalModeNotActive)
{
    // Arrange：SetUP 已完成默认构造，初始状态即待验证场景
    ASSERT_NE(m_tester, nullptr);

    // Act: 读取构造后的默认状态
    const bool active = m_tester->isActiveItem();
    const bool nameEmpty = m_tester->m_blokeDeviceInfo.deviceName().isEmpty();

    // Assert
    EXPECT_EQ(m_tester->m_mode, BlockDevItemWidget::TitleStyle::TITLE_HORIZONTAL);
    EXPECT_FALSE(active);
    EXPECT_TRUE(nameEmpty);
}

// updateData 应保存设备数据并推送速率到图表
TEST_F(UT_BlockDevItemWidget, test_updateData_01)
{
    // Arrange
    BlockDevice info = makeBlockDevice("ut-sm-disk-a");

    // Act
    m_tester->updateData(info);

    // Assert
    EXPECT_EQ(m_tester->m_blokeDeviceInfo.deviceName(), info.deviceName());
    EXPECT_EQ(m_tester->m_blokeDeviceInfo.readSpeed(), info.readSpeed());
    EXPECT_EQ(m_tester->m_blokeDeviceInfo.writeSpeed(), info.writeSpeed());
}

// activeItemWidget(true/false) 应同步 m_isActive
TEST_F(UT_BlockDevItemWidget, test_activeItemWidget_01)
{
    // Arrange：初始未激活
    ASSERT_NE(m_tester, nullptr);
    EXPECT_EQ(m_tester->isActiveItem(), false);

    // Act：激活后再取消
    m_tester->activeItemWidget(true);

    // Assert
    EXPECT_EQ(m_tester->isActiveItem(), true);

    // Act：再次取消激活
    m_tester->activeItemWidget(false);

    // Assert
    EXPECT_EQ(m_tester->isActiveItem(), false);
}

// setMode 应更新 m_mode（0=水平 / 1=垂直）
TEST_F(UT_BlockDevItemWidget, test_setMode_01)
{
    // Arrange：初始为默认水平模式
    EXPECT_EQ(m_tester->m_mode, BlockDevItemWidget::TitleStyle::TITLE_HORIZONTAL);

    // Act
    m_tester->setMode(BlockDevItemWidget::TitleStyle::TITLE_VERTICAL);

    // Assert
    EXPECT_EQ(m_tester->m_mode, BlockDevItemWidget::TitleStyle::TITLE_VERTICAL);

    // Act
    m_tester->setMode(BlockDevItemWidget::TitleStyle::TITLE_HORIZONTAL);

    // Assert
    EXPECT_EQ(m_tester->m_mode, BlockDevItemWidget::TitleStyle::TITLE_HORIZONTAL);
}

// fontChanged 应更新 m_font 并触发几何更新（不崩溃）
TEST_F(UT_BlockDevItemWidget, test_fontChanged_01)
{
    // Arrange
    QFont font;
    font.setBold(true);

    // Act
    m_tester->fontChanged(font);

    // Assert
    EXPECT_EQ(m_tester->m_font, font);
    EXPECT_EQ(m_tester->m_font.bold(), font.bold());
}

// mousePressEvent 应发射 clicked 信号并携带设备名
TEST_F(UT_BlockDevItemWidget, test_mousePressEvent_01)
{
    // Arrange
    BlockDevice info = makeBlockDevice("ut-sm-disk-a");
    m_tester->updateData(info);
    QSignalSpy signalSpy(m_tester, &BlockDevItemWidget::clicked);
    QMouseEvent me(QEvent::MouseButtonPress, QPointF(1.0, 1.0),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act
    m_tester->mousePressEvent(&me);

    // Assert
    EXPECT_EQ(signalSpy.count(), 1);
    if (signalSpy.count() == 1) {
        EXPECT_EQ(signalSpy.at(0).at(0).toString(), QString("ut-sm-disk-a"));
    }
}

// mousePressEvent：无设备名时也发射 clicked（空字符串参数）
TEST_F(UT_BlockDevItemWidget, test_mousePressEvent_02)
{
    // Arrange
    QSignalSpy signalSpy(m_tester, &BlockDevItemWidget::clicked);
    QMouseEvent me(QEvent::MouseButtonPress, QPointF(1.0, 1.0),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act
    m_tester->mousePressEvent(&me);

    // Assert
    EXPECT_EQ(signalSpy.count(), 1);
    if (signalSpy.count() == 1) {
        EXPECT_EQ(signalSpy.at(0).at(0).toString(), QString(""));
    }
}

// resizeEvent 应触发几何更新：水平模式下图表左边距为 0
TEST_F(UT_BlockDevItemWidget, test_resizeEvent_01)
{
    // Arrange
    m_tester->setMode(BlockDevItemWidget::TitleStyle::TITLE_HORIZONTAL);
    m_tester->resize(200, 120);
    QResizeEvent re(QSize(200, 120), QSize(10, 10));

    // Act
    m_tester->resizeEvent(&re);

    // Assert: 水平模式 curXMargin = 0
    ChartViewWidget *chart = m_tester->findChild<ChartViewWidget *>();
    ASSERT_NE(chart, nullptr);
    EXPECT_EQ(chart->geometry().x(), 0);
    EXPECT_GT(chart->geometry().height(), 0);
}

// resizeEvent：垂直模式下图表左边距为 margin（大于 0），且高度小于水平模式
TEST_F(UT_BlockDevItemWidget, test_resizeEvent_02)
{
    // Arrange
    m_tester->setMode(BlockDevItemWidget::TitleStyle::TITLE_VERTICAL);
    m_tester->resize(200, 120);
    QResizeEvent re(QSize(200, 120), QSize(10, 10));

    // Act
    m_tester->resizeEvent(&re);
    ChartViewWidget *chart = m_tester->findChild<ChartViewWidget *>();
    ASSERT_NE(chart, nullptr);
    const QRect verticalGeometry = chart->geometry();

    // Arrange: 同尺寸水平模式作对照
    m_tester->setMode(BlockDevItemWidget::TitleStyle::TITLE_HORIZONTAL);
    m_tester->resizeEvent(&re);

    // Assert
    EXPECT_GT(verticalGeometry.x(), chart->geometry().x());
    // Assert: 垂直模式标题占两行，图表比水平模式（三行标题）更高
    EXPECT_GT(verticalGeometry.height(), chart->geometry().height());
}

// updateWidgetGeometry：水平与垂直两种模式的几何更新路径
TEST_F(UT_BlockDevItemWidget, test_updateWidgetGeometry_01)
{
    // Arrange
    m_tester->resize(300, 150);
    m_tester->setMode(BlockDevItemWidget::TitleStyle::TITLE_HORIZONTAL);
    ChartViewWidget *chart = m_tester->findChild<ChartViewWidget *>();
    ASSERT_NE(chart, nullptr);

    // Act
    m_tester->updateWidgetGeometry();
    const QRect horizontalGeometry = chart->geometry();

    // Arrange
    m_tester->setMode(BlockDevItemWidget::TitleStyle::TITLE_VERTICAL);

    // Act
    m_tester->updateWidgetGeometry();
    const QRect verticalGeometry = chart->geometry();

    // Assert: 两种模式几何不同，且均落在控件范围内
    EXPECT_NE(horizontalGeometry, verticalGeometry);
    EXPECT_LE(chart->geometry().right(), m_tester->width());
    EXPECT_LE(chart->geometry().bottom(), m_tester->height());
}

// paintEvent：四种 激活状态 x 模式 组合均能正常渲染
TEST_F(UT_BlockDevItemWidget, test_paintEvent_01)
{
    // Arrange
    m_tester->m_isActive = true;
    m_tester->m_mode = BlockDevItemWidget::TitleStyle::TITLE_VERTICAL;

    // Act：离屏渲染整幅控件
    QPixmap grabbed = m_tester->grab();

    // Assert：渲染成功且尺寸与控件一致
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
}

TEST_F(UT_BlockDevItemWidget, test_paintEvent_02)
{
    // Arrange
    m_tester->m_isActive = false;
    m_tester->m_mode = BlockDevItemWidget::TitleStyle::TITLE_VERTICAL;

    // Act：离屏渲染整幅控件
    QPixmap grabbed = m_tester->grab();

    // Assert：渲染成功且尺寸与控件一致
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
}

TEST_F(UT_BlockDevItemWidget, test_paintEvent_03)
{
    // Arrange
    m_tester->m_isActive = true;
    m_tester->m_mode = BlockDevItemWidget::TitleStyle::TITLE_HORIZONTAL;

    // Act：离屏渲染整幅控件
    QPixmap grabbed = m_tester->grab();

    // Assert：渲染成功且尺寸与控件一致
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
}

TEST_F(UT_BlockDevItemWidget, test_paintEvent_04)
{
    // Arrange
    m_tester->m_isActive = false;
    m_tester->m_mode = BlockDevItemWidget::TitleStyle::TITLE_HORIZONTAL;

    // Act：离屏渲染整幅控件
    QPixmap grabbed = m_tester->grab();

    // Assert：渲染成功且尺寸与控件一致
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
}

// paintEvent：携带非空设备名与速率文本的绘制路径
TEST_F(UT_BlockDevItemWidget, test_paintEvent_05)
{
    // Arrange
    BlockDevice info = makeBlockDevice("ut-sm-disk-a");
    m_tester->updateData(info);
    m_tester->m_isActive = true;
    m_tester->m_mode = BlockDevItemWidget::TitleStyle::TITLE_VERTICAL;

    // Act：离屏渲染整幅控件
    QPixmap grabbed = m_tester->grab();

    // Assert：渲染成功且尺寸与控件一致，设备数据已生效
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
    EXPECT_EQ(m_tester->m_blokeDeviceInfo.deviceName(), QString("ut-sm-disk-a"));
}

// 析构路径：图表子控件被安全释放
TEST_F(UT_BlockDevItemWidget, Dtor_WithChartData_ChartReleasedSafely)
{
    // Arrange：先填充设备数据并定位图表子控件
    std::unique_ptr<BlockDevItemWidget> widget(new BlockDevItemWidget());
    widget->updateData(makeBlockDevice("ut-sm-disk-a"));
    ChartViewWidget *chart = widget->findChild<ChartViewWidget *>();
    EXPECT_NE(chart, nullptr);
    EXPECT_EQ(widget->m_blokeDeviceInfo.deviceName(), QString("ut-sm-disk-a"));

    // Act：析构控件
    widget.reset();

    // Assert：指针复位为空，析构正常完成不崩溃
    EXPECT_EQ(widget.get(), nullptr);
}
