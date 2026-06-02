// Copyright (C) 2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "linglong_controller.h"

#include "application.h"
#include "ddlog.h"

#include <QFile>
#include <QProcess>
#include <QRegularExpression>

#include <cerrno>
#include <csignal>

#define CMD_LL_CLI "/usr/bin/ll-cli"

using namespace DDLog;

LinglongController::LinglongController(const QString &appId, int signal, QObject *parent)
    : QObject(parent)
    , m_appId(appId)
    , m_signal(signal)
{
    qCDebug(app) << "LinglongController created for appId:" << m_appId << "with signal:" << m_signal;
    m_proc = new QProcess(this);
    connect(m_proc, &QProcess::started, this, [=]() {
        qCDebug(app) << "QProcess started";
        Q_EMIT gApp->backgroundTaskStateChanged(Application::kTaskStarted);
    });
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int rc, QProcess::ExitStatus) {
                qCDebug(app) << "ll-cli kill finished with rc:" << rc;
                if (rc != 0) {
                    QString stderrOutput = QString::fromLocal8Bit(m_proc->readAllStandardError()).trimmed();
                    qCWarning(app) << "ll-cli kill failed. appId:" << m_appId
                                   << "rc:" << rc << "stderr:" << stderrOutput;
                }
                Q_EMIT resultReady(rc);
                Q_EMIT gApp->backgroundTaskStateChanged(Application::kTaskFinished);
                m_proc->deleteLater();
                Q_EMIT finished();
            });
}

void LinglongController::execute()
{
    qCDebug(app) << "Executing ll-cli kill for appId:" << m_appId;

    // Validate appId format: only allow reverse domain characters
    static const QRegularExpression appIdRe("^[a-zA-Z0-9.-]+$");
    if (!appIdRe.match(m_appId).hasMatch()) {
        qCWarning(app) << "Invalid linglong appId format:" << m_appId;
        Q_EMIT resultReady(EINVAL);
        Q_EMIT finished();
        return;
    }

    if (!QFile::exists(CMD_LL_CLI)) {
        qCWarning(app) << "ll-cli not found at" << CMD_LL_CLI;
        Q_EMIT resultReady(ENOENT);
        Q_EMIT finished();
        return;
    }

    // ll-cli kill -s expects signal name (e.g., SIGTERM, SIGKILL), not number
    QString sigName = (m_signal == SIGKILL) ? "SIGKILL" : "SIGTERM";
    QStringList params;
    params << "kill" << "-s" << sigName << m_appId;
    qCDebug(app) << "Starting process:" << CMD_LL_CLI << params;

    m_proc->start(CMD_LL_CLI, params);
}
