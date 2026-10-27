// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "priority_slider.h"

//gtest
#include <gtest/gtest.h>

//Qt
#include <QSignalSpy>
#include <QResizeEvent>

class UT_PrioritySlider : public ::testing::Test
{
public:
    UT_PrioritySlider() : m_tester(nullptr) {}

    void SetUp() override
    {
        m_tester = nullptr;
    }

    void TearDown() override
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    PrioritySlider *m_tester;
};

// 用例计数声明表（方法 | level | factors | min | actual）
// | PrioritySlider | mid | in_degree:7 | 2 | 2 |
// | resizeEvent | low | - | 1 | 2 |

// 默认构造为水平方向滑块
TEST_F(UT_PrioritySlider, Constructor_DefaultOrientation_IsHorizontal)
{
    // Arrange
    m_tester = new PrioritySlider();
    // Act: 查询构造完成的方向与初值
    Qt::Orientation orientation = m_tester->orientation();
    int value = m_tester->value();
    // Assert
    EXPECT_EQ(orientation, Qt::Horizontal);
    EXPECT_EQ(value, 0);
    EXPECT_EQ(m_tester->receivers(SIGNAL(sizeChanged(QSize))), 0);
}

// 显式构造为垂直方向滑块
TEST_F(UT_PrioritySlider, Constructor_VerticalOrientation_IsVertical)
{
    // Arrange
    m_tester = new PrioritySlider(Qt::Vertical);
    // Act: 查询构造完成的方向
    Qt::Orientation orientation = m_tester->orientation();
    // Assert
    EXPECT_EQ(orientation, Qt::Vertical);
    EXPECT_NE(orientation, Qt::Horizontal);
    EXPECT_NE(m_tester, nullptr);
}

// 尺寸变化发出 sizeChanged 信号并携带新尺寸
TEST_F(UT_PrioritySlider, ResizeEvent_SizeChanged_EmitsSignalWithNewSize)
{
    // Arrange
    m_tester = new PrioritySlider();
    QSignalSpy spy(m_tester, &PrioritySlider::sizeChanged);
    // Act: 先显示（创建原生窗口），清掉 show 事件，再改变尺寸
    m_tester->show();
    spy.clear();
    m_tester->resize(200, 60);
    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.first().at(0).toSize(), QSize(200, 60));
}

// 直接调用 resizeEvent 发出携带事件尺寸的 sizeChanged 信号
// 注：Qt6 中 QWidget::resizeEvent 为空实现，向基类的转发无可观察副作用，
// 故通过 sizeChanged 信号携带的事件尺寸验证覆写行为
TEST_F(UT_PrioritySlider, ResizeEvent_DirectCall_EmitsSignalWithEventSize)
{
    // Arrange
    m_tester = new PrioritySlider();
    QSignalSpy spy(m_tester, &PrioritySlider::sizeChanged);
    QResizeEvent ev(QSize(150, 40), QSize(100, 30));
    // Act
    m_tester->resizeEvent(&ev);
    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.first().at(0).toSize(), QSize(150, 40));
}
