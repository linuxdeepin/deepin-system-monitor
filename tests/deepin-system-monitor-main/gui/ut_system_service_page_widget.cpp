// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "system_service_page_widget.h"
#include "system_service_table_view.h"
#include "settings.h"
#include "main_window.h"
#include "toolbar.h"
#include "application.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QHBoxLayout>
#include <QPointer>
#include <QPainterPath>
#include <QPaintEvent>

/***************************************STUB begin*********************************************/
// 全局填充调用计数与捕获的填充矩形
static int g_fillPathCount = 0;
static QRectF g_fillPathRect;

// 按成员调用约定（this, args）捕获 QPainter::fillPath 入参
static void stub_QPainter_fillPath(QPainter *, const QPainterPath &path, const QBrush &)
{
    ++g_fillPathCount;
    g_fillPathRect = path.boundingRect();
}

// Settings::instance 返回空指针，隔离配置文件读取
static Settings *stub_Settings_instance_null()
{
    return nullptr;
}
/***************************************STUB end**********************************************/

class UT_SystemServicePageWidget : public ::testing::Test
{
public:
    UT_SystemServicePageWidget() : m_tester(nullptr) {}

    void SetUp() override
    {
        g_fillPathCount = 0;
        g_fillPathRect = QRectF();
        // 注入 MainWindow/Toolbar：SystemServiceTableView 构造时会连接
        // gApp->mainWindow()->toolbar() 的 search 信号（与既有测试套件一致）
        static QWidget wid;
        static MainWindow wm;
        static Toolbar toolbar;
        wm.m_toolbar = &toolbar;
        gApp->setMainWindow(&wm);
        // 捕获 paintEvent 的背景填充
        m_stub.set(ADDR(QPainter, fillPath), stub_QPainter_fillPath);
        m_tester = new SystemServicePageWidget(&wid);
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
    SystemServicePageWidget *m_tester;
};

// 用例计数声明表（方法 | level | factors | min | actual）
// | SystemServicePageWidget | low | - | 1 | 1 |
// | ~SystemServicePageWidget | low | - | 1 | 1 |
// | paintEvent | low | - | 1 | 2 |

// 构造函数创建服务表格子控件并设置 10 像素边距
TEST_F(UT_SystemServicePageWidget, Constructor_DefaultLayout_TableChildWithMargins)
{
    // Arrange
    const int kMargin = 10;
    // Act: 查询构造完成的布局与子控件
    QMargins margins = m_tester->layout()->contentsMargins();
    SystemServiceTableView *tv = m_tester->findChild<SystemServiceTableView *>("SvcTableView");
    // Assert
    EXPECT_NE(tv, nullptr);
    EXPECT_EQ(margins.left(), kMargin);
    EXPECT_EQ(margins.right(), kMargin);
}

// 析构后子控件一并销毁（QPointer 置空验证无悬挂）
TEST_F(UT_SystemServicePageWidget, Destructor_AfterDelete_ReleasesChildTableView)
{
    // Arrange
    SystemServiceTableView *tv = m_tester->findChild<SystemServiceTableView *>();
    EXPECT_NE(tv, nullptr);
    QPointer<SystemServiceTableView> guard(tv);
    QPointer<SystemServicePageWidget> widgetGuard(m_tester);
    EXPECT_FALSE(guard.isNull());
    EXPECT_NE(m_tester->layout(), nullptr);
    // Act
    delete m_tester;
    m_tester = nullptr;
    // Assert
    EXPECT_TRUE(guard.isNull());
    EXPECT_TRUE(widgetGuard.isNull());
}

// paintEvent 以整个部件矩形填充背景
TEST_F(UT_SystemServicePageWidget, PaintEvent_PaintBackground_FillsWidgetRect)
{
    // Arrange
    Stub settingsStub;
    settingsStub.set(ADDR(Settings, instance), stub_Settings_instance_null); // 隔离配置文件 IO
    m_tester->resize(300, 200);
    QPaintEvent ev(m_tester->rect());
    // Act
    m_tester->paintEvent(&ev);
    // Assert
    EXPECT_EQ(g_fillPathCount, 1);
    EXPECT_EQ(g_fillPathRect, QRectF(m_tester->rect()));
}

// paintEvent 在不同尺寸下依旧覆盖整个部件矩形
TEST_F(UT_SystemServicePageWidget, PaintEvent_DifferentSize_CoversWholeRect)
{
    // Arrange
    Stub settingsStub;
    settingsStub.set(ADDR(Settings, instance), stub_Settings_instance_null); // 隔离配置文件 IO
    m_tester->resize(120, 80);
    QPaintEvent ev(m_tester->rect());
    // Act
    m_tester->paintEvent(&ev);
    // Assert
    EXPECT_EQ(g_fillPathCount, 1);
    EXPECT_EQ(g_fillPathRect.width(), 120.0);
    EXPECT_EQ(g_fillPathRect.height(), 80.0);
}
