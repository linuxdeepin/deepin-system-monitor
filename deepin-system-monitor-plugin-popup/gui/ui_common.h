/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <QColor>
#include <QPainter>
#include <QPixmap>

// Display mode enum
enum DisplayMode { kDisplayModeCompact = 0, kDisplayModeExpand };
enum DisplayModuleIndex {kDisPlayCPUIndex = 1, kDisPlayMemoryIndex = 2, kDisPlayNetIfIndex = 3, kDisPlayBlockIndex = 4};

/**
 * @brief Format QColor to human friendly text
 * @param color Color to be formated
 * @param base The base argument specifies the base to use when converting integer number to string
 * @return Formatted color text
 */
inline QString formatColor(const QColor &color, int base = 10)
{
    if (base == 16) {
        // hex decimal format
        return QString("A:%1 R:%2 G:%3 B:%4")
               .arg(color.alpha(), 0, 16)
               .arg(color.red(), 0, 16)
               .arg(color.green(), 0, 16)
               .arg(color.blue(), 0, 16);
    } else if (base == 10) {
        // decimal format
        return QString("A:%1 R:%2 G:%3 B:%4")
               .arg(color.alpha())
               .arg(color.red())
               .arg(color.green())
               .arg(color.blue());
    } else {
        return {};
    }
}

/**
 * @brief Generate icon qrc res path ready for use
 * @param icon Qrc icon name
 * @return Concatenated qrc icon file path
 */
inline QString iconPathFromQrc(const QString &icon)
{
    return QString(":/icons/deepin/builtin/%1").arg(icon);
}

#endif  // UI_COMMON_H
