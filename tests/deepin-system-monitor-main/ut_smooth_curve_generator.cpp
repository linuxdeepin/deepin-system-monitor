// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "smooth_curve_generator.h"

//gtest
#include <gtest/gtest.h>

//qt
#include <QPainterPath>
#include <QPointF>
#include <QList>
#include <QtGlobal>
#include <cmath>

// SmoothCurveGenerator 为纯静态算法类，无外部依赖（DBus/文件/网络），
// 无需打桩，直接构造受控数据点验证曲线生成数学性质。
//
// 分支清单（源自 smooth_curve_generator.cpp）:
// 1. generateSmoothCurve: len<2 -> 返回空路径 | len>=2 -> moveTo + (len-1) 段 cubicTo
// 2. calculateControlPoints: n==1 -> 直线特例 (P1=(2P0+P3)/3, P2=2P1-P0) |
//    n>=2 -> 通用分支（rhs 填充、首/次控制点、末位特殊取值 (knots[n]+xs[n-1])/2）
// 3. calculateFirstControlPoints: b=4.0（i<n-1）| b=3.5（i==n-1 边界）；
//    前代 + 回代求解三对角系统

class SmoothCurveGeneratorTest : public ::testing::Test
{
    virtual void SetUp() override {}
    virtual void TearDown() override {}
};

// ───────────────────────── generateSmoothCurve ─────────────────────────
TEST_F(SmoothCurveGeneratorTest, GenerateSmoothCurve_WithEmptyList_ReturnsEmptyPath)
{
    // Arrange: 空数据点
    QList<QPointF> points;

    // Act
    QPainterPath path = SmoothCurveGenerator::generateSmoothCurve(points);

    // Assert: len<2 分支直接返回空路径
    EXPECT_TRUE(path.isEmpty());
    EXPECT_EQ(path.elementCount(), 0);
}

TEST_F(SmoothCurveGeneratorTest, GenerateSmoothCurve_WithSinglePoint_ReturnsEmptyPath)
{
    // Arrange: 单点（边界值，len-1=0 段曲线）
    QList<QPointF> points {QPointF(0, 0)};

    // Act
    QPainterPath path = SmoothCurveGenerator::generateSmoothCurve(points);

    // Assert
    EXPECT_TRUE(path.isEmpty());
    EXPECT_EQ(path.elementCount(), 0);
}

TEST_F(SmoothCurveGeneratorTest, GenerateSmoothCurve_WithTwoPoints_GeneratesOneCubicSegment)
{
    // Arrange: 两点 -> moveTo + 1 段 cubicTo（3 元素）
    QList<QPointF> points {QPointF(0, 0), QPointF(10, 10)};

    // Act
    QPainterPath path = SmoothCurveGenerator::generateSmoothCurve(points);

    // Assert: 元素数 = 1 + 3，且 moveTo 落在首点
    EXPECT_EQ(path.elementCount(), 4);
    EXPECT_DOUBLE_EQ(path.elementAt(0).x, 0.0);
}

TEST_F(SmoothCurveGeneratorTest, GenerateSmoothCurve_WithFivePoints_GeneratesTenElements)
{
    // Arrange: 5 点 -> moveTo + 4 段 cubicTo
    QList<QPointF> points {
        QPointF(0, 0), QPointF(10, 2), QPointF(20, 1), QPointF(30, 5), QPointF(40, 3)
    };

    // Act
    QPainterPath path = SmoothCurveGenerator::generateSmoothCurve(points);

    // Assert: 元素数 = 1 + 3*4 = 13
    EXPECT_EQ(path.elementCount(), 13);
    // Assert: 起点 moveTo 至第一个数据点
    EXPECT_DOUBLE_EQ(path.elementAt(0).x, 0.0);
    EXPECT_DOUBLE_EQ(path.elementAt(0).y, 0.0);
}

TEST_F(SmoothCurveGeneratorTest, GenerateSmoothCurve_WithMonotonicPoints_KeepsStartAndEndKnots)
{
    // Arrange: 单调数据点
    QList<QPointF> points {
        QPointF(0, 100), QPointF(25, 80), QPointF(50, 60), QPointF(75, 40), QPointF(100, 20)
    };

    // Act
    QPainterPath path = SmoothCurveGenerator::generateSmoothCurve(points);

    // Assert: 贝塞尔曲线严格经过首尾数据点
    QPointF start = path.pointAtPercent(0.0);
    EXPECT_DOUBLE_EQ(start.x(), 0.0);
    EXPECT_DOUBLE_EQ(start.y(), 100.0);
    QPointF end = path.pointAtPercent(1.0);
    EXPECT_DOUBLE_EQ(end.x(), 100.0);
    EXPECT_DOUBLE_EQ(end.y(), 20.0);
}

// ───────────────────────── calculateControlPoints ─────────────────────────
TEST_F(SmoothCurveGeneratorTest, CalculateControlPoints_WithTwoKnots_ProducesStraightLineControlPoints)
{
    // Arrange: n=1 直线特例：P1=(2P0+P3)/3，P2=2P1-P0
    QList<QPointF> knots {QPointF(0, 0), QPointF(3, 6)};
    QList<QPointF> firstControlPoints;
    QList<QPointF> secondControlPoints;

    // Act
    SmoothCurveGenerator::calculateControlPoints(knots, &firstControlPoints, &secondControlPoints);

    // Assert: P1 = ((2*0+3)/3, (2*0+6)/3) = (1, 2)
    ASSERT_EQ(firstControlPoints.size(), 1);
    EXPECT_DOUBLE_EQ(firstControlPoints[0].x(), 1.0);
    EXPECT_DOUBLE_EQ(firstControlPoints[0].y(), 2.0);
    // Assert: P2 = (2*1-0, 2*2-0) = (2, 4)，三点共线保证贝塞尔为直线
    ASSERT_EQ(secondControlPoints.size(), 1);
    EXPECT_DOUBLE_EQ(secondControlPoints[0].x(), 2.0);
    EXPECT_DOUBLE_EQ(secondControlPoints[0].y(), 4.0);
}

TEST_F(SmoothCurveGeneratorTest, CalculateControlPoints_WithFiveKnots_ProducesFiniteControlPoints)
{
    // Arrange: 5 个数据点 -> n=4 段 -> 每段 1 组控制点
    QList<QPointF> knots {
        QPointF(0, 0), QPointF(10, 4), QPointF(20, 2), QPointF(30, 8), QPointF(40, 6)
    };
    QList<QPointF> firstControlPoints;
    QList<QPointF> secondControlPoints;

    // Act
    SmoothCurveGenerator::calculateControlPoints(knots, &firstControlPoints, &secondControlPoints);

    // Assert: 控制点数量与曲线段数一致，且坐标均为有限值（求解矩阵无发散）
    ASSERT_EQ(firstControlPoints.size(), 4);
    ASSERT_EQ(secondControlPoints.size(), 4);
    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(std::isfinite(firstControlPoints[i].x()));
        EXPECT_TRUE(std::isfinite(firstControlPoints[i].y()));
        EXPECT_TRUE(std::isfinite(secondControlPoints[i].x()));
        EXPECT_TRUE(std::isfinite(secondControlPoints[i].y()));
    }
}

// ───────────────────────── calculateFirstControlPoints ─────────────────────────
TEST_F(SmoothCurveGeneratorTest, CalculateFirstControlPoints_WithThreeElementRHS_SolvesTridiagonalSystem)
{
    // Arrange: rhs={2,4,3}, n=3：
    //   前代: r0=2/2=1; tmp[1]=1/2, b=4-1/2=7/2, r1=(4-1)/(7/2)=6/7;
    //         tmp[2]=2/7, b=7/2-2/7=45/14, r2=(3-6/7)/(45/14)=2/3
    //   回代: r1 -= tmp[2]*r2 = 6/7-(2/7)(2/3) = 2/3; r0 -= tmp[1]*r1 = 1-(1/2)(2/3) = 2/3
    const double rhs[3] = {2.0, 4.0, 3.0};
    double *result = new double[3];

    // Act
    SmoothCurveGenerator::calculateFirstControlPoints(result, rhs, 3);

    // Assert: 求解结果与手工推演一致（三分量收敛至 2/3）
    EXPECT_NEAR(result[0], 2.0 / 3.0, 1e-12);
    EXPECT_NEAR(result[1], 2.0 / 3.0, 1e-12);
    EXPECT_NEAR(result[2], 2.0 / 3.0, 1e-12);

    delete[] result;
}

TEST_F(SmoothCurveGeneratorTest, CalculateFirstControlPoints_WithTwoElementRHS_AppliesLastRowBoundaryCoefficient)
{
    // Arrange: rhs={2,8}, n=2（末行系数 3.5-1/2=3 分支）:
    //   前代: r0=2/2=1; r1=(8-1)/3=7/3
    //   回代: r0 -= tmp[1]*r1 = 1-(1/2)(7/3) = -1/6
    const double rhs[2] = {2.0, 8.0};
    double *result = new double[2];

    // Act
    SmoothCurveGenerator::calculateFirstControlPoints(result, rhs, 2);

    // Assert
    EXPECT_NEAR(result[0], -1.0 / 6.0, 1e-12);
    EXPECT_NEAR(result[1], 7.0 / 3.0, 1e-12);

    delete[] result;
}
