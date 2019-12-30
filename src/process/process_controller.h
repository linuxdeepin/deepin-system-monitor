#ifndef PROCESSCONTROLLER_H
#define PROCESSCONTROLLER_H

#include <QThread>

#include "common/error_context.h"

class ProcessController : public QThread
{
    Q_OBJECT

public:
    explicit ProcessController(pid_t pid, int signal, QObject *parent = nullptr);

Q_SIGNALS:
    void resultReady(int code);

protected:
    void run() override;

private:
    pid_t m_pid;
    int m_signal {0};
};

#endif  // PROCESSCONTROLLER_H
