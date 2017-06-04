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

#include "process_view.h"
#include <QTimer>

ProcessView::ProcessView()
{
    // Set row height.
    setRowHeight(36);
    
    // Set column widths.
    QList<int> widths;
    widths << -1 << 60 << 70 << 80 << 80 << 70 << 70 << 60;
    setColumnWidths(widths);
    
    // Set column titles.
    QList<QString> titles;
    titles << "进程名" << "CPU" << "内存" << "磁盘写入" << "磁盘读取" << "下载" << "上传" << "序号";
    setColumnTitles(titles, 36);
    
    // Set column hide flags.
    QList<bool> toggleHideFlags;
    toggleHideFlags << false << true << true << true << true << true << true << true;
    setColumnHideFlags(toggleHideFlags);
    
    // Focus keyboard when create.
    QTimer::singleShot(0, this, SLOT(setFocus()));
}
