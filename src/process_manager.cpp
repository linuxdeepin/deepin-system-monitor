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

#include "QVBoxLayout"
#include "attributes_dialog.h"
#include "dthememanager.h"
#include "list_view.h"
#include "process_item.h"
#include "process_manager.h"
#include <DDesktopServices>
#include <QApplication>
#include <QDebug>
#include <QList>
#include <QProcess>
#include <QStyleFactory>
#include <QToolTip>
#include <proc/sysinfo.h>
#include <signal.h>

DUTIL_USE_NAMESPACE

using namespace Utils;

ProcessManager::ProcessManager(int tabIndex, QList<bool> columnHideFlags, int sortingIndex, bool sortingOrder)
{
    // Init widget.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget *topWidget = new QWidget();
    topWidget->setFixedHeight(24);
    QHBoxLayout *topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(2, 0, 2, 0);
    
    processView = new ProcessView(columnHideFlags);
    connect(processView, &ListView::changeColumnVisible, this, &ProcessManager::changeColumnVisible);
    connect(processView, &ListView::changeSortingStatus, this, &ProcessManager::changeSortingStatus);
    
    layout->addWidget(topWidget);
    layout->addWidget(processView);

    statusLabel = new QLabel("");
    
    initTheme();
    
    connect(DThemeManager::instance(), &DThemeManager::themeChanged, this, &ProcessManager::changeTheme);
    
    processSwitchTab = new ProcessSwitchTab(tabIndex);
    
    connect(processSwitchTab, &ProcessSwitchTab::activeTab, this, &ProcessManager::activeTab);
    
    topLayout->addWidget(statusLabel, 0, Qt::AlignBottom);
    topLayout->addStretch();
    topLayout->addWidget(processSwitchTab, 0, Qt::AlignBottom);

    // Set sort algorithms.
    QList<SortAlgorithm> *alorithms = new QList<SortAlgorithm>();
    alorithms->append(&ProcessItem::sortByName);
    alorithms->append(&ProcessItem::sortByCPU);
    alorithms->append(&ProcessItem::sortByMemory);
    alorithms->append(&ProcessItem::sortByDiskWrite);
    alorithms->append(&ProcessItem::sortByDiskRead);
    alorithms->append(&ProcessItem::sortByNetworkDownload);
    alorithms->append(&ProcessItem::sortByNetworkUpload);
    alorithms->append(&ProcessItem::sortByPid);
    processView->setColumnSortingAlgorithms(alorithms, sortingIndex, sortingOrder);
    processView->setSearchAlgorithm(&ProcessItem::search);

    killProcessDialog = new DDialog(QString(tr("End process")), QString(tr("Ending this process may cause data loss.\nAre you sure to continue?")));
    killProcessDialog->setWindowFlags(killProcessDialog->windowFlags() | Qt::WindowStaysOnTopHint);
    killProcessDialog->setIcon(QIcon(Utils::getQrcPath("deepin-system-monitor.svg")));
    killProcessDialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
    killProcessDialog->addButton(QString(tr("End process")), true, DDialog::ButtonNormal);
    connect(killProcessDialog, &DDialog::buttonClicked, this, &ProcessManager::dialogButtonClicked);
    
    actionPids = new QList<int>();

    rightMenu = new QMenu();
    rightMenu->setStyle(QStyleFactory::create("dlight"));
    killAction = new QAction(tr("End process"), this);
    connect(killAction, &QAction::triggered, this, &ProcessManager::showKillProcessDialog);
    pauseAction = new QAction(tr("Suspend process"), this);
    connect(pauseAction, &QAction::triggered, this, &ProcessManager::stopProcesses);
    resumeAction = new QAction(tr("Resume process"), this);
    connect(resumeAction, &QAction::triggered, this, &ProcessManager::resumeProcesses);
    openDirectoryAction = new QAction(tr("View process location"), this);
    connect(openDirectoryAction, &QAction::triggered, this, &ProcessManager::openProcessDirectory);
    attributesAction = new QAction(tr("Properties"), this);
    connect(attributesAction, &QAction::triggered, this, &ProcessManager::showAttributes);
    rightMenu->addAction(killAction);
    rightMenu->addAction(pauseAction);
    rightMenu->addAction(resumeAction);
    rightMenu->addSeparator();
    rightMenu->addAction(openDirectoryAction);
    rightMenu->addSeparator();
    rightMenu->addAction(attributesAction);

    connect(processView, &ProcessView::rightClickItems, this, &ProcessManager::popupMenu, Qt::QueuedConnection);
    connect(processView, &ProcessView::changeHoverItem, this, &ProcessManager::changeHoverItem, Qt::QueuedConnection);
}

ProcessManager::~ProcessManager()
{
    delete killProcessDialog;
    delete processSwitchTab;
    delete processView;
    delete attributesAction;
    delete killAction;
    delete openDirectoryAction;
    delete pauseAction;
    delete resumeAction;
    delete statusLabel;
    delete actionPids;
    delete rightMenu;
}

void ProcessManager::initTheme()
{
    if (DThemeManager::instance()->theme() == "light") {
        statusLabel->setStyleSheet("QLabel { background-color : transparent; color : #505050; }");
    } else {
        statusLabel->setStyleSheet("QLabel { background-color : transparent; color : #666666; }");
    }
}

void ProcessManager::changeTheme(QString )
{
    initTheme();
}

ProcessView* ProcessManager::getProcessView()
{
    return processView;
}

void ProcessManager::dialogButtonClicked(int index, QString)
{
    if (index == 1) {
        killProcesses();
    }
}

void ProcessManager::focusProcessView()
{
    // Use QTimer to avoid user press Tab key to override setFocus.
    QTimer::singleShot(100, processView, SLOT(setFocus()));
}

void ProcessManager::handleSearch(QString searchContent)
{
    processView->search(searchContent);
}

void ProcessManager::changeHoverItem(QPoint, ListItem* item, int columnIndex)
{
    ProcessItem *processItem = static_cast<ProcessItem*>(item);
    
    if (columnIndex != 0 || processItem->isNameDisplayComplete()) {
        QToolTip::hideText();  
    } else {
        if (QToolTip::text() == processItem->getDisplayName()) {
            // QToolTip won't update position if tooltip text same as current one.
            // So we add blank char at end to force tooltip update position.
            QToolTip::showText(QCursor::pos(), processItem->getDisplayName() + " "); 
        } else {
            QToolTip::showText(QCursor::pos(), processItem->getDisplayName()); 
        }
    }
}

void ProcessManager::killProcesses()
{
    for (int pid : *actionPids) {
        // Resume process first, otherwise kill process too slow.
        kill(pid, SIGCONT);
        
        if (kill(pid, SIGKILL) != 0) {
            qDebug() << QString("Kill process %1 failed, permission denied.").arg(pid);
        }
    }

    actionPids->clear();
}

void ProcessManager::openProcessDirectory()
{
    for (int pid : *actionPids) {
        QString cmdline = Utils::getProcessCmdline(pid);
        if (cmdline.size() > 0) {
            // Found wine program location if cmdline starts with c://.
            if (cmdline.startsWith("c:\\")) {
                QString winePrefix = Utils::getProcessEnvironmentVariable(pid, "WINEPREFIX");
                cmdline = cmdline.replace("\\", "/").replace("c:/", "/drive_c/");
                
                DDesktopServices::showFileItem(winePrefix + cmdline);
            }
            // Else find program location through 'which' command.
            else {
                cmdline = cmdline.split(QRegExp("\\s")).at(0);
            
                QProcess whichProcess;
                QString exec = "which";
                QStringList params;
                params << cmdline;
                whichProcess.start(exec, params);
                whichProcess.waitForFinished();
                QString output(whichProcess.readAllStandardOutput());

                QString processPath = output.split("\n")[0];
                DDesktopServices::showFileItem(processPath);
            }
            
        }
    }

    actionPids->clear();
}

void ProcessManager::popupMenu(QPoint pos, QList<ListItem*> items)
{
    actionPids->clear();
    
    for (ListItem *item : items) {
        ProcessItem *processItem = static_cast<ProcessItem*>(item);
        actionPids->append(processItem->getPid());
    }
    rightMenu->exec(pos);
}

void ProcessManager::resumeProcesses()
{
    for (int pid : *actionPids) {
        if (kill(pid, SIGCONT) != 0) {
            qDebug() << QString("Resume process %1 failed, permission denied.").arg(pid);
        }
    }

    actionPids->clear();
}

void ProcessManager::showAttributes()
{
    for (int pid : *actionPids) {
        foreach (QWidget *widget, QApplication::topLevelWidgets()) {
            // Show attribute dialog if it has create, avoid create attribute dialog duplicate.
            if (qobject_cast<const AttributesDialog*>(widget) != 0) {
                AttributesDialog *dialog = qobject_cast<AttributesDialog*>(widget);
                if (dialog->getPid() == pid) {
                    dialog->show();
                    actionPids->clear();
                    
                    return;
                }
            }
        }
        
        AttributesDialog *dialog = new AttributesDialog(this, pid);
        dialog->show();
    }

    actionPids->clear();
}

void ProcessManager::showKillProcessDialog()
{
    killProcessDialog->exec();
}

void ProcessManager::stopProcesses()
{
    for (int pid : *actionPids) {
        if (kill(pid, SIGSTOP) != 0) {
            qDebug() << QString("Stop process %1 failed, permission denied.").arg(pid);
        }
    }

    actionPids->clear();
}

void ProcessManager::updateProcessNumber(QString tabName, int guiProcessNumber, int systemProcessNumber)
{
    statusLabel->setText((QString("%1 (") + tr("%2 applications and %3 processes are running") + ")").arg(tabName).arg(guiProcessNumber).arg(systemProcessNumber));
}

void ProcessManager::updateStatus(QList<ListItem*> items)
{
    processView->refreshItems(items);
}
