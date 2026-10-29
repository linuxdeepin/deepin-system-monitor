// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "block_dev_detail_view_widget.h"
#include "block_dev_stat_view_widget.h"
#include "block_dev_summary_view_widget.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QPointer>
#include <QFont>

/***************************************STUB begin*********************************************/
// 基类 detailFontChanged 调用计数
static int g_baseFontChangedCount = 0;
static void stub_BaseDetail_fontChanged(BaseDetailViewWidget *, const QFont &)
{
    ++g_baseFontChangedCount;
}

// BlockStatViewWidget::fontChanged 捕获转发的字体
static QFont g_statWidgetFont;
static bool g_statFontChangedCalled = false;
static void stub_stat_fontChanged(BlockStatViewWidget *, const QFont &font)
{
    g_statWidgetFont = font;
    g_statFontChangedCalled = true;
}

// BlockDevSummaryViewWidget::fontChanged 捕获转发的字体
static QFont g_summaryWidgetFont;
static bool g_summaryFontChangedCalled = false;
static void stub_summary_fontChanged(BlockDevSummaryViewWidget *, const QFont &font)
{
    g_summaryWidgetFont = font;
    g_summaryFontChangedCalled = true;
}
/***************************************STUB end**********************************************/

class UT_BlockDevDetailViewWidget : public ::testing::Test
{
public:
    UT_BlockDevDetailViewWidget() : m_tester(nullptr) {}

    void SetUp() override
    {
        g_baseFontChangedCount = 0;
        g_statFontChangedCalled = false;
        g_summaryFontChangedCalled = false;
        g_statWidgetFont = QFont();
        g_summaryWidgetFont = QFont();
        m_tester = new BlockDevDetailViewWidget();
    }

    void TearDown() override
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    Stub m_stub;
    BlockDevDetailViewWidget *m_tester;
};

// 用例计数声明表（方法 | level | factors | min | actual）
// | BlockDevDetailViewWidget | low | - | 1 | 1 |
// | ~BlockDevDetailViewWidget | low | - | 1 | 1 |
// | detailFontChanged | low | - | 1 | 4 |

// 构造函数设置对象名、标题并创建两个子视图
TEST_F(UT_BlockDevDetailViewWidget, Constructor_DefaultLayout_BuildsChildrenTitle)
{
    // Arrange
    const QString kObjectName = "BlockDevDetailViewWidget";
    const QString kTitle = "Disks";
    // Act: 查询构造完成的名称、标题与子视图
    QString objectName = m_tester->objectName();
    QString title = m_tester->title();
    BlockStatViewWidget *stat = m_tester->findChild<BlockStatViewWidget *>();
    BlockDevSummaryViewWidget *summary = m_tester->findChild<BlockDevSummaryViewWidget *>("BlocksummaryWidget");
    // Assert
    EXPECT_EQ(objectName, kObjectName);
    EXPECT_EQ(title, kTitle);
    EXPECT_NE(stat, nullptr);
    EXPECT_NE(summary, nullptr);
}

// 构造函数沿用基类中央布局边距（16,10,16,16）
TEST_F(UT_BlockDevDetailViewWidget, Constructor_DefaultLayout_MarginsFromBase)
{
    // Arrange
    const QMargins kMargins(16, 10, 16, 16);
    const int kSpacing = 16;
    // Act
    QMargins margins = m_tester->layout()->contentsMargins();
    int spacing = m_tester->layout()->spacing();
    // Assert
    EXPECT_EQ(margins, kMargins);
    EXPECT_EQ(spacing, kSpacing);
}

// 构造函数立即以应用字体初始化两级字号
TEST_F(UT_BlockDevDetailViewWidget, Constructor_DefaultFont_TwoLevelFontSizes)
{
    // Arrange
    QFont appFont = m_tester->font();
    // Act: 读取构造完成的两级字号
    float titlePts = m_tester->m_titleFont.pointSizeF();
    float contentPts = m_tester->m_contentFont.pointSizeF();
    // Assert: 标题字号 = 应用字号 + 12，内容字号 = 应用字号 - 1
    EXPECT_FLOAT_EQ(titlePts, appFont.pointSizeF() + 12.0);
    EXPECT_FLOAT_EQ(contentPts, appFont.pointSizeF() - 1.0);
    EXPECT_GE(titlePts, 0.0);
}

// detailFontChanged 转发字体到统计视图子控件
TEST_F(UT_BlockDevDetailViewWidget, DetailFontChanged_NewFont_ForwardsToStatWidget)
{
    // Arrange
    QFont font;
    font.setPointSize(9);
    m_stub.set(ADDR(BaseDetailViewWidget, detailFontChanged), stub_BaseDetail_fontChanged);
    m_stub.set(ADDR(BlockStatViewWidget, fontChanged), stub_stat_fontChanged);
    m_stub.set(ADDR(BlockDevSummaryViewWidget, fontChanged), stub_summary_fontChanged);
    // Act
    m_tester->detailFontChanged(font);
    // Assert
    EXPECT_TRUE(g_statFontChangedCalled);
    EXPECT_EQ(g_statWidgetFont, font);
    EXPECT_GE(g_baseFontChangedCount, 1);
}

// detailFontChanged 同步转发字体到摘要视图子控件
TEST_F(UT_BlockDevDetailViewWidget, DetailFontChanged_NewFont_ForwardsToSummaryWidget)
{
    // Arrange
    QFont font;
    font.setPointSize(11);
    m_stub.set(ADDR(BaseDetailViewWidget, detailFontChanged), stub_BaseDetail_fontChanged);
    m_stub.set(ADDR(BlockStatViewWidget, fontChanged), stub_stat_fontChanged);
    m_stub.set(ADDR(BlockDevSummaryViewWidget, fontChanged), stub_summary_fontChanged);
    // Act
    m_tester->detailFontChanged(font);
    // Assert
    EXPECT_TRUE(g_summaryFontChangedCalled);
    EXPECT_EQ(g_summaryWidgetFont, font);
    EXPECT_TRUE(g_statFontChangedCalled);
}

// detailFontChanged 沿用基类规则更新标题/内容两级字号（+12/-1）
TEST_F(UT_BlockDevDetailViewWidget, DetailFontChanged_NewFont_UpdatesBaseFonts)
{
    // Arrange
    QFont font;
    font.setPointSize(10);
    // Act
    m_tester->detailFontChanged(font);
    // Assert
    EXPECT_FLOAT_EQ(m_tester->m_titleFont.pointSizeF(), font.pointSizeF() + 12.0);
    EXPECT_FLOAT_EQ(m_tester->m_contentFont.pointSizeF(), font.pointSizeF() - 1.0);
    EXPECT_NE(m_tester->m_titleFont.pointSizeF(), m_tester->m_contentFont.pointSizeF());
}

// 析构后两个子视图一并销毁（QPointer 置空验证无悬挂）
TEST_F(UT_BlockDevDetailViewWidget, Destructor_AfterDelete_ReleasesChildWidgets)
{
    // Arrange
    BlockStatViewWidget *stat = m_tester->findChild<BlockStatViewWidget *>();
    BlockDevSummaryViewWidget *summary = m_tester->findChild<BlockDevSummaryViewWidget *>();
    EXPECT_NE(stat, nullptr);
    EXPECT_NE(summary, nullptr);
    QPointer<BlockStatViewWidget> statGuard(stat);
    QPointer<BlockDevSummaryViewWidget> summaryGuard(summary);
    EXPECT_EQ(m_tester->title(), QString("Disks"));
    // Act
    delete m_tester;
    m_tester = nullptr;
    // Assert
    EXPECT_TRUE(statGuard.isNull());
    EXPECT_TRUE(summaryGuard.isNull());
}
