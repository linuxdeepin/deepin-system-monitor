// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process_controller.h"

#include "application.h"
#include "ddlog.h"

#include <QFile>
#include <QProcess>

#define CMD_PKEXEC "/usr/bin/pkexec"
#define CMD_KILL "/usr/bin/kill"

using namespace DDLog;

// constructor
ProcessController::ProcessController(pid_t pid, int signal, QObject *parent)
    : QObject(parent)
    , m_pid(pid)
    , m_signal(signal)
{
    qCDebug(app) << "ProcessController created for pid:" << m_pid << "with signal:" << m_signal;
    m_proc = new QProcess(this);
    // emit background task state changed signal when process about to start
    connect(m_proc, &QProcess::started, this, [=]() {
        qCDebug(app) << "QProcess started";
        Q_EMIT gApp->backgroundTaskStateChanged(Application::kTaskStarted);
    });
    // process finished signal
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](int rc, QProcess::ExitStatus) {
        qCDebug(app) << "pkexec kill finished with rc:" << rc;
        // EINVAL means call with invalid signal
        Q_ASSERT(rc != EINVAL);
        if (rc == ESRCH) {
            qCDebug(app) << "Process not found (ESRCH)";
            // binary non exists
            rc = ESRCH;
        } else if (rc != 0) {
            qCDebug(app) << "Permission denied (EPERM)";
            // permission denied
            rc = EPERM;
        }
        Q_EMIT resultReady(rc);
        // emit background task state changed signal when finished
        Q_EMIT gApp->backgroundTaskStateChanged(Application::kTaskFinished);
        m_proc->deleteLater();
        Q_EMIT finished();
    });
}

// execute pkexec+kill
void ProcessController::execute()
{
    qCDebug(app) << "Executing pkexec kill for pid:" << m_pid;
    QStringList params;

    // check pkexec existance
    if (!QFile::exists({CMD_PKEXEC})) {
        qCDebug(app) << "pkexec not found";
        Q_EMIT resultReady(ENOENT);
        exit(ENOENT);
    }
    // check kill existance
    if (!QFile::exists({CMD_KILL})) {
        qCDebug(app) << "kill not found";
        Q_EMIT resultReady(ENOENT);
        exit(ENOENT);
    }

    // format: kill -{signal} {pid}
    params << QString(CMD_KILL) << QString("-%1").arg(m_signal) << QString("%1").arg(m_pid);
    qCDebug(app) << "Starting process:" << CMD_PKEXEC << params;

    // EINVAL, EPERM, ESRCH
    // -2: cant not be started; -1: crashed; other: exit code of pkexec
    // pkexec: 127: not auth/cant auth/error; 126: dialog dismiss
    m_proc->start({CMD_PKEXEC}, params);
}
