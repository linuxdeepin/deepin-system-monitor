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

#include "constant.h"
#include "dthememanager.h"
#include "dwindowmanagerhelper.h"
#include "main_window.h"
#include <DTitlebar>
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QStyleFactory>
#include <iostream>
#include <signal.h>

using namespace std;

MainWindow::MainWindow(DMainWindow *parent) : DMainWindow(parent)
{
    installEventFilter(this);   // add event filter

    settings = new Settings(this);
    settings->init();

    // Init window size.
    int width = Constant::WINDOW_MIN_WIDTH;
    int height = Constant::WINDOW_MIN_HEIGHT;

    if (!settings->getOption("window_width").isNull()) {
        width = settings->getOption("window_width").toInt();
    }

    if (!settings->getOption("window_height").isNull()) {
        height = settings->getOption("window_height").toInt();
    }

    this->resize(width, height);
    
    // Init.
    if (this->titlebar()) {
        menu = new QMenu();
        menu->setStyle(QStyleFactory::create("dlight"));
        killAction = new QAction(tr("Force to end application"), this);
        connect(killAction, &QAction::triggered, this, &MainWindow::showWindowKiller);
        themeAction = new QAction(tr("Dark theme"), this);
        themeAction->setCheckable(true);
        connect(themeAction, &QAction::triggered, this, &MainWindow::switchTheme);
        menu->addAction(killAction);
        menu->addAction(themeAction);
        menu->addSeparator();

        initTheme();

        connect(DThemeManager::instance(), &DThemeManager::themeChanged, this, &MainWindow::changeTheme);
        connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasCompositeChanged, this, [=] () {
                changeTheme(DThemeManager::instance()->theme());
            });

        toolbar = new Toolbar();
        this->titlebar()->setCustomWidget(toolbar, Qt::AlignVCenter, false);
        this->titlebar()->setMenu(menu);

        layoutWidget = new QWidget();
        layout = new QHBoxLayout(layoutWidget);

        this->setCentralWidget(layoutWidget);

        int tabIndex = settings->getOption("process_tab_index").toInt();

        processManager = new ProcessManager(tabIndex, getColumnHideFlags(), getSortingIndex(), getSortingOrder());
        processManager->getProcessView()->installEventFilter(this);
        statusMonitor = new StatusMonitor(tabIndex);

        connect(toolbar, &Toolbar::pressEsc, processManager, &ProcessManager::focusProcessView);
        connect(toolbar, &Toolbar::pressTab, processManager, &ProcessManager::focusProcessView);

        connect(processManager, &ProcessManager::activeTab, this, &MainWindow::switchTab);
        connect(processManager, &ProcessManager::changeColumnVisible, this, &MainWindow::recordVisibleColumn);
        connect(processManager, &ProcessManager::changeSortingStatus, this, &MainWindow::recordSortingStatus);

        connect(statusMonitor, &StatusMonitor::updateProcessStatus, processManager, &ProcessManager::updateStatus, Qt::QueuedConnection);
        connect(statusMonitor, &StatusMonitor::updateProcessNumber, processManager, &ProcessManager::updateProcessNumber, Qt::QueuedConnection);

        connect(toolbar, &Toolbar::search, processManager, &ProcessManager::handleSearch, Qt::QueuedConnection);

        statusMonitor->updateStatus();

        layout->addWidget(statusMonitor);
        layout->addWidget(processManager);

        killPid = -1;

        killProcessDialog = new DDialog(QString(tr("End application")), QString(tr("Ending this application may cause data loss.\nAre you sure to continue?")), this);
        killProcessDialog->setWindowFlags(killProcessDialog->windowFlags() | Qt::WindowStaysOnTopHint);
        killProcessDialog->setIcon(QIcon(Utils::getQrcPath("deepin-system-monitor.svg")));
        killProcessDialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
        killProcessDialog->addButton(QString(tr("End application")), true, DDialog::ButtonNormal);
        connect(killProcessDialog, &DDialog::buttonClicked, this, &MainWindow::dialogButtonClicked);

        killer = NULL;
    }
}

MainWindow::~MainWindow()
{
    // We don't need clean pointers because application has exit here.
}

void MainWindow::changeTheme(QString theme)
{
    if (theme == "light") {
        backgroundColor = "#FFFFFF";

        if (DWindowManagerHelper::instance()->hasComposite()) {
            setBorderColor(QColor(0, 0, 0, 0.15 * 255));
        } else {
            setBorderColor(QColor(217, 217, 217));
        }
    } else {
        backgroundColor = "#0E0E0E";

        if (DWindowManagerHelper::instance()->hasComposite()) {
            setBorderColor(QColor(0, 0, 0, 0.15 * 255));
        } else {
            setBorderColor(QColor(16, 16, 16));
        }
    }

    initThemeAction();
}

QList<bool> MainWindow::getColumnHideFlags()
{
    QString processColumns = settings->getOption("process_columns").toString();

    QList<bool> toggleHideFlags;
    toggleHideFlags << processColumns.contains("name");
    toggleHideFlags << processColumns.contains("cpu");
    toggleHideFlags << processColumns.contains("memory");
    toggleHideFlags << processColumns.contains("disk_write");
    toggleHideFlags << processColumns.contains("disk_read");
    toggleHideFlags << processColumns.contains("download");
    toggleHideFlags << processColumns.contains("upload");
    toggleHideFlags << processColumns.contains("pid");

    return toggleHideFlags;
}

bool MainWindow::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        adjustStatusBarWidth();
    } else if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_F) {
            if (keyEvent->modifiers() == Qt::ControlModifier) {
                toolbar->focusInput();
            }
        }
    } else if (event->type() == QEvent::Close) {
        if (this->rect().width() > Constant::WINDOW_MIN_WIDTH) {
            settings->setOption("window_width", this->rect().width());
        }

        if (this->rect().height() > Constant::WINDOW_MIN_HEIGHT) {
            settings->setOption("window_height", this->rect().height());
        }
    }

    return false;
}

int MainWindow::getSortingIndex()
{
    QString sortingName = settings->getOption("process_sorting_column").toString();

    QList<QString> columnNames = {
        "name", "cpu", "memory", "disk_write", "disk_read", "download", "upload", "pid"
    };

    return columnNames.indexOf(sortingName);
}

bool MainWindow::getSortingOrder()
{
    return settings->getOption("process_sorting_order").toBool();
}

void MainWindow::initTheme()
{
    QString theme = settings->getOption("theme_style").toString();
    DThemeManager::instance()->setTheme(theme);

    changeTheme(theme);
}

void MainWindow::initThemeAction()
{
    themeAction->setChecked(settings->getOption("theme_style") == "dark");
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QPainterPath path;
    path.addRect(QRectF(rect()));
    painter.setOpacity(1);
    painter.fillPath(path, QColor(backgroundColor));
}

void MainWindow::createWindowKiller()
{
    killer = new InteractiveKill();
    killer->setFocus();
    connect(killer, &InteractiveKill::killWindow, this, &MainWindow::popupKillConfirmDialog, Qt::QueuedConnection);
}

void MainWindow::dialogButtonClicked(int index, QString)
{
    if (index == 1) {
        if (killPid != -1) {
            if (kill(killPid, SIGKILL) != 0) {
                cout << "Kill failed." << endl;
            }

            killPid = -1;
        }
    }
}

void MainWindow::popupKillConfirmDialog(int pid)
{
    killer->close();

    killPid = pid;
    killProcessDialog->show();
}

void MainWindow::recordSortingStatus(int index, bool sortingOrder)
{
    QList<QString> columnNames = {
        "name", "cpu", "memory", "disk_write", "disk_read", "download", "upload", "pid"
    };

    settings->setOption("process_sorting_column", columnNames[index]);
    settings->setOption("process_sorting_order", sortingOrder);
}

void MainWindow::recordVisibleColumn(int, bool, QList<bool> columnVisibles)
{
    QList<QString> visibleColumns;
    visibleColumns << "name";


    if (columnVisibles[1]) {
        visibleColumns << "cpu";
    }

    if (columnVisibles[2]) {
        visibleColumns << "memory";
    }

    if (columnVisibles[3]) {
        visibleColumns << "disk_write";
    }

    if (columnVisibles[4]) {
        visibleColumns << "disk_read";
    }

    if (columnVisibles[5]) {
        visibleColumns << "download";
    }

    if (columnVisibles[6]) {
        visibleColumns << "upload";
    }

    if (columnVisibles[7]) {
        visibleColumns << "pid";
    }

    QString processColumns = "";
    for (int i = 0; i < visibleColumns.length(); i++) {
        if (i != visibleColumns.length() - 1) {
            processColumns += QString("%1,").arg(visibleColumns[i]);
        } else {
            processColumns += visibleColumns[i];
        }
    }

    settings->setOption("process_columns", processColumns);
}

void MainWindow::showWindowKiller()
{
    // Minimize window before show killer window.
    this->showMinimized();
    
    QTimer::singleShot(200, this, SLOT(createWindowKiller()));
}

void MainWindow::switchTab(int index)
{
    if (index == 0) {
        statusMonitor->switchToOnlyGui();
    } else if (index == 1) {
        statusMonitor->switchToOnlyMe();
    } else {
        statusMonitor->switchToAllProcess();
    }

    settings->setOption("process_tab_index", index);
}

void MainWindow::switchTheme()
{
    if (settings->getOption("theme_style") == "dark") {
        settings->setOption("theme_style", "light");
        DThemeManager::instance()->setTheme("light");

        themeAction->setChecked(false);
    } else {
        settings->setOption("theme_style", "dark");
        DThemeManager::instance()->setTheme("dark");

        themeAction->setChecked(true);
    }

    repaint();
}

void MainWindow::adjustStatusBarWidth()
{
    QRect rect = QApplication::desktop()->screenGeometry();

    // Just change status monitor width when screen width is more than 1024.
    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    if (rect.width() * 0.2 > statusBarMaxWidth) {
        if (windowState() == Qt::WindowMaximized) {
            statusMonitor->setFixedWidth(rect.width() * 0.2);
        } else {
            statusMonitor->setFixedWidth(statusBarMaxWidth);
        }
    }
}
