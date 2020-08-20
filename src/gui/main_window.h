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

class MainWindow : public DMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY(MainWindow)

public:
    static MainWindow *instance()
    {
        MainWindow *sin = m_instance.load();
        if (!sin) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!sin) {
                sin = new MainWindow();
                m_instance.store(sin);
            }
        }

        return sin;
    }

    inline Toolbar *toolbar() const { return m_toolbar; }
    inline ProcessPageWidget *processPage() const { return m_procPage; }
    inline SystemServicePageWidget *systemServicePage() const { return m_svcPage; }

Q_SIGNALS:
    void killProcessPerformed();
    void displayModeChanged(DisplayMode mode);
    void loadingStatusChanged(bool loading);
    void authProgressStarted();
    void authProgressEnded();

public Q_SLOTS:
    inline void initDisplay()
    {
        initUI();
        initConnections();
    }

    void displayShortcutHelpDialog();

protected:
    void initUI();
    void initConnections();

    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;

private:
    MainWindow(DWidget *parent = nullptr);
    ~MainWindow() override;

    Settings *m_settings;

    QAction *m_killAction                       {};
    DMenu   *m_modeMenu                         {};

    Toolbar                 *m_toolbar          {};
    DStackedWidget          *m_pages            {};
    ProcessPageWidget       *m_procPage         {};
    SystemServicePageWidget *m_svcPage          {};
    DShadowLine             *m_tbShadow         {};

    bool m_loading {true};
    char __unused__[7];     // ##padding##
    QTimer *m_timer {nullptr};

    static std::atomic<MainWindow *> m_instance;
    static std::mutex m_mutex;
};

#endif  // MAIN_WINDOW_H
