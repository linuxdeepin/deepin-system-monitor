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
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "application.h"

#include "common/thread_manager.h"
#include "system/system_monitor_thread.h"
#include "process/process_db.h"

#include <QEvent>
#include <QMetaType>
#include <QDebug>
#include <QTimer>

using namespace common::core;
using namespace core::system;

Application::Application(int &argc, char **argv)
    : DApplication(argc, argv)
{
    qRegisterMetaType<Application::TaskState>("Application::BackgroundTaskState");

    ThreadManager::instance()->attach(new SystemMonitorThread);
}

bool Application::event(QEvent *event)
{
    if (event && event->type() == kMonitorStartEventType) {
        SystemMonitorThread *thread = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread);
        thread->start();
    }

    return DApplication::event(event);
}

MonitorStartEvent::~MonitorStartEvent()
{
}
