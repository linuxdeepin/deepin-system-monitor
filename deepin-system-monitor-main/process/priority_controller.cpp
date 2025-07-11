// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "priority_controller.h"
#include "ddlog.h"

#include "application.h"

#include <QProcess>
#include <QFile>

#define CMD_PKEXEC "/usr/bin/pkexec"
#define CMD_RENICE "/usr/bin/renice"

using namespace DDLog;

// constructor
PriorityController::PriorityController(pid_t pid, int priority, QObject *parent)
    : QObject(parent)
    , m_pid(pid)
    , m_priority(priority)
{
    qCDebug(app) << "PriorityController created for pid:" << m_pid << "with priority:" << m_priority;
    m_proc = new QProcess(this);
    // connect process finished signal
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](int rc, QProcess::ExitStatus) {
        if (rc != 0) {
            qCDebug(app) << "renice finished with error, code:" << rc;
            if (rc == EPERM) {
                // Operation not permitted
                Q_EMIT resultReady(EPERM);
            } else if (rc == EACCES) {
                // Permission denied
                Q_EMIT resultReady(EACCES);
            } else {
                // Operation not permitted
                Q_EMIT resultReady(EPERM);
            }
        } else {
            qCDebug(app) << "renice finished successfully";
            // success
            Q_EMIT resultReady(0);
        }
        // emit background task finished signal
        Q_EMIT gApp->backgroundTaskStateChanged(Application::kTaskFinished);
        m_proc->deleteLater();
        Q_EMIT finished();
    });
    // watch on process state changed signal
    connect(m_proc, &QProcess::stateChanged, this, [=](QProcess::ProcessState state) {
        // process about to be started
        if (state == QProcess::Starting) {
            qCDebug(app) << "QProcess starting";
            Q_EMIT gApp->backgroundTaskStateChanged(Application::kTaskStarted);
        }
    });
}

// execute pkexec+renice
void PriorityController::execute()
{
    qCDebug(app) << "Executing renice for pid:" << m_pid;
    QStringList params;

    // check pkexec existance
    if (!QFile::exists({CMD_PKEXEC})) {
        qCDebug(app) << "pkexec not found";
        Q_EMIT resultReady(ENOENT);
        exit(ENOENT);
    }
    // check renice existance
    if (!QFile::exists({CMD_RENICE})) {
        qCDebug(app) << "renice not found";
        Q_EMIT resultReady(ENOENT);
        exit(ENOENT);
    }

    // format: renice {priority} {pid}
    params << QString(CMD_RENICE) << QString("%1").arg(m_priority) << QString("%1").arg(m_pid);
    qCDebug(app) << "Starting process:" << CMD_PKEXEC << params;

    // -2: cant not be started; -1: crashed; other: exit code of pkexec
    // pkexec: 127: not auth/cant auth/error; 126: dialog dismiss
    m_proc->start({CMD_PKEXEC}, params);
}
