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
#ifndef UTILS_H
#define UTILS_H

#include <dwindowmanager.h>
#include <proc/readproc.h>
#include <QFileInfoList>
#include <QLayout>
#include <QObject>
#include <QPainter>
#include <QString>
#include <unordered_set>
#include "find_window_title.h"
#include <QDebug>

#include <QVariant>

DWM_USE_NAMESPACE

const int RECTANGLE_PADDING = 24;
const int RECTANGLE_RADIUS = 8;
const int RECTANGLE_FONT_SIZE = 11;

namespace Utils {

enum SizeUnit {
    B,      // 1024 ^ 0
    KB,     // 1024 ^ 1
    MB,     // 1024 ^ 2
    GB,     // 1024 ^ 3
    TB,     // 1024 ^ 4
    PB,     // 1024 ^ 5
    EB      // 1024 ^ 6
};

const char *const UnitSuffix[] = {"B", "K", "M", "G", "T", "P", "E"};
const char *const UnitSuffixExt[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};

inline QString formatUnit(QVariant size, SizeUnit base = B, int prec = 1, bool isSpeed = false)
{
    int u = base;
    if (!size.canConvert(QMetaType::Double)) return {};
    qreal v = size.toReal();

    while (v > 1024. && u <= EB) {
        v /= 1024;
        u++;
    }

    if (isSpeed) {
        return QString("%1%2%3").arg(v, 0, 'f', prec).arg(UnitSuffixExt[u]).arg("/s");
    } else {
        return QString("%1%2").arg(v, 0, 'f', prec).arg(UnitSuffix[u]);
    }
}

inline QString normalizeProcName(const QString &proc_name, const QByteArrayList &cmdline)
{
    if (cmdline.size() == 0 || proc_name.length() < 15) {
        return proc_name;
    }

    // windows apps case
    if (cmdline[0].length() > 3 &&
            isalpha(cmdline[0].at(0)) &&
            cmdline[0].at(1) == ':' &&
            (cmdline[0].at(2) == '/' || cmdline[0].at(2) == '\\')) {
        return QFileInfo(cmdline[0]).fileName();
    }

    for (auto cmd : cmdline) {
        QString name = {basename(cmd)};
        if (name.startsWith(proc_name.toLocal8Bit())) {
            return name;
        }
    }
    return proc_name;
}

int getStatusBarMaxWidth();
int getWindowPid(DWindowManager *windowManager, xcb_window_t window);
long getProcessMemory(QString cmdline, long residentMemroy, long shareMemory);
QList<xcb_window_t> getTrayWindows();
QMap<QString, QString> getProcessDescriptions();
QMap<QString, QString> getDesktopfileMap();
QPixmap getDesktopFileIcon(std::string desktopFile, int iconSize = 24);
QPixmap getProcessIcon(int pid, std::string desktopFile,
                       QScopedPointer<FindWindowTitle> &findWindowTitle, int iconSize);
QSize getRenderSize(int fontSize, QString string);

inline QString formatUnitSize(QVariant v, QStringList orders, bool showUnit = true, int prec = 1)
{
    int order = 0;
    qreal value = v.toReal();
    while (value >= 1024 && order + 1 < orders.size()) {
        order++;
        value = value / 1024;
    }

    QString size = QString::number(value, 'f', prec);

    if (showUnit) {
        return QString("%1%2").arg(size).arg(orders[order]);
    } else {
        return QString("%1").arg(size);
    }
}

inline QString formatBandwidth(QVariant v)
{
    QStringList orders;
    orders << "KB/s"
           << "MB/s"
           << "GB/s"
           << "TB/s";

    return formatUnitSize(v, orders);
}

inline QString formatByteCount(QVariant v, bool showUnit = true, int prec = 1)
{
    QStringList orders;
    orders << "B"
           << "K"
           << "M"
           << "G"
           << "T";

    return formatUnitSize(v, orders, showUnit, prec);
}

QString getProcessCmdline(pid_t pid);
QString getProcessEnvironmentVariable(pid_t pid, QString environmentName);
QString getProcessNameFromCmdLine(const pid_t pid);
QString getQrcPath(QString imageName);
QString getFlatpakAppIcon(QString flatpakAppid);
QDir getFlatpakAppPath(QString flatpakAppid);
bool fileExists(QString path);
void blurRect(DWindowManager *windowManager, int widgetId, QRectF rect);
void blurRects(DWindowManager *windowManager, int widgetId, QList<QRectF> rects);
void clearBlur(DWindowManager *windowManager, int widgetId);
void drawLoadingRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth,
                     int loadingAngle, int rotationAngle, QColor foregroundColor,
                     double foregroundOpacity, QColor backgroundColor, double backgroundOpacity,
                     double percent);
void drawRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth,
              int loadingAngle, int rotationAngle, QColor color, double opacity);
void drawTooltipBackground(QPainter &painter, QRect rect, qreal opacity = 0.4);
void drawTooltipText(QPainter &painter, QString text, QString textColor, int textSize, QRectF rect);
void passInputEvent(int wid);
void setFontSize(QPainter &painter, int textSize);
const std::vector<std::string> explode(const std::string &s, const char &c);
}  // namespace Utils

#endif
