/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#ifndef SMOOTHCURVEGENERATOR_H
#define SMOOTHCURVEGENERATOR_H

#include <QList>
#include <QPainterPath>
#include <QPointF>

class SmoothCurveGenerator {
public:
    /**
     * Generate smooth curve by pass in point list.
     * @param points - point list
     * @return - return QPainterPath of smooth surve.
     */
    static QPainterPath generateSmoothCurve(const QList<QPointF> &points);
    
private:
    /**
     * Solves a tridiagonal system for one of coordinates (x or y)
     * of first Bezier control points.
     * @param result - Solution vector.
     * @param rhs - Right hand side vector.
     * @param n - Size of rhs.
     */
    static void calculateFirstControlPoints(double * &result, const double *rhs, int n);
    
    /**
     * Calculate control points of the smooth curve using the given knots.
     * @param knots - Points of the given curve.
     * @param firstControlPoints - Store the generated first control points.
     * @param secondControlPoints - Store the generated second control points.
     */
    static void calculateControlPoints(const QList<QPointF> &knots,
                                       QList<QPointF> *firstControlPoints,
                                       QList<QPointF> *secondControlPoints);
};
#endif // SMOOTHCURVEGENERATOR_H
