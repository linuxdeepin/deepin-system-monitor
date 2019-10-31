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

#include <DApplicationHelper>
#include <DHiDPIHelper>
#include <DPalette>
#include <QDebug>

#include "dthememanager.h"
#include "process_switch_tab.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE

ProcessSwitchTab::ProcessSwitchTab(int tabIndex)
{
    onlyGuiDarkNormalImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_gui_dark_normal.svg"));
    onlyGuiDarkHoverImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_gui_dark_hover.svg"));
    onlyGuiDarkActiveImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_gui_dark_active.svg"));
    onlyMeDarkNormalImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_me_dark_normal.svg"));
    onlyMeDarkHoverImage = DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_me_dark_hover.svg"));
    onlyMeDarkActiveImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_me_dark_active.svg"));
    allProcessDarkNormalImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("all_process_dark_normal.svg"));
    allProcessDarkHoverImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("all_process_dark_hover.svg"));
    allProcessDarkActiveImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("all_process_dark_active.svg"));

    onlyGuiLightNormalImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_gui_light_normal.svg"));
    onlyGuiLightHoverImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_gui_light_hover.svg"));
    onlyGuiLightActiveImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_gui_light_active.svg"));
    onlyMeLightNormalImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_me_light_normal.svg"));
    onlyMeLightHoverImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_me_light_hover.svg"));
    onlyMeLightActiveImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("only_me_light_active.svg"));
    allProcessLightNormalImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("all_process_light_normal.svg"));
    allProcessLightHoverImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("all_process_light_hover.svg"));
    allProcessLightActiveImage =
        DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("all_process_light_active.svg"));

    activeIndex = tabIndex;

    installEventFilter(this);  // add event filter
    setMouseTracking(true);    // make MouseMove can response

    setFixedSize(width * 3, height);

    auto *pAppHelper = DApplicationHelper::instance();
    connect(pAppHelper, &DApplicationHelper::themeTypeChanged, this,
            &ProcessSwitchTab::changeTheme);
    m_themeType = pAppHelper->themeType();
}

void ProcessSwitchTab::mouseMoveEvent(QMouseEvent *mouseEvent)
{
    int prevHoverIndex = hoverIndex;

    if (mouseEvent->x() < width) {
        hoverIndex = 0;
    } else if (mouseEvent->x() < width * 2) {
        hoverIndex = 1;
    } else {
        hoverIndex = 2;
    }

    if (hoverIndex != prevHoverIndex) {
        repaint();
    }
}

void ProcessSwitchTab::mousePressEvent(QMouseEvent *mouseEvent)
{
    int prevActiveIndex = activeIndex;

    if (mouseEvent->x() < width) {
        activeIndex = 0;
    } else if (mouseEvent->x() < width * 2) {
        activeIndex = 1;
    } else {
        activeIndex = 2;
    }

    if (activeIndex != prevActiveIndex) {
        activeTab(activeIndex);

        repaint();
    }
}

void ProcessSwitchTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    switch (m_themeType) {
        case DApplicationHelper::LightType:
            frameColor = "#000000";
            frameOpacity = 0.05;

            onlyGuiNormalImage = onlyGuiLightNormalImage;
            onlyGuiHoverImage = onlyGuiLightHoverImage;
            onlyGuiActiveImage = onlyGuiLightActiveImage;
            onlyMeNormalImage = onlyMeLightNormalImage;
            onlyMeHoverImage = onlyMeLightHoverImage;
            onlyMeActiveImage = onlyMeLightActiveImage;
            allProcessNormalImage = allProcessLightNormalImage;
            allProcessHoverImage = allProcessLightHoverImage;
            allProcessActiveImage = allProcessLightActiveImage;
            break;
        case DApplicationHelper::DarkType:
            frameColor = "#ffffff";
            frameOpacity = 0.05;

            onlyGuiNormalImage = onlyGuiDarkNormalImage;
            onlyGuiHoverImage = onlyGuiDarkHoverImage;
            onlyGuiActiveImage = onlyGuiDarkActiveImage;
            onlyMeNormalImage = onlyMeDarkNormalImage;
            onlyMeHoverImage = onlyMeDarkHoverImage;
            onlyMeActiveImage = onlyMeDarkActiveImage;
            allProcessNormalImage = allProcessDarkNormalImage;
            allProcessHoverImage = allProcessDarkHoverImage;
            allProcessActiveImage = allProcessDarkActiveImage;
            break;
        default:
            break;
    }

    int penSize = 1;

    QPainterPath path;
    path.addRoundedRect(QRect(rect().x() + penSize, rect().y() + penSize,
                              rect().width() - penSize * 2, rect().height() - penSize * 2 - 1),
                        5, 5);

    QPen pen;
    pen.setColor(QColor(frameColor));
    painter.setOpacity(frameOpacity);
    pen.setWidth(penSize);
    painter.setPen(pen);
    painter.drawPath(path);

    painter.drawLine(rect().x() + width, rect().y() + penSize + 1, rect().x() + width,
                     rect().y() + rect().height() - penSize * 2 - 1);
    painter.drawLine(rect().x() + width * 2, rect().y() + penSize + 1, rect().x() + width * 2,
                     rect().y() + rect().height() - penSize * 2 - 1);

    painter.setOpacity(1);
    for (int i = 0; i < 3; i++) {
        int iconX = rect().x() + width * i;
        int iconY = rect().y();
        if (i == activeIndex) {
            if (i == 0) {
                painter.drawPixmap(QPoint(iconX, iconY), onlyGuiActiveImage);
            } else if (i == 1) {
                painter.drawPixmap(QPoint(iconX, iconY), onlyMeActiveImage);
            } else {
                painter.drawPixmap(QPoint(iconX, iconY), allProcessActiveImage);
            }
        } else if (i == hoverIndex) {
            if (i == 0) {
                painter.drawPixmap(QPoint(iconX, iconY), onlyGuiHoverImage);
            } else if (i == 1) {
                painter.drawPixmap(QPoint(iconX, iconY), onlyMeHoverImage);
            } else {
                painter.drawPixmap(QPoint(iconX, iconY), allProcessHoverImage);
            }
        } else {
            if (i == 0) {
                painter.drawPixmap(QPoint(iconX, iconY), onlyGuiNormalImage);
            } else if (i == 1) {
                painter.drawPixmap(QPoint(iconX, iconY), onlyMeNormalImage);
            } else {
                painter.drawPixmap(QPoint(iconX, iconY), allProcessNormalImage);
            }
        }
    }
}

void ProcessSwitchTab::leaveEvent(QEvent *event)
{
    hoverIndex = -1;

    QWidget::leaveEvent(event);

    repaint();
}

void ProcessSwitchTab::changeTheme(DApplicationHelper::ColorType themeType)
{
    m_themeType = themeType;
}
