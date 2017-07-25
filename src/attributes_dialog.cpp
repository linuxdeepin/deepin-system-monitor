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

#include "attributes_dialog.h"
#include "constant.h"
#include "utils.h"
#include <QDateTime>
#include <QDebug>
#include <QPainter>
#include <dthememanager.h>
#include <proc/readproc.h>
#include <proc/sysinfo.h>

DWIDGET_USE_NAMESPACE

using namespace Utils;
using namespace Dtk;

AttributesDialog::AttributesDialog(QWidget *parent, int processId) : DAbstractDialog(parent)
{
    pid = processId;
    
    setAttribute(Qt::WA_DeleteOnClose, true);
    
    setMinimumWidth(320);
    
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    nameLayout = new QHBoxLayout();
    nameLayout->setContentsMargins(0, 0, 0, 0);
    cmdlineLayout = new QHBoxLayout();
    cmdlineLayout->setContentsMargins(0, 0, 0, 0);
    startTimeLayout = new QHBoxLayout();
    startTimeLayout->setContentsMargins(0, 0, 0, 0);
    
    closeButton = new DWindowCloseButton;
    closeButton->setFixedSize(27, 23);
    connect(closeButton, &DWindowCloseButton::clicked, this, &DAbstractDialog::close);
    Dtk::Widget::DThemeManager::instance()->setTheme(closeButton, "light") ;

    iconLabel = new QLabel();
    
    titleLabel = new QLabel();
    titleLabel->setStyleSheet("QLabel { background-color : transparent; font-size: 14px; font-weight: 500; color : #303030; }");
    
    nameTitleLabel = new QLabel(QString("%1:").arg(tr("Process name")));
    nameTitleLabel->setStyleSheet("QLabel { background-color : transparent; color : #666666; }");
    nameTitleLabel->setFixedWidth(100);
    nameTitleLabel->setAlignment(Qt::AlignRight);
    
    nameLabel = new QLabel();
    nameLabel->setStyleSheet("QLabel { background-color : transparent; color : #000000; }");
    
    nameLayout->addWidget(nameTitleLabel);
    nameLayout->addWidget(nameLabel);
    nameLayout->addSpacing(20);

    cmdlineTitleLabel = new QLabel(QString("%1:").arg(tr("Command line")));
    cmdlineTitleLabel->setStyleSheet("QLabel { background-color : transparent; color : #666666; }");
    cmdlineTitleLabel->setFixedWidth(100);
    cmdlineTitleLabel->setAlignment(Qt::AlignRight);
    
    cmdlineLabel = new QLabel();
    cmdlineLabel->setStyleSheet("QLabel { background-color : transparent; color : #000000; }");
    cmdlineLabel->setWordWrap(true);
    cmdlineLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    
    cmdlineLayout->addWidget(cmdlineTitleLabel);
    cmdlineLayout->addWidget(cmdlineLabel);
    cmdlineLayout->addSpacing(20);

    startTimeTitleLabel = new QLabel(QString("%1:").arg(tr("Start time")));
    startTimeTitleLabel->setStyleSheet("QLabel { background-color : transparent; color : #666666; }");
    startTimeTitleLabel->setFixedWidth(100);
    startTimeTitleLabel->setAlignment(Qt::AlignRight);
    
    startTimeLabel = new QLabel();
    startTimeLabel->setStyleSheet("QLabel { background-color : transparent; color : #000000; }");
    startTimeLabel->setWordWrap(true);
    
    startTimeLayout->addWidget(startTimeTitleLabel);
    startTimeLayout->addWidget(startTimeLabel);
    startTimeLayout->addSpacing(20);
    
    layout->addWidget(closeButton, 0, Qt::AlignTop | Qt::AlignRight);
    layout->addSpacing(20);
    layout->addWidget(iconLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(14);
    layout->addWidget(titleLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(20);
    layout->addLayout(nameLayout);
    layout->addLayout(cmdlineLayout);
    layout->addLayout(startTimeLayout);
    layout->addSpacing(20);
    
    // Read the list of open processes information.
    PROCTAB* proc = openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLUSR | PROC_FILLCOM);
    static proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_t));

    std::map<int, proc_t> processes;
    while (readproc(proc, &proc_info) != NULL) {
        processes[proc_info.tid]=proc_info;
    }
    closeproc(proc);

    findWindowTitle = new FindWindowTitle();
    findWindowTitle->updateWindowInfos();
    
    for (auto &i:processes) {
        int processId = (&i.second)->tid;
        
        if (pid == processId) {
            QString cmdline = Utils::getProcessCmdline(processId);
            QString name = getProcessName(&i.second, cmdline);
            std::string desktopFile = getDesktopFileFromName(processId, name, cmdline);
            QPixmap icon;
            if (desktopFile.size() == 0) {
                icon = findWindowTitle->getWindowIcon(findWindowTitle->getWindow(pid), 96);
            } else {
                icon = getDesktopFileIcon(desktopFile, 96);
            }
            QString displayName = getDisplayNameFromName(name, desktopFile, false);
            
            iconLabel->setPixmap(icon);
            titleLabel->setText(displayName);
            nameLabel->setText(name);
            cmdlineLabel->setText(cmdline);
            
            startTimeLabel->setText(QFileInfo(QString("/proc/%1").arg(processId)).created().toString("yyyy-MM-dd hh:mm:ss"));
            
            break;
        }
    }
}

AttributesDialog::~AttributesDialog()
{
    delete findWindowTitle;
    delete closeButton;
    delete iconLabel;
    delete nameTitleLabel;
    delete nameLabel;
    delete titleLabel;
    delete cmdlineTitleLabel;
    delete startTimeLabel;
    delete startTimeTitleLabel;
    delete cmdlineLabel;
    delete nameLayout;
    delete cmdlineLayout;
    delete startTimeLayout;
    delete layout;
}

int AttributesDialog::getPid()
{
    return pid;
}

void AttributesDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QPainterPath path;
    path.addRect(QRectF(rect()));
    painter.setOpacity(1);
    painter.fillPath(path, QColor("#ffffff"));
}
