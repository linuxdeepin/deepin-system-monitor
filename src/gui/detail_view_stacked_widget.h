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
#ifndef DETAIL_VIEW_STACKED_WIDGET_H
#define DETAIL_VIEW_STACKED_WIDGET_H

#include "animation_stackedwidget.h"
#include <DMenu>

/**
 * @brief Detail view stacked widget (main content stacked area)
 */
class MemDetailViewWidget;
class NetifDetailViewWidget;
class BlockDevDetailViewWidget;

DWIDGET_USE_NAMESPACE
class DetailViewStackedWidget : public AnimationStackedWidget
{
public:
    explicit DetailViewStackedWidget(QWidget *parent);

    void addProcessWidget(QWidget *processWidget);

    void deleteDetailPage();

public slots:
    void onShowPerformMenu(QPoint pos);
    void onDetailInfoClicked();
    void onSwitchProcessPage();
    void onSwitchPageFinished();

private:
    QString m_lastDteailWidgetName;

    QWidget *m_processWidget = nullptr;
    QWidget *m_cpudetailWidget = nullptr;
    QWidget *m_memDetailWidget = nullptr;
    QWidget *m_netifDetailWidget = nullptr;
    QWidget *m_blockDevDetailWidget = nullptr;

    QAction *cpuAct;
    QAction *memAct;
    QAction *netifAct;
    QAction *blockDevAct;

    DMenu *m_menu = nullptr;
};

#endif // DETAIL_VIEW_STACKED_WIDGET_H
