// Copyright (C) 2019 ~ 2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ACCESSIBLE_POPUP_H
#define ACCESSIBLE_POPUP_H

#include "accessibledefine.h"

#include "gui/mainwindow.h"
#include "gui/cpu_widget.h"
#include "gui/disk_widget.h"
#include "gui/memory_widget.h"
#include "gui/net_widget.h"
#include "gui/process_widget.h"
#include "gui/process_table_view.h"
#include "gui/base/base_table_view.h"

#include <QWidget>
#include <QAccessible>
#include <QAccessibleWidget>

// Plugin-popup specific widgets
SET_FORM_ACCESSIBLE(MainWindow, "PluginPopupMainWindow")
SET_FORM_ACCESSIBLE(CpuWidget, "CpuWidget")
SET_FORM_ACCESSIBLE(DiskWidget, "DiskWidget")
SET_FORM_ACCESSIBLE(MemoryWidget, "MemoryWidget")
SET_FORM_ACCESSIBLE(NetWidget, "NetWidget")
SET_FORM_ACCESSIBLE(ProcessWidget, "ProcessWidget")
SET_FORM_ACCESSIBLE(ProcessTableView, "ProcessTableView")
SET_FORM_ACCESSIBLE(BaseTableView, "BaseTableView")

QAccessibleInterface *accessibleFactory(const QString &classname, QObject *object)
{
    QAccessibleInterface *interface = nullptr;

    USE_ACCESSIBLE(classname, MainWindow);
    USE_ACCESSIBLE(classname, CpuWidget);
    USE_ACCESSIBLE(classname, DiskWidget);
    USE_ACCESSIBLE(classname, MemoryWidget);
    USE_ACCESSIBLE(classname, NetWidget);
    USE_ACCESSIBLE(classname, ProcessWidget);
    USE_ACCESSIBLE(classname, ProcessTableView);
    USE_ACCESSIBLE(classname, BaseTableView);

    return interface;
}

#endif // ACCESSIBLE_POPUP_H