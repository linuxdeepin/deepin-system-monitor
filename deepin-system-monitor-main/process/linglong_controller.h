// Copyright (C) 2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LINGLONG_CONTROLLER_H
#define LINGLONG_CONTROLLER_H

#include <QObject>

class QProcess;

/**
 * @brief Proxy class to execute ll-cli kill for linglong container processes
 *
 * Linglong applications run in isolated namespaces where the host's kill()
 * cannot reach them. This class wraps `ll-cli kill <appId> -s <signal>` via
 * QProcess to terminate linglong container processes.
 */
class LinglongController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Linglong controller constructor
     * @param appId Linglong application ID (e.g., "com.qq.im")
     * @param signal Signal to send (SIGTERM or SIGKILL)
     * @param parent Parent object
     */
    explicit LinglongController(const QString &appId, int signal, QObject *parent = nullptr);

    /**
     * @brief Execute ll-cli kill in a subprocess
     */
    void execute();

Q_SIGNALS:
    /**
     * @brief Process execute result ready signal
     * @param code Return code of the finished process
     */
    void resultReady(int code);
    /**
     * @brief Process finished signal
     */
    void finished();

private:
    QString m_appId;
    int m_signal {0};
    QProcess *m_proc;
};

#endif  // LINGLONG_CONTROLLER_H
