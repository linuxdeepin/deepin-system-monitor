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

#include <signal.h>
#include <DApplicationHelper>
#include <DHiDPIHelper>
#include <DTitlebar>
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QStyleFactory>
#include <iostream>

#include "constant.h"
#include "dthememanager.h"
#include "dwindowmanagerhelper.h"
#include "main_window.h"
#include "process_page_widget.h"

using namespace std;

ProcessPageWidget::ProcessPageWidget(DWidget *parent)
    : DFrame(parent)
{
    m_settings = Settings::instance();
    Q_ASSERT(m_settings != nullptr);
    m_settings->init();

    initUI();
    initConnections();
}

ProcessPageWidget::~ProcessPageWidget() {}

void ProcessPageWidget::initUI()
{
    installEventFilter(this);  // add event filter

    // Init.
    layout = new QHBoxLayout(this);
    setLayout(layout);

    QVariant vindex = m_settings->getOption("process_tab_index");
    int tabIndex = 0;
    if (vindex.isValid())
        tabIndex = vindex.toInt();

    processManager =
        new ProcessManager(tabIndex, getColumnHideFlags(), getSortingIndex(), getSortingOrder());
    processManager->getProcessView()->installEventFilter(this);
    statusMonitor = new StatusMonitor(tabIndex);

    //    connect(toolbar, &Toolbar::pressEsc, processManager, &ProcessManager::focusProcessView);
    //    connect(toolbar, &Toolbar::pressTab, processManager, &ProcessManager::focusProcessView);

    MainWindow *mainWindow = MainWindow::instance();
    connect(mainWindow->toolbar(), &Toolbar::search, processManager, &ProcessManager::handleSearch);

    statusMonitor->updateStatus();

    layout->addWidget(statusMonitor);
    layout->addWidget(processManager);

    killPid = -1;

    killProcessDialog = new DDialog(QString(tr("End application")),
                                    QString(tr("Ending this application may cause data "
                                               "loss.\nAre you sure you want to continue?")),
                                    this);
    killProcessDialog->setWindowFlags(killProcessDialog->windowFlags() | Qt::WindowStaysOnTopHint);
    killProcessDialog->setIconPixmap(QPixmap(Utils::getQrcPath("deepin-system-monitor.svg")));
    killProcessDialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
    killProcessDialog->addButton(QString(tr("End application")), true, DDialog::ButtonNormal);
    connect(killProcessDialog, &DDialog::buttonClicked, this,
            &ProcessPageWidget::dialogButtonClicked);

    killer = nullptr;

    QVariant mode = m_settings->getOption("display_mode");
    if (mode.isValid()) {
        switchDisplayMode(static_cast<DisplayMode>(mode.toInt()));
    } else {
        switchDisplayMode(kDisplayModeCompact);
    }
}

void ProcessPageWidget::initConnections()
{
    connect(processManager, &ProcessManager::activeTab, this, &ProcessPageWidget::switchTab);
    connect(processManager, &ProcessManager::changeColumnVisible, this,
            &ProcessPageWidget::recordVisibleColumn);
    connect(processManager, &ProcessManager::changeSortingStatus, this,
            &ProcessPageWidget::recordSortingStatus);

    connect(statusMonitor, &StatusMonitor::updateProcessStatus, processManager,
            &ProcessManager::updateStatus, Qt::QueuedConnection);
    connect(statusMonitor, &StatusMonitor::updateProcessNumber, processManager,
            &ProcessManager::updateProcessNumber, Qt::QueuedConnection);

    MainWindow *mainWindow = MainWindow::instance();
    connect(mainWindow, &MainWindow::killProcessPerformed, this,
            &ProcessPageWidget::showWindowKiller);
    connect(mainWindow, &MainWindow::displayModeChanged, this,
            &ProcessPageWidget::switchDisplayMode);
}

QList<bool> ProcessPageWidget::getColumnHideFlags()
{
    QString processColumns = m_settings->getOption("process_columns").toString();

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

bool ProcessPageWidget::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        adjustDiskMoitor();
    }

    if (event->type() == QEvent::WindowStateChange) {
        adjustStatusBarWidth();
    } else if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_F) {
            // TODO: control + f
            if (keyEvent->modifiers() == Qt::ControlModifier) {
                //                toolbar->focusInput();
            }
        }
    }

    return false;
}

int ProcessPageWidget::getSortingIndex()
{
    QString sortingName = m_settings->getOption("process_sorting_column").toString();

    QList<QString> columnNames = {"name",      "cpu",      "memory", "disk_write",
                                  "disk_read", "download", "upload", "pid"};

    return columnNames.indexOf(sortingName);
}

bool ProcessPageWidget::getSortingOrder()
{
    return m_settings->getOption("process_sorting_order").toBool();
}

// TODO: save compact mode check state
// void ProcessPageWidget::initCompactModeAction()
//{
//    compactModeAction->setChecked(settings->getOption("compact_mode").toBool());
//}

void ProcessPageWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QPainterPath path;
    path.addRect(QRectF(rect()));
    painter.setOpacity(1);

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QColor bgColor = palette.color(DPalette::Base);

    painter.fillPath(path, bgColor);
}

void ProcessPageWidget::createWindowKiller()
{
    killer = new InteractiveKill();
    killer->setFocus();
    connect(killer, &InteractiveKill::killWindow, this, &ProcessPageWidget::popupKillConfirmDialog,
            Qt::QueuedConnection);
}

void ProcessPageWidget::dialogButtonClicked(int index, QString)
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

void ProcessPageWidget::popupKillConfirmDialog(int pid)
{
    killer->close();

    killPid = pid;
    killProcessDialog->show();
}

void ProcessPageWidget::recordSortingStatus(int index, bool sortingOrder)
{
    auto *settings = Settings::instance();
    Q_ASSERT(settings != nullptr);

    QList<QString> columnNames = {"name",      "cpu",      "memory", "disk_write",
                                  "disk_read", "download", "upload", "pid"};

    m_settings->setOption("process_sorting_column", columnNames[index]);
    m_settings->setOption("process_sorting_order", sortingOrder);
}

void ProcessPageWidget::recordVisibleColumn(int, bool, QList<bool> columnVisibles)
{
    auto *settings = Settings::instance();
    Q_ASSERT(settings != nullptr);

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

    m_settings->setOption("process_columns", processColumns);
}

void ProcessPageWidget::showWindowKiller()
{
    // Minimize window before show killer window.
    this->showMinimized();

    QTimer::singleShot(200, this, SLOT(createWindowKiller()));
}

void ProcessPageWidget::switchTab(int index)
{
    if (index == 0) {
        statusMonitor->switchToOnlyGui();
    } else if (index == 1) {
        statusMonitor->switchToOnlyMe();
    } else {
        statusMonitor->switchToAllProcess();
    }

    m_settings->setOption("process_tab_index", index);
}

void ProcessPageWidget::switchDisplayMode(DisplayMode mode)
{
    switch (mode) {
        case kDisplayModeExpand:
            statusMonitor->disableCompactMode();
            adjustDiskMoitor();

            break;
        case kDisplayModeCompact:
            statusMonitor->enableCompactMode();

            break;
    }
}

void ProcessPageWidget::adjustStatusBarWidth()
{
    QRect rect = QApplication::desktop()->screenGeometry();

    // Just change status monitor width when screen width is more than 1024.
    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    if (rect.width() * 0.2 > statusBarMaxWidth) {
        if (windowState() == Qt::WindowMaximized) {
            statusMonitor->setFixedWidth(static_cast<int>(rect.width() * 0.2));
        } else {
            statusMonitor->setFixedWidth(statusBarMaxWidth);
        }
    }
}

void ProcessPageWidget::adjustDiskMoitor()
{
    if (this->rect().height() > 830) {
        statusMonitor->showDiskMonitor();
    } else {
        statusMonitor->hideDiskMonitor();
    }
}
