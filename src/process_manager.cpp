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

#include <DDesktopServices>
#include <QStyleFactory>
#include "QVBoxLayout"
#include "list_view.h"
#include "process_item.h"
#include "process_manager.h"
#include <QDebug>
#include <QProcess>
#include <QList>
#include <proc/sysinfo.h>
#include <signal.h>
#include "attributes_dialog.h"

using namespace Utils;
DUTIL_USE_NAMESPACE

ProcessManager::ProcessManager(QWidget *parent) : QWidget(parent)
{
    // Init widget.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget *topWidget = new QWidget();
    topWidget->setFixedHeight(24);
    QHBoxLayout *topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(2, 0, 2, 0);
    processView = new ProcessView();
    layout->addWidget(topWidget);
    layout->addWidget(processView);

    statusLabel = new QLabel("");
    statusLabel->setStyleSheet("QLabel { background-color : transparent; color : #666666; }");
    
    processSwitchTab = new ProcessSwitchTab();
    
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
    processView->setColumnSortingAlgorithms(alorithms, 1, true);
    processView->setSearchAlgorithm(&ProcessItem::search);

    killProcessDialog = new DDialog(QString("结束进程"), QString("结束进程会有丢失数据的风险\n您确定要结束选中的进程吗？"));
    killProcessDialog->setIcon(QIcon(Utils::getQrcPath("deepin-system-monitor.svg")));
    killProcessDialog->addButton(QString("取消"), false, DDialog::ButtonNormal);
    killProcessDialog->addButton(QString("结束进程"), true, DDialog::ButtonNormal);
    connect(killProcessDialog, &DDialog::buttonClicked, this, &ProcessManager::dialogButtonClicked);
    
    actionPids = new QList<int>();

    rightMenu = new QMenu();
    rightMenu->setStyle(QStyleFactory::create("dlight"));
    killAction = new QAction("结束进程", this);
    connect(killAction, &QAction::triggered, this, &ProcessManager::showKillProcessDialog);
    pauseAction = new QAction("暂停进程", this);
    connect(pauseAction, &QAction::triggered, this, &ProcessManager::stopProcesses);
    resumeAction = new QAction("继续进程", this);
    connect(resumeAction, &QAction::triggered, this, &ProcessManager::resumeProcesses);
    openDirectoryAction = new QAction("打开程序所在目录", this);
    connect(openDirectoryAction, &QAction::triggered, this, &ProcessManager::openProcessDirectory);
    attributesAction = new QAction("属性", this);
    connect(attributesAction, &QAction::triggered, this, &ProcessManager::showAttributes);
    rightMenu->addAction(killAction);
    rightMenu->addAction(pauseAction);
    rightMenu->addAction(resumeAction);
    rightMenu->addAction(openDirectoryAction);
    rightMenu->addAction(attributesAction);

    connect(processView, &ProcessView::rightClickItems, this, &ProcessManager::popupMenu, Qt::QueuedConnection);
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

void ProcessManager::dialogButtonClicked(int index, QString)
{
    if (index == 1) {
        killProcesses();
    }
}

void ProcessManager::handleSearch(QString searchContent)
{
    processView->search(searchContent);
}

void ProcessManager::killProcesses()
{
    for (int pid : *actionPids) {
        if (kill(pid, SIGTERM) != 0) {
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

    actionPids->clear();
}

void ProcessManager::popupMenu(QPoint pos, QList<ListItem*> items)
{
    for (ListItem *item : items) {
        ProcessItem *processItem = static_cast<ProcessItem*>(item);
        actionPids->append(processItem->getPid());
    }
    rightMenu->exec(this->mapToGlobal(pos));
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
        AttributesDialog *dialog = new AttributesDialog(pid);
        dialog->show();
    }

    actionPids->clear();
}

void ProcessManager::showKillProcessDialog()
{
    killProcessDialog->show();
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

void ProcessManager::updateProcessNumber(int guiProcessNumber, int systemProcessNumber)
{
    statusLabel->setText(QString("正在运行%1个应用进程和%2个系统进程").arg(guiProcessNumber).arg(systemProcessNumber));
}

void ProcessManager::updateStatus(QList<ListItem*> items)
{
    processView->refreshItems(items);
}
