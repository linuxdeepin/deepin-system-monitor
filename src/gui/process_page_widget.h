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
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PROCESS_PAGE_WIDGET_H
#define PROCESS_PAGE_WIDGET_H

#include <DApplicationHelper>
#include <DButtonBox>
#include <DFrame>
#include <DStackedWidget>
#include <DLabel>

#include "ui_common.h"

DWIDGET_USE_NAMESPACE

class MainWindow;
class InteractiveKill;
class MonitorCompactView;
class MonitorExpandView;
class ProcessTableView;
class Settings;

class ProcessPageWidget : public DFrame
{
    Q_OBJECT

public:
    explicit ProcessPageWidget(DWidget *parent = nullptr);
    ~ProcessPageWidget();

    void adjustStatusBarWidth();

    void initUI();
    void initConnections();

    bool eventFilter(QObject *obj, QEvent *event);

public Q_SLOTS:
    void switchDisplayMode(DisplayMode mode);

protected:
    void paintEvent(QPaintEvent *);

private Q_SLOTS:
    void popupKillConfirmDialog(int pid);
    void showWindowKiller();
    void createWindowKiller();
    void updateProcessSummary(int napps, int nprocs);
    void changeIconTheme(DApplicationHelper::ColorType themeType);

private:
    Settings *m_settings;

    DStackedWidget *m_views {nullptr};
    MonitorExpandView *m_expandView {nullptr};
    MonitorCompactView *m_compactView {nullptr};

    DLabel *m_procViewMode {nullptr};
    DLabel *m_procViewModeSummary {nullptr};
    DButtonBoxButton *m_appButton {nullptr};
    DButtonBoxButton *m_myProcButton {nullptr};
    DButtonBoxButton *m_allProcButton {nullptr};

    ProcessTableView *m_procTable {nullptr};

    InteractiveKill *m_wndKiller {nullptr};
};

#endif  // PROCESS_PAGE_WIDGET_H
