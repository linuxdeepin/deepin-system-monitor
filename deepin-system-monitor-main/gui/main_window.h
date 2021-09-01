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

#include <DMainWindow>
#include <DShadowLine>
#include <DStackedWidget>

DWIDGET_USE_NAMESPACE

class Toolbar;
class SystemServicePageWidget;
class ProcessPageWidget;
class Settings;

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
    ~MainWindow() override;

    /**
     * @brief toolbar Get toolbar instance
     * @return Toolbar instance
     */
    inline Toolbar *toolbar() const { return m_toolbar; }

Q_SIGNALS:
    /**
     * @brief Kill application operation requested signal
     */
    void killProcessPerformed();
    /**
     * @brief Display mode changed signal
     * @param mode Display mode
     */
    void displayModeChanged(int mode);
    /**
     * @brief Loading status changed signal
     * @param loading Loading state flag
     */
    void loadingStatusChanged(bool loading);

public Q_SLOTS:
    /**
     * @brief Initialize ui components
     */
    void initDisplay();

    /**
     * @brief onLoadStatusChanged
     */
    void onLoadStatusChanged(bool loading);

    /**
     * @brief onStartMonitorJob
     */
    void onStartMonitorJob();

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
    void resizeEvent(QResizeEvent *event) override;
    /**
     * @brief closeEvent Close event handler
     * @param event Close event
     */
    void closeEvent(QCloseEvent *event) override;
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
    void showEvent(QShowEvent *event) override;

private:
    Settings *m_settings = nullptr;

    Toolbar *m_toolbar = nullptr;
    DStackedWidget *m_pages = nullptr;
    ProcessPageWidget *m_procPage = nullptr;
    SystemServicePageWidget *m_svcPage = nullptr;

    bool m_initLoad = false;
    DShadowLine *m_tbShadow  = nullptr;
    QWidget *m_focusedWidget = nullptr;
};

#endif  // MAIN_WINDOW_H
