// Copyright (C) 2026 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "onlineicon.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

// 分支清单（来源：实际源码 onlineicon.cpp）
// OnlineIcon::OnlineIcon(parent)：
//   无分支：创建阴影特效（黑色 alpha=16、偏移 (0,2)）并挂到控件
// OnlineIcon::setColor(color)：
//   单一路径：m_color = color
// OnlineIcon::paintEvent(event)：
//   单一路径：抗锯齿 → m_color 填充内切圆 → 白色 1px 描边 → 重挂阴影特效
// 用例映射：
//   - Ctor_InstallsShadowEffect                            → ctor
//   - SetColor_NewColor_StoresColor                        → setColor
//   - PaintEvent_DefaultGreen_RendersCircle                → paintEvent（默认绿）
//   - PaintEvent_AfterSetColor_RendersNewColor             → paintEvent（新颜色）

/* 用例计数声明（min 依据 test-code-gen §4：level 基数 + factor 加成）
| method | level | factors | min | actual |
| Ctor | mid | in_degree:2 | 2 | 2 |
| setColor | low | - | 1 | 1 |
| paintEvent | low | - | 1 | 2 |
*/

class UT_OnlineIcon : public ::testing::Test
{
public:
    UT_OnlineIcon() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new OnlineIcon();
        m_tester->resize(16, 16);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    OnlineIcon *m_tester;
};

TEST_F(UT_OnlineIcon, Ctor_InstallsDrop_ShadowEffect)
{
    // Arrange：构造已在 SetUp 完成
    EXPECT_NE(m_tester, nullptr);

    // Act：读取构造后的特效状态
    // Assert：阴影特效创建并挂载，颜色/偏移按源码初始化
    auto *effect = m_tester->graphicsEffect();
    EXPECT_NE(effect, nullptr);
    auto *shadow = dynamic_cast<QGraphicsDropShadowEffect *>(effect);
    ASSERT_NE(shadow, nullptr);
    EXPECT_EQ(shadow->color(), QColor(0, 0, 0, 16));
    EXPECT_EQ(shadow->offset(), QPointF(0, 2));
}

TEST_F(UT_OnlineIcon, Ctor_DefaultColor_IsGreen)
{
    // Arrange：构造已在 SetUp 完成
    EXPECT_NE(m_tester, nullptr);

    // Act：读取默认颜色
    // Assert：默认 m_color 为绿色（Qt::green）
    EXPECT_EQ(m_tester->m_color, QColor(Qt::green));
    EXPECT_NE(m_tester->m_shadowEffect, nullptr);
}

TEST_F(UT_OnlineIcon, SetColor_NewColor_StoresColor)
{
    // Arrange：默认颜色为绿色
    EXPECT_EQ(m_tester->m_color, QColor(Qt::green));

    // Act：设置新颜色
    m_tester->setColor(QColor(255, 0, 0));

    // Assert：颜色被存储且不影响特效
    EXPECT_EQ(m_tester->m_color, QColor(255, 0, 0));
    EXPECT_NE(m_tester->graphicsEffect(), nullptr);
}

TEST_F(UT_OnlineIcon, PaintEvent_DefaultGreen_RendersCircle)
{
    // Arrange：保持默认绿色
    EXPECT_EQ(m_tester->m_color, QColor(Qt::green));

    // Act：默认颜色下强制重绘
    QPixmap grabbed = m_tester->grab();

    // Assert：渲染非空且尺寸一致，颜色未被改动
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
    EXPECT_EQ(m_tester->m_color, QColor(Qt::green));
}

TEST_F(UT_OnlineIcon, PaintEvent_AfterSetColor_RendersNewColor)
{
    // Arrange：切换颜色
    m_tester->setColor(QColor(0, 0, 255));

    // Act：强制重绘
    QPixmap grabbed = m_tester->grab();

    // Assert：渲染成功且新颜色保留
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(m_tester->m_color, QColor(0, 0, 255));
    EXPECT_EQ(grabbed.size(), m_tester->size());
}
