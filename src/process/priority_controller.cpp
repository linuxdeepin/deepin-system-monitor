#include <DApplication>
#include <QFile>

#include "priority_controller.h"

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
