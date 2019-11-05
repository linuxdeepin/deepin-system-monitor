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

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <QTimer>

#include "process_view.h"

DWIDGET_USE_NAMESPACE

ProcessView::ProcessView(QList<bool> columnHideFlags)
{
    // Enable frame and radius.
    setFrame(true);
    setClipRadius(8);

    // Set row height.
    setRowHeight(36);

    // Set column widths.
    QList<QString> titles;
    titles << DApplication::translate("Process.Table.Header", "Name")
           << DApplication::translate("Process.Table.Header", "CPU")
           << DApplication::translate("Process.Table.Header", "Memory")
           << DApplication::translate("Process.Table.Header", "Disk write")
           << DApplication::translate("Process.Table.Header", "Disk read")
           << DApplication::translate("Process.Table.Header", "Download")
           << DApplication::translate("Process.Table.Header", "Upload")
           << DApplication::translate("Process.Table.Header", "PID");
    QList<int> widths;
    widths << -1 << 70 << 70 << 80 << 80 << 70 << 70 << 70;
    setColumnTitleInfo(titles, widths, 36);

    // Set column hide flags.
    setColumnHideFlags(columnHideFlags, 0);

    // Focus keyboard when create.
    QTimer::singleShot(0, this, SLOT(setFocus()));

    auto *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &ProcessView::changeTheme);
}

void ProcessView::changeTheme(DApplicationHelper::ColorType themeType)
{
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    titleColor = palette.color(DPalette::Text).name();
    backgroundColor = palette.color(DPalette::Base).name();

    switch (themeType) {
        case DApplicationHelper::LightType:
            titleLineColor = "#000000";

            //            titleAreaColor = "#ffffff";
            //            titleAreaOpacity = 0.02;

            //            backgroundColor = "#ffffff";
            //            backgroundOpacity = 0.03;

            //            frameColor = "#000000";
            //            frameOpacity = 0.1;

            //            searchColor = "#D0D0D0";

            arrowUpNormalImage = arrowUpLightNormalImage;
            arrowUpHoverImage = arrowUpLightHoverImage;
            arrowUpPressImage = arrowUpLightPressImage;
            arrowDownNormalImage = arrowDownLightNormalImage;
            arrowDownHoverImage = arrowDownLightHoverImage;
            arrowDownPressImage = arrowDownLightPressImage;

            scrollbarColor = "#101010";

            scrollbarNormalOpacity = 0.5;
            scrollbarHoverOpacity = 0.7;
            scrollbarPressOpacity = 0.8;

            scrollbarFrameNormalOpacity = 0;
            scrollbarFrameHoverOpacity = 0;
            scrollbarFramePressOpacity = 0;
            break;
        case DApplicationHelper::DarkType:
            titleLineColor = "#ffffff";

            //            titleAreaColor = "#ffffff";
            //            titleAreaOpacity = 0.02;

            //            backgroundColor = "#ffffff";
            //            backgroundOpacity = 0.03;

            //            frameColor = "#000000";
            //            frameOpacity = 0;

            //            searchColor = "#666666";

            arrowUpNormalImage = arrowUpDarkNormalImage;
            arrowUpHoverImage = arrowUpDarkHoverImage;
            arrowUpPressImage = arrowUpDarkPressImage;
            arrowDownNormalImage = arrowDownDarkNormalImage;
            arrowDownHoverImage = arrowDownDarkHoverImage;
            arrowDownPressImage = arrowDownDarkPressImage;

            scrollbarColor = "#ffffff";

            scrollbarNormalOpacity = 0.2;
            scrollbarHoverOpacity = 0.4;
            scrollbarPressOpacity = 0.15;

            scrollbarFrameNormalOpacity = 0.05;
            scrollbarFrameHoverOpacity = 0.1;
            scrollbarFramePressOpacity = 0.05;
            break;
        default:
            break;
    }
}
