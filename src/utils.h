/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
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

#include <QFileInfoList>
#include <QLayout>
#include <QObject>
#include <QPainter>
#include <QString>

class Utils : public QObject
{
    Q_OBJECT
    
public:
    Utils(QObject *parent = 0);
    
    static QFileInfoList getRecordingFileinfos();
    static QSize getRenderSize(int fontSize, QString string);
    static QString formatMillisecond(int millisecond);
    static QString getImagePath(QString imageName);
    static QString getQrcPath(QString imageName);
    static QString getQssPath(QString qssName);
    static QString getRecordingSaveDirectory();
    static bool fileExists(QString path);
    static qreal easeInOut(qreal x);
    static qreal easeInQuad(qreal x);
    static qreal easeInQuint(qreal x);
    static qreal easeOutQuad(qreal x);
    static qreal easeOutQuint(qreal x);
    static void addLayoutWidget(QLayout *layout, QWidget *widget);
    static void applyQss(QWidget *widget, QString qssName);
    static void removeChildren(QWidget *widget);
    static void removeLayoutChild(QLayout *layout, int index);
    static void setFontSize(QPainter &painter, int textSize);
};
