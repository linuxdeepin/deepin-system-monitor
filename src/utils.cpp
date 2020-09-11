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

#include <stdio.h>
#include <time.h>
#include <fstream>
#include <sstream>
#include <string>

#include <DDesktopEntry>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFontMetrics>
#include <QIcon>
#include <QImageReader>
#include <QLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QStandardPaths>
#include <QString>
#include <QWidget>
#include <QtDBus>
#include <QtMath>

#include "utils.h"

DCORE_USE_NAMESPACE

namespace Utils {

int getStatusBarMaxWidth()
{
    // TODO: use more elegent way to calc bar width
    return 300;
}

QString getProcessEnvironmentVariable(pid_t pid, QString environmentName)
{
    std::string temp;
    try {
        std::fstream fs;
        fs.open("/proc/" + std::to_string(long(pid)) + "/environ", std::fstream::in);
        std::getline(fs, temp);
        fs.close();
    } catch (std::ifstream::failure &e) {
        Q_UNUSED(e)
        return "FAILED TO READ PROC";
    }

    // change \0 to ' '
    std::replace(temp.begin(), temp.end(), '\0', '\n');

    if (temp.size() < 1) {
        return "";
    }

    foreach (auto environmentVariable, QString::fromStdString(temp).trimmed().split("\n")) {
        if (environmentVariable.startsWith(QString("%1=").arg(environmentName))) {
            return environmentVariable.remove(0, QString("%1=").arg(environmentName).length());
        }
    }

    return "";
}

QString getProcessCmdline(pid_t pid)
{
    std::string temp;
    try {
        std::fstream fs;
        fs.open("/proc/" + std::to_string(long(pid)) + "/cmdline", std::fstream::in);
        std::getline(fs, temp);
        fs.close();
    } catch (std::ifstream::failure &e) {
        Q_UNUSED(e)
        return "FAILED TO READ PROC";
    }

    // change \0 to ' '
    std::replace(temp.begin(), temp.end(), '\0', ' ');

    if (temp.size() < 1) {
        return "";
    }

    return QString::fromStdString(temp).trimmed();
}

void drawLoadingRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth,
                     int loadingAngle, int rotationAngle, QColor foregroundColor,
                     double foregroundOpacity, QColor backgroundColor, double backgroundOpacity,
                     double percent)
{
    drawRing(painter, centerX, centerY, radius, penWidth, loadingAngle, rotationAngle,
             backgroundColor, backgroundOpacity);
    drawRing(painter, centerX, centerY, radius, penWidth, int(loadingAngle * percent), rotationAngle,
             foregroundColor, foregroundOpacity);
}

void drawRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth,
              int loadingAngle, int rotationAngle, QColor color, double opacity)
{
    QRect drawingRect;

    drawingRect.setX(centerX - radius + penWidth);
    drawingRect.setY(centerY - radius + penWidth);
    drawingRect.setWidth(radius * 2 - penWidth * 2);
    drawingRect.setHeight(radius * 2 - penWidth * 2);

    painter.setOpacity(opacity);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(QBrush(QColor(color)), penWidth);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);

    int arcLengthApproximation = penWidth + penWidth / 3;
    painter.drawArc(drawingRect, 90 * 16 - arcLengthApproximation + rotationAngle * 16,
                    -loadingAngle * 16);
}

void setFontSize(QPainter &painter, int textSize)
{
    QFont font = painter.font();
    font.setPointSize(textSize);
    painter.setFont(font);
}
}  // namespace Utils
