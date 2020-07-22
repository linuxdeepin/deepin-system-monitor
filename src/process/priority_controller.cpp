﻿/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "priority_controller.h"

#include <DApplication>

#include <QProcess>
#include <QFile>

#define CMD_PKEXEC "/usr/bin/pkexec"
#define CMD_RENICE "/usr/bin/renice"

DWIDGET_USE_NAMESPACE

PriorityController::PriorityController(pid_t pid, int priority, QObject *parent)
    : QThread(parent)
    , m_pid(pid)
    , m_priority(priority)
{
}

void PriorityController::run()
{
    QProcess proc;
    QStringList params;
    int rc;
    ErrorContext ec {};

    // check bin existance
    if (!QFile::exists({CMD_PKEXEC})) {
        Q_EMIT resultReady(ENOENT);
        exit(ENOENT);
    }
    if (!QFile::exists({CMD_RENICE})) {
        Q_EMIT resultReady(ENOENT);
        exit(ENOENT);
    }

    // renice {priority} {pid}
    params << QString(CMD_RENICE) << QString("%1").arg(m_priority) << QString("%1").arg(m_pid);

    // -2: cant not be started; -1: crashed; other: exit code of pkexec
    // pkexec: 127: not auth/cant auth/error; 126: dialog dismiss
    rc = proc.execute({CMD_PKEXEC}, params);
    if (rc != 0) {
        if (rc == EPERM) {
            Q_EMIT resultReady(EPERM);
        } else if (rc == EACCES) {
            Q_EMIT resultReady(EACCES);
        } else {
            Q_EMIT resultReady(EPERM);
        }
    } else {
        Q_EMIT resultReady(0);
    }
}
