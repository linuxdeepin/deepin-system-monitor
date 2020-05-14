#ifndef PRIORITY_CONTROLLER_H
#define PRIORITY_CONTROLLER_H

#include <QProcess>
#include <QThread>

#include "common/error_context.h"

class PriorityController : public QThread
{
    Q_OBJECT

public:
    explicit PriorityController(pid_t pid, int priority, QObject *parent = nullptr);

Q_SIGNALS:
    void resultReady(int code);

protected:
    void run() override;

private:
    pid_t m_pid;
    int m_priority {0};
};

#endif  // PRIORITY_CONTROLLER_H
