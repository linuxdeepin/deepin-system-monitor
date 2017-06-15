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

#include "dthememanager.h"
#include "process_view.h"
#include <QTimer>

DWIDGET_USE_NAMESPACE

ProcessView::ProcessView(QList<bool> columnHideFlags)
{
    initTheme();
    
    connect(DThemeManager::instance(), &DThemeManager::themeChanged, this, &ProcessView::changeTheme);
    
    // Set row height.
    setRowHeight(36);
    
    // Set column widths.
    QList<int> widths;
    widths << -1 << 70 << 70 << 80 << 80 << 70 << 70 << 70;
    setColumnWidths(widths);
    
    // Set column titles.
    QList<QString> titles;
    titles << "名称" << "处理器" << "内存" << "磁盘写入" << "磁盘读取" << "下载" << "上传" << "进程号";
    setColumnTitles(titles, 36);
    
    // Set column hide flags.
    setColumnHideFlags(columnHideFlags, 0);
    
    // Focus keyboard when create.
    QTimer::singleShot(0, this, SLOT(setFocus()));
}

void ProcessView::initTheme()
{
    if (DThemeManager::instance()->theme() == "light") {
        titleColor = "#000000";
        titleLineColor = "#000000";
        
        titleAreaColor = "#ffffff";
        titleAreaOpacity = 0.02;

        backgroundColor = "#ffffff";
        backgroundOpacity = 0.03;
        
        frameColor = "#000000";
        frameOpacity = 0.1;
    } else {
        titleColor = "#9A9A9A";
        titleLineColor = "#ffffff";

        titleAreaColor = "#ffffff";
        titleAreaOpacity = 0.02;
        
        backgroundColor = "#ffffff";
        backgroundOpacity = 0.03;

        frameColor = "#000000";
        frameOpacity = 0;
    }
}

void ProcessView::changeTheme(QString )
{
    initTheme();
}

