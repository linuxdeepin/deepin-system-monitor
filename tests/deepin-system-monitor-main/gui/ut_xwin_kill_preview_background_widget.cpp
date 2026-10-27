// Copyright (C) 2019-2026 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "xwin_kill_preview_background_widget.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QPainter>
#include <QRegion>
#include <QTimer>
#include <DStyle>
#include <DFontSizeManager>

DWIDGET_USE_NAMESPACE

/***************************************STUB begin*********************************************/
// QWidget::style 桩：paintEvent 中 dynamic_cast<DStyle*> 需要真实 DStyle 实例
static Dtk::Widget::DStyle *stub_paintEvent_dstyle()
{
    static Dtk::Widget::DStyle style;
    return &style;
}
/***************************************STUB end**********************************************/

// 分支清单（来源：实际源码 xwin_kill_preview_background_widget.cpp）
// XWinKillPreviewBackgroundWidget::XWinKillPreviewBackgroundWidget(background, parent)：
//   无分支：initUI() → initConnection()
// XWinKillPreviewBackgroundWidget::updateSelection(region)：
//   单一路径：坐标平移 (-geometry().x(), -geometry().y()) → 存储 → update()
// XWinKillPreviewBackgroundWidget::clearSelection()（inline）：
//   单一路径：m_selRegion = {} → update()
// XWinKillPreviewBackgroundWidget::paintEvent(e)：
//   Qt6 路径：绘制背景截图 → 红色选区遮罩 → DGuiApplicationHelper 调色板 → 帮助提示（圆角矩形+图标+文本）
// 用例映射：
//   - Ctor_InitializesWindowFlagsAndAttributes             → ctor
//   - Ctor_InitUITextMetricsAndIcon                        → initUI 效果
//   - UpdateSelection_NewRegion_TranslatesAndStores        → updateSelection
//   - UpdateSelection_EmptyRegion_StoresEmpty              → updateSelection 边界
//   - ClearSelection_AfterUpdate_ResetsRegion              → clearSelection
//   - ClearSelection_RepeatedCall_Safe                     → clearSelection 幂等
//   - PaintEvent_WithSelection_RendersComposite            → paintEvent 选区分支
//   - PaintEvent_IdleWithoutSelection_RendersTooltip       → paintEvent 空选区
//   - InitUI_SecondCall_KeepsAttributes                    → initUI 幂等
//   - InitConnection_SecondCall_NoCrash                    → initConnection

/* 用例计数声明（min 依据 test-code-gen §4：level 基数 + factor 加成）
| method | level | factors | min | actual |
| Ctor | mid | in_degree:3 | 2 | 2 |
| updateSelection | mid | in_degree:2 | 2 | 2 |
| clearSelection | low | - | 1 | 2 |
| paintEvent | low | - | 1 | 2 |
| initUI | low | - | 1 | 2 |
| initConnection | low | - | 1 | 1 |
*/

class UT_XWinKillPreviewBackgroundWidget : public ::testing::Test
{
public:
    UT_XWinKillPreviewBackgroundWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget wid;
        static QPixmap pixmap = QPixmap(100, 100);
        pixmap.fill(Qt::black);
        m_tester = new XWinKillPreviewBackgroundWidget(pixmap, &wid);
        m_tester->resize(300, 200);
        m_stub.set(ADDR(QWidget, style), stub_paintEvent_dstyle);
    }

    virtual void TearDown()
    {
        m_stub.reset(ADDR(QWidget, style));
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    XWinKillPreviewBackgroundWidget *m_tester;
    Stub m_stub;
};

TEST_F(UT_XWinKillPreviewBackgroundWidget, Ctor_WindowFlags_AttributesInitialized)
{
    // Arrange：构造已在 SetUp 完成
    EXPECT_NE(m_tester, nullptr);

    // Act：读取窗口标志与属性
    Qt::WindowFlags flags = m_tester->windowFlags();

    // Assert：置顶/无边框/绕过 WM 标志与透明背景、自动删除属性按源码设置
    EXPECT_TRUE(flags & Qt::WindowStaysOnTopHint);
    EXPECT_TRUE(flags & Qt::FramelessWindowHint);
    EXPECT_TRUE(flags & Qt::BypassWindowManagerHint);
    EXPECT_TRUE(m_tester->testAttribute(Qt::WA_TranslucentBackground));
    EXPECT_TRUE(m_tester->testAttribute(Qt::WA_DeleteOnClose));
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, Ctor_InitUI_TextMetricsIcon)
{
    // Arrange：构造已在 SetUp 完成
    EXPECT_NE(m_tester, nullptr);

    // Act：读取 initUI 产出的文本与图标状态
    // Assert：提示文案、字号、文本尺寸与图标按源码初始化
    EXPECT_EQ(m_tester->m_text, QApplication::translate("Process.Choose.Window.Dialog",
                                                         "Click the application you want to end"));
    EXPECT_FALSE(m_tester->m_text.isEmpty());
    EXPECT_EQ(m_tester->m_font.pixelSize(), DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T6));
    EXPECT_FALSE(m_tester->m_textSize.isEmpty());
    EXPECT_FALSE(m_tester->m_icon.isNull());
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, UpdateSelection_NewRegion_TranslatesAndStores)
{
    // Arrange：设定几何位置 (100,100)，目标区域在其局部坐标 (150,150)
    m_tester->setGeometry(100, 100, 800, 600);
    QRegion region(QRect(150, 150, 10, 10));

    // Act：更新选区
    m_tester->updateSelection(region);

    // Assert：选区被平移到控件局部坐标 (50,50)
    QRect bounds = m_tester->m_selRegion.boundingRect();
    EXPECT_EQ(bounds, QRect(50, 50, 10, 10));
    EXPECT_EQ(m_tester->m_selRegion, region.translated(-100, -100));
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, UpdateSelection_EmptyRegion_StoresEmpty)
{
    // Arrange：初始无选区
    ASSERT_TRUE(m_tester->m_selRegion.isEmpty());

    // Act：传入空区域
    m_tester->updateSelection(QRegion());

    // Assert：存储空选区
    EXPECT_EQ(m_tester->m_selRegion, QRegion());
    EXPECT_TRUE(m_tester->m_selRegion.isEmpty());
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, ClearSelection_AfterUpdate_ResetsRegion)
{
    // Arrange：先产生一个非空选区
    m_tester->updateSelection(QRegion(QRect(0, 0, 20, 20)));
    ASSERT_FALSE(m_tester->m_selRegion.isEmpty());

    // Act：清空选区
    m_tester->clearSelection();

    // Assert：选区复位为空
    EXPECT_EQ(m_tester->m_selRegion, QRegion());
    EXPECT_TRUE(m_tester->m_selRegion.isEmpty());
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, ClearSelection_RepeatedCall_Safe)
{
    // Arrange：连续清空一次
    m_tester->clearSelection();
    ASSERT_TRUE(m_tester->m_selRegion.isEmpty());

    // Act：再次清空
    m_tester->clearSelection();

    // Assert：仍为空且无副作用
    EXPECT_EQ(m_tester->m_selRegion, QRegion());
    EXPECT_NE(m_tester, nullptr);
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, PaintEvent_WithSelection_RendersComposite)
{
    // Arrange：产生选区后强制重绘
    m_tester->updateSelection(QRegion(QRect(10, 10, 50, 50)));

    // Act：抓取渲染结果
    QPixmap grabbed = m_tester->grab();

    // Assert：合成渲染非空且尺寸与控件一致
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, PaintEvent_IdleWithoutSelection_RendersTooltip)
{
    // Arrange：不产生任何选区
    ASSERT_TRUE(m_tester->m_selRegion.isEmpty());

    // Act：无选区直接渲染
    QPixmap grabbed = m_tester->grab();

    // Assert：帮助提示渲染成功且不改变选区状态
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
    EXPECT_TRUE(m_tester->m_selRegion.isEmpty());
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, InitUI_SecondCall_KeepsAttributes)
{
    // Arrange：记录首次 initUI 文案
    QString firstText = m_tester->m_text;
    ASSERT_FALSE(firstText.isEmpty());

    // Act：重复调用 initUI
    m_tester->initUI();

    // Assert：窗口属性与文案保持稳定（幂等）
    EXPECT_TRUE(m_tester->testAttribute(Qt::WA_TranslucentBackground));
    EXPECT_EQ(m_tester->m_text, firstText);
}

TEST_F(UT_XWinKillPreviewBackgroundWidget, InitConnection_SecondCall_NoCrash)
{
    // Arrange：连接初始化一次（构造期）
    ASSERT_NE(m_tester, nullptr);

    // Act：再次调用空实现
    m_tester->initConnection();

    // Assert：状态不受影响
    EXPECT_TRUE(m_tester->testAttribute(Qt::WA_TranslucentBackground));
    EXPECT_EQ(m_tester->m_text, QApplication::translate("Process.Choose.Window.Dialog",
                                                         "Click the application you want to end"));
}
