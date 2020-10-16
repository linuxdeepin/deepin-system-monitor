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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "ui_common.h"

#include <DMainWindow>
#include <DShadowLine>
#include <DSpinner>
#include <DStackedWidget>

#include <mutex>
#include <thread>

DWIDGET_USE_NAMESPACE

class Toolbar;
class SystemServicePageWidget;
class ProcessPageWidget;
class Settings;
class QTimer;

/**
 * @brief The MainWindow class
 */
class MainWindow : public DMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY(MainWindow)

public:
    /**
     * @brief MainWindow Constructor
     * @param parent Parent Object
     */
    explicit MainWindow(QWidget *parent = nullptr);
    /**
     * @brief Destructor
    */
    ~MainWindow() override;

    /**
     * @brief toolbar Get toolbar instance
     * @return Toolbar instance
     */
    inline Toolbar *toolbar() const { return m_toolbar; }
    /**
     * @brief processPage Get process page instance
     * @return Process page instance
     */
    inline ProcessPageWidget *processPage() const { return m_procPage; }
    /**
     * @brief systemServicePage Get service page instance
     * @return Service page instance
     */
    inline SystemServicePageWidget *systemServicePage() const { return m_svcPage; }

Q_SIGNALS:
    /**
     * @brief Kill application operation requested signal
     */
    void killProcessPerformed();
    /**
     * @brief Display mode changed signal
     * @param mode Display mode
     */
    void displayModeChanged(DisplayMode mode);
    /**
     * @brief Loading status changed signal
     * @param loading Loading state flag
     */
    void loadingStatusChanged(bool loading);

public Q_SLOTS:
    /**
     * @brief Initialize ui components
     */
    inline void initDisplay()
    {
        initUI();
        initConnections();
    }

    /**
     * @brief Show shortcut help dialog
     */
    void displayShortcutHelpDialog();

protected:
    /**
     * @brief Initialize ui components
     */
    void initUI();
    /**
     * @brief Initialize connections
     */
    void initConnections();

    /**
     * @brief resizeEvent Resize event handler
     * @param event Resize event
     */
    virtual void resizeEvent(QResizeEvent *event) override;
    /**
     * @brief closeEvent Close event handler
     * @param event Close event
     */
    virtual void closeEvent(QCloseEvent *event) override;
    /**
     * @brief eventFilter Filters events if this object has been installed as an event filter for the watched object.
     * @param obj Object being watched
     * @param event Event to be filtered
     * @return To filter the event out, return true; otherwise return false
     */
    bool eventFilter(QObject *obj, QEvent *event) override;
    /**
     * @brief showEvent Show event handler
     * @param event Show event
     */
    virtual void showEvent(QShowEvent *event) override;

private:
    // Global config settings instance
    Settings *m_settings;

    // Interactive kill application (select by window) action instance
    QAction *m_killAction {};
    // Display mode menu (compact/detail)
    DMenu *m_modeMenu {};

    // Toolbar instance
    Toolbar *m_toolbar {};
    // Stacked widget to hold process & service page
    DStackedWidget *m_pages {};
    // Process page widget
    ProcessPageWidget *m_procPage {};
    // Service page widget
    SystemServicePageWidget *m_svcPage {};
    // Shadow widget under titlebar
    DShadowLine *m_tbShadow {};

    // loading status flag
    bool m_loading {true};
    char __unused__[7];     // ##padding##

    // Last focused widget
    QWidget *m_focusedWidget {};
};

#endif  // MAIN_WINDOW_H
