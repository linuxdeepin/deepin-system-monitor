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

#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <proc/readproc.h>
#include <DLabel>
#include <QMap>
#include <QMenu>
#include <QPixmap>
#include <QString>
#include <QWidget>

#include "ddialog.h"
#include "process_switch_tab.h"
#include "process_view.h"

DWIDGET_USE_NAMESPACE

class ProcessManager : public QWidget
{
    Q_OBJECT

public:
    ProcessManager(int tabIndex, QList<bool> columnHideFlags, int sortingIndex, bool sortingOrder);
    ~ProcessManager();

    ProcessView *getProcessView();

signals:
    void activeTab(int index);
    void changeColumnVisible(int index, bool visible, QList<bool> columnVisibles);
    void changeSortingStatus(int index, bool sortingOrder);

public slots:
    void dialogButtonClicked(int index, QString buttonText);
    void focusProcessView();
    void handleSearch(QString searchContent);
    void changeHoverItem(QPoint pos, DSimpleListItem *item, int columnIndex);
    void killProcesses();
    void terminateProcess();
    void openProcessDirectory();
    void popupMenu(QPoint pos, QList<DSimpleListItem *> items);
    void resumeProcesses();
    void showAttributes();
    void showKillProcessDialog();
    void stopProcesses();
    void updateProcessNumber(QString tabName, int guiProcessNumber, int systemProcessNumber);
    void updateStatus(QList<DSimpleListItem *> items);

private:
    DDialog *killProcessDialog;
    ProcessSwitchTab *processSwitchTab;
    ProcessView *processView;
    DLabel *statusLabel;
    QList<int> *actionPids;

    DMenu *rightMenu;
    QAction *m_termProcAction;     // end process
    QAction *m_pauseProcAction;    // suspend process
    QAction *m_resumeProcAction;   // resume process
    QAction *m_openExecDirAction;  // open cmd dir
    QAction *m_showAttrAction;     // show attributes
    QAction *m_killProcAction;     // kill process
};

#endif
