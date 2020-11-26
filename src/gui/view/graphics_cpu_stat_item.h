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
#ifndef GRAPHICS_CPU_STAT_ITEM_H
#define GRAPHICS_CPU_STAT_ITEM_H

#include "graphics_stat_item.h"

/**
 * @brief CPU stat item (grid style or graph style)
 */
class GraphicsCPUStatItem : public GraphicsStatItem
{
public:
    explicit GraphicsCPUStatItem();
};

#endif // GRAPHICS_CPU_STAT_ITEM_H
