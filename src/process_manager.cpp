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

#include <proc/sysinfo.h>
#include <signal.h>

#include <DApplication>
#include <DDesktopServices>
#include <DSimpleListView>
#include <QDebug>
#include <QDir>
#include <QList>
#include <QProcess>
#include <QStyleFactory>
#include <QToolTip>
#include <QVBoxLayout>

#include "attributes_dialog.h"
#include "dthememanager.h"
#include "process_item.h"
#include "process_manager.h"

DCORE_USE_NAMESPACE

using namespace Utils;

ProcessManager::ProcessManager(int tabIndex, QList<bool> columnHideFlags, int sortingIndex,
                               bool sortingOrder)
{
    // Init widget.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget *topWidget = new QWidget();
    topWidget->setFixedHeight(24);
    QHBoxLayout *topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(2, 0, 2, 0);

    processView = new ProcessView(columnHideFlags);
    connect(processView, &DSimpleListView::changeColumnVisible, this,
            &ProcessManager::changeColumnVisible);
    connect(processView, &DSimpleListView::changeSortingStatus, this,
            &ProcessManager::changeSortingStatus);

    layout->addWidget(topWidget);
    layout->addWidget(processView);

    statusLabel = new QLabel("", this);

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

    killProcessDialog = new DDialog(
        DApplication::translate("Kill.Process.Dialog", "End process"),
        DApplication::translate(
            "Kill.Process.Dialog",
            "Ending this process may cause data loss.\nAre you sure you want to continue?"),
        this);
    killProcessDialog->setWindowFlags(killProcessDialog->windowFlags() | Qt::WindowStaysOnTopHint);
    killProcessDialog->setIconPixmap(QPixmap(Utils::getQrcPath("deepin-system-monitor.svg")));
    killProcessDialog->addButton(DApplication::translate("Kill.Process.Dialog", "Cancel"), false);
    killProcessDialog->addButton(DApplication::translate("Kill.Process.Dialog", "Terminate"), true);
    connect(killProcessDialog, &DDialog::buttonClicked, this, &ProcessManager::dialogButtonClicked);

    actionPids = new QList<int>();

    rightMenu = new DMenu(this);

    m_termProcAction = new QAction(
        DApplication::translate("Process.Table.Context.Menu", "Terminate process"), rightMenu);
    connect(m_termProcAction, &QAction::triggered, this, &ProcessManager::terminateProcess);
    m_pauseProcAction = new QAction(
        DApplication::translate("Process.Table.Context.Menu", "Suspend process"), rightMenu);
    connect(m_pauseProcAction, &QAction::triggered, this, &ProcessManager::stopProcesses);
    m_resumeProcAction = new QAction(
        DApplication::translate("Process.Table.Context.Menu", "Resume process"), rightMenu);
    connect(m_resumeProcAction, &QAction::triggered, this, &ProcessManager::resumeProcesses);
    m_openExecDirAction = new QAction(
        DApplication::translate("Process.Table.Context.Menu", "View command location"), rightMenu);
    connect(m_openExecDirAction, &QAction::triggered, this, &ProcessManager::openProcessDirectory);
    m_showAttrAction =
        new QAction(DApplication::translate("Process.Table.Context.Menu", "Properties"), rightMenu);
    connect(m_showAttrAction, &QAction::triggered, this, &ProcessManager::showAttributes);
    m_killProcAction = new QAction(
        DApplication::translate("Process.Table.Context.Menu", "Force terminate process"),
        rightMenu);
    connect(m_killProcAction, &QAction::triggered, this, &ProcessManager::showKillProcessDialog);

    rightMenu->addAction(m_termProcAction);
    rightMenu->addSeparator();
    rightMenu->addAction(m_pauseProcAction);
    rightMenu->addAction(m_resumeProcAction);
    rightMenu->addAction(m_openExecDirAction);
    rightMenu->addAction(m_showAttrAction);
    rightMenu->addSeparator();
    rightMenu->addAction(m_killProcAction);

    connect(processView, &ProcessView::rightClickItems, this, &ProcessManager::popupMenu,
            Qt::QueuedConnection);
    connect(processView, &ProcessView::changeHoverItem, this, &ProcessManager::changeHoverItem,
            Qt::QueuedConnection);
}

ProcessManager::~ProcessManager()
{
    killProcessDialog->deleteLater();
    processSwitchTab->deleteLater();
    processView->deleteLater();
    delete actionPids;
}

ProcessView *ProcessManager::getProcessView()
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

void ProcessManager::changeHoverItem(QPoint, DSimpleListItem *item, int columnIndex)
{
    ProcessItem *processItem = static_cast<ProcessItem *>(item);
    if (!processItem)
        return;

    if (columnIndex != 0 || processItem->isNameDisplayComplete()) {
        QWidgetList qwl = QApplication::topLevelWidgets();
        for (QWidget *qw : qwl) {
            if (qw->metaObject()->className() == QStringLiteral("QTipLabel")) {
                qw->close();
            }
        }
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

void ProcessManager::terminateProcess()
{
    for (int pid : *actionPids) {
        // Resume process first, otherwise kill process too slow.
        kill(pid, SIGCONT);

        if (kill(pid, SIGTERM) != 0) {
            qDebug() << QString("Kill process %1 failed, ERRNO:[%2] %3")
                            .arg(pid)
                            .arg(errno)
                            .arg(strerror(errno));
        }
    }

    actionPids->clear();
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
            } else {
                QString flatpakAppidEnv =
                    Utils::getProcessEnvironmentVariable(pid, "FLATPAK_APPID");

                // Else find program location through 'which' command.
                if (flatpakAppidEnv == "") {
                    QProcess whichProcess;
                    QString exec = "which";
                    QStringList params;
                    params << cmdline.split(" ");
                    whichProcess.start(exec, params);
                    whichProcess.waitForFinished();
                    QString output(whichProcess.readAllStandardOutput());

                    QString processPath = output.split("\n")[0];
                    DDesktopServices::showFileItem(processPath);
                }
                // Find flatpak application location.
                else {
                    QDir flatpakRootDir = Utils::getFlatpakAppPath(flatpakAppidEnv);
                    flatpakRootDir.cd("files");
                    flatpakRootDir.cd("bin");

                    // Need split full path to get last filename.
                    DDesktopServices::showFileItem(
                        flatpakRootDir.absoluteFilePath(cmdline.split("/").last()));
                }
            }
        }
    }

    actionPids->clear();
}

void ProcessManager::popupMenu(QPoint pos, QList<DSimpleListItem *> items)
{
    actionPids->clear();

    for (DSimpleListItem *item : items) {
        ProcessItem *processItem = static_cast<ProcessItem *>(item);
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
            if (qobject_cast<const AttributesDialog *>(widget) != nullptr) {
                AttributesDialog *dialog = qobject_cast<AttributesDialog *>(widget);
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
    int currentPid = getpid();

    for (int pid : *actionPids) {
        if (pid != currentPid) {
            if (kill(pid, SIGSTOP) != 0) {
                qDebug() << QString("Stop process %1 failed, permission denied.").arg(pid);
            }
        }
    }

    actionPids->clear();
}

void ProcessManager::updateProcessNumber(QString tabName, int guiProcessNumber,
                                         int systemProcessNumber)
{
    statusLabel->setText((QString("%1 (") +
                          DApplication::translate("Process.Summary",
                                                  "%2 applications and %3 processes are running") +
                          ")")
                             .arg(tabName)
                             .arg(guiProcessNumber)
                             .arg(systemProcessNumber));
}

void ProcessManager::updateStatus(QList<DSimpleListItem *> items)
{
    processView->refreshItems(items);
}
