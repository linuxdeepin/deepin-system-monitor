/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      Wang Yong <wangyong@deepin.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DISKMONITOR_H
#define DISKMONITOR_H

#include <DApplicationHelper>

#include <QWidget>
#include <QPainterPath>

DWIDGET_USE_NAMESPACE

class DiskMonitor : public QWidget
{
    Q_OBJECT

public:
    DiskMonitor(QWidget *parent = nullptr);
    ~DiskMonitor();

public slots:
    void changeTheme(DApplicationHelper::ColorType themeType);
    void updateStatus(qreal totalReadKbs, qreal totalWriteKbs);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QPixmap iconImage;
    QPixmap iconDarkImage;
    QPixmap iconLightImage;
    QList<qreal> *readSpeeds;
    QList<qreal> *writeSpeeds;
    QPainterPath readPath;
    QPainterPath writePath;
    QColor readColor {"#1094D8"};
    QColor summaryColor;
    QColor textColor;
    QColor writeColor {"#F7B300"};
    qreal totalReadBps = 0;
    qreal totalWriteBps = 0;
    int readRenderMaxHeight = 50;
    int readRenderPaddingX = 13;
    int readRenderPaddingY = 50;
    int readRenderSize = 9;
    int readWaveformsRenderOffsetX = 4;
    int readWaveformsRenderOffsetY = 156;
    int gridPaddingRight = 21;
    int gridPaddingTop = 10;
    int gridRenderOffsetY = 99;
    int gridSize = 20;
    int iconRenderOffsetX = -5;
    int iconRenderOffsetY = 10;
    int pointerRadius = 3;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 9;
    int pointsNumber = 51;
    int textPadding = 12;
    int titleRenderOffsetX = 20;
    int titleRenderSize = 20;
    int writeRenderMaxHeight = 10;
    int writeRenderPaddingX = 13;
    int writeRenderPaddingY = 70;
    int writeRenderSize = 9;
    int writeWaveformsRenderOffsetY = -5;
    int waveformRenderPadding = 20;
};

#endif
