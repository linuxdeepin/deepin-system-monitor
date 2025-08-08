// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "application.h"

#include "common/thread_manager.h"
#include "system/system_monitor_thread.h"
#include "system/netif_monitor_thread.h"
#include "process/process_db.h"
#include "ddlog.h"

#include <QEvent>
#include <QMetaType>
#include <QDebug>
#include <QTimer>

using namespace common::core;
using namespace core::system;
using namespace DDLog;

Application::Application(int &argc, char **argv)
    : DApplication(argc, argv)
{
    qCDebug(app) << "Application constructor";
    qRegisterMetaType<Application::TaskState>("Application::TaskState");
    qRegisterMetaType<pid_t>("pid_t");
    qRegisterMetaType<ErrorContext>("ErrorContext");

    ThreadManager::instance()->attach(new SystemMonitorThread);
    ThreadManager::instance()->attach(new NetifMonitorThread);
}

Application::~Application()
{
    // qCDebug(app) << "Application destructor";
}

void Application::setCurrentTaskState(TaskState taskState)
{
    qCDebug(app) << "setCurrentTaskState with taskState:" << taskState;
    m_currentTaskStat = taskState;
}

Application::TaskState Application::getCurrentTaskState()
{
    qCDebug(app) << "getCurrentTaskState";
    return m_currentTaskStat;
}

bool Application::event(QEvent *event)
{
    if (event && event->type() == kMonitorStartEventType) {
        // qCDebug(app) << "kMonitorStartEventType";
        SystemMonitorThread *thread = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread);
        thread->start();
    } else if (event && event->type() == kNetifStartEventType) {
        // qCDebug(app) << "kNetifStartEventType";
        NetifMonitorThread *thread = ThreadManager::instance()->thread<NetifMonitorThread>(BaseThread::kNetifMonitorThread);
        thread->start();
    }
    return DApplication::event(event);
}

MonitorStartEvent::~MonitorStartEvent()
{
    // qCDebug(app) << "MonitorStartEvent destructor";
}

NetifStartEvent::~NetifStartEvent()
{
    // qCDebug(app) << "NetifStartEvent destructor";
}
