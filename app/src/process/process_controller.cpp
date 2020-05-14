#include <DApplication>
#include <QFile>
#include <QProcess>

#include "process_controller.h"

#define CMD_PKEXEC "/usr/bin/pkexec"
#define CMD_KILL "/usr/bin/kill"

DWIDGET_USE_NAMESPACE

ProcessController::ProcessController(pid_t pid, int signal, QObject *parent)
    : QThread(parent)
    , m_pid(pid)
    , m_signal(signal)
{
}

void ProcessController::run()
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
    if (!QFile::exists({CMD_KILL})) {
        Q_EMIT resultReady(ENOENT);
        exit(ENOENT);
    }

    // kill -{signal} {pid}
    params << QString(CMD_KILL) << QString("-%1").arg(m_signal) << QString("%1").arg(m_pid);

    // EINVAL, EPERM, ESRCH
    // -2: cant not be started; -1: crashed; other: exit code of pkexec
    // pkexec: 127: not auth/cant auth/error; 126: dialog dismiss
    rc = proc.execute({CMD_PKEXEC}, params);
    // EINVAL means call with invalid signal
    Q_ASSERT(rc != EINVAL);
    if (rc == ESRCH)
        rc = ESRCH;
    else if (rc != 0) {
        rc = EPERM;
    }
    Q_EMIT resultReady(rc);
}
