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

#ifndef PROCESS_PAGE_WIDGET_H
#define PROCESS_PAGE_WIDGET_H

#include "ui_common.h"

#include <DApplicationHelper>
#include <DButtonBox>
#include <DFrame>
#include <DStackedWidget>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class MainWindow;
class MonitorCompactView;
class MonitorExpandView;
class ProcessTableView;
class Settings;
class XWinKillPreviewWidget;
class DetailViewStackedWidget;

/**
 * @brief Process & performance monitor view frame
 */
class ProcessPageWidget : public DFrame
{
    Q_OBJECT

public:
    /**
     * @brief Default constructor
     * @param parent Parent object
     */
    explicit ProcessPageWidget(DWidget *parent = nullptr);
    /**
     * @brief Destructor
     */
    ~ProcessPageWidget();

    /**
     * @brief Initialize ui components
     */
    void initUI();
    /**
     * @brief Initialize connections
     */
    void initConnections();

    /**
     * @brief Filters events if this object has been installed as an event filter for the watched object
     * @param obj Object being watched
     * @param event Event to be filtered
     * @return To filter this event out, return true; otherwise return false
     */
    bool eventFilter(QObject *obj, QEvent *event);

public Q_SLOTS:
    /**
     * @brief Switch performance display mode when required
     * @param mode Display mode
     */
    void switchDisplayMode(int mode);

protected:
    /**
     * @brief Paint event handler
     */
    void paintEvent(QPaintEvent *);

private Q_SLOTS:
    /**
     * @brief Popup up process kill confirm dialog
     * @param pid Process id
     */
    void popupKillConfirmDialog(int pid);
    /**
     * @brief Show kill application preview widget
     */
    void showWindowKiller();
    /**
     * @brief Create application preview widget
     */
    void createWindowKiller();

    /**
     * @brief Change icons when theme changed
     * @param themeType Theme currently applied
     */
    void changeIconTheme(DApplicationHelper::ColorType themeType);

    /**
     * @brief 列表数据刷新
     */
    void onStatInfoUpdated();

    /**
     * @brief 详情页切换
     */
    void onDetailWidgetChanged(int index);

    /**
     * @brief 延迟加载左侧数据控件
     */
    void onLoadLeftDataWidgetDelay();

private:
    // global setttings instance
    Settings *m_settings = nullptr;

    // stacked widget to hold expand & compact monitor view
    DStackedWidget *m_views = nullptr;

    DetailViewStackedWidget *m_rightStackView = nullptr;
    // expand monitor view
    MonitorExpandView *m_expandView = nullptr;
    // compact monitor view
    MonitorCompactView *m_compactView = nullptr;

    // process view context text
    DLabel *m_procViewMode = nullptr;
    // process view context summary
    DLabel *m_procViewModeSummary = nullptr;
    // show app view mode button
    DButtonBoxButton *m_appButton = nullptr;
    // show my proc view mode button
    DButtonBoxButton *m_myProcButton = nullptr;
    // show all proc mode button
    DButtonBoxButton *m_allProcButton = nullptr;

    // process table view
    ProcessTableView *m_procTable = nullptr;
    QWidget *m_processWidget = nullptr;

    // kill process by window selection preview widget
    XWinKillPreviewWidget *m_xwkillPreview = nullptr;
};

#endif  // PROCESS_PAGE_WIDGET_H
