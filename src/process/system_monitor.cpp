#include <errno.h>
#include <proc/sysinfo.h>
#include <pwd.h>
#include <sched.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <DApplication>
#include <QDebug>
#include <QIcon>

#include "common/error_context.h"
#include "constant.h"
#include "priority_controller.h"
#include "process/process_entry.h"
#include "process_controller.h"
#include "system_monitor.h"
#include "utils.h"
#include "stats_collector.h"

DWIDGET_USE_NAMESPACE
using namespace Utils;

void SystemMonitor::startMonitorJob()
{
    m_workerThread.start();
}

void SystemMonitor::setFilterType(SystemMonitor::FilterType type)
{
    Q_EMIT filterTypeChanged(type);
}

void SystemMonitor::endProcess(pid_t pid)
{
    sendSignalToProcess(pid, SIGTERM);
}

void SystemMonitor::pauseProcess(pid_t pid)
{
    sendSignalToProcess(pid, SIGSTOP);
}

void SystemMonitor::resumeProcess(pid_t pid)
{
    sendSignalToProcess(pid, SIGCONT);
}

void SystemMonitor::killProcess(pid_t pid)
{
    sendSignalToProcess(pid, SIGKILL);
}

ErrorContext SystemMonitor::setProcessPriority(pid_t pid, int priority)
{
    sched_param param {};
    ErrorContext ec {};

    auto errfmt = [ = ](int err, ErrorContext & errorContext) -> ErrorContext & {
        errorContext.setCode(ErrorContext::kErrorTypeSystem);
        errorContext.setSubCode(err);
        errorContext.setErrorName(
            DApplication::translate("Process.Priority", "Failed to change process priority"));
        QString errmsg = QString("PID: %1, Error: [%2] %3").arg(pid).arg(err).arg(strerror(err));
        errorContext.setErrorMessage(errmsg);
        return errorContext;
    };

    errno = 0;
    int rc = sched_getparam(pid, &param);
    if (rc == -1) {
        return errfmt(errno, ec);
    }
    if (param.sched_priority == 0) {
        // dynamic priority
        if (priority > kVeryLowPriorityMin)
            priority = kVeryLowPriorityMin;
        else if (priority < kVeryHighPriorityMax)
            priority = kVeryHighPriorityMax;

        errno = 0;
        rc = setpriority(PRIO_PROCESS, id_t(pid), priority);
        if (rc == -1 && errno != 0) {
            if (errno == EACCES || errno == EPERM) {
                // call pkexec to change priority
                PriorityController *ctrl = new PriorityController(pid, priority, this);
                connect(ctrl, &PriorityController::resultReady, this, [ = ](int code) {
                    if (code == 0) {
                        Q_EMIT processPriorityChanged(pid, priority);
                    } else {
                        ErrorContext ec {};
                        ec.setCode(ErrorContext::kErrorTypeSystem);
                        ec.setSubCode(code);
                        ec.setErrorName(DApplication::translate("Process.Priority",
                                                                "Failed to change process priority"));
                        ec.setErrorMessage(
                            DApplication::translate("Process.Priority", "PID: %1, Error: [%2] %3")
                            .arg(pid)
                            .arg(code)
                            .arg(strerror(code)));
                        Q_EMIT priorityPromoteResultReady(ec);
                    }
                });
                connect(ctrl, &PriorityController::finished, ctrl, &QObject::deleteLater);
                ctrl->start();
                return {};
            } else {
                return errfmt(errno, ec);
            }
        } else {
            Q_EMIT processPriorityChanged(pid, priority);
            return ec;
        }
    } else {
        // static priority
        // TODO: do nothing at this moment, call sched_setparam to change static priority when
        // needed
    }
    return ec;
}

void SystemMonitor::sendSignalToProcess(pid_t pid, int signal)
{
    int rc = 0;
    ErrorContext ec = {};
    auto errfmt = [ = ](decltype(errno) err, const QString & title, int p, int sig,
    ErrorContext & ectx) -> ErrorContext & {
        ectx.setCode(ErrorContext::kErrorTypeSystem);
        ectx.setSubCode(err);
        ectx.setErrorName(title);
        QString errmsg = QString("PID: %1, Signal: [%2], Error: [%3] %4")
        .arg(p)
        .arg(sig)
        .arg(err)
        .arg(strerror(err));
        ectx.setErrorMessage(errmsg);
        return ectx;
    };
    auto emitSignal = [ = ](int signal) {
        if (signal == SIGTERM) {
            Q_EMIT processEnded(pid);
        } else if (signal == SIGSTOP) {
            Q_EMIT processPaused(pid, 'T');
        } else if (signal == SIGCONT) {
            Q_EMIT processResumed(pid, 'R');
        } else if (signal == SIGKILL) {
            Q_EMIT processKilled(pid);
        } else {
            qDebug() << "Unexpected signal in this case:" << signal;
        }
    };
    auto fmsg = [ = ](int signal) -> QString {
        if (signal == SIGTERM)
        {
            return DApplication::translate("Process.Signal", "Failed to end process");
        } else if (signal == SIGSTOP)
        {
            return DApplication::translate("Process.Signal", "Failed to pause process");
        } else if (signal == SIGCONT)
        {
            return DApplication::translate("Process.Signal", "Failed to resume process");
        } else if (signal == SIGKILL)
        {
            return DApplication::translate("Process.Signal", "Failed to kill process");
        } else
        {
            return DApplication::translate("Process.Signal", "Unknown error");
        }
    };
    auto pctl = [ = ](pid_t pid, int signal) {
        // call pkexec to promote
        auto *ctrl = new ProcessController(pid, signal, this);
        connect(ctrl, &ProcessController::resultReady, this,
        [this, errfmt, emitSignal, pid, signal, fmsg](int code) {
            if (code != 0) {
                ErrorContext ec = {};
                ec = errfmt(code, fmsg(signal), pid, signal, ec);
                Q_EMIT processControlResultReady(ec);
            } else {
                emitSignal(signal);
            }
        });
        connect(ctrl, &PriorityController::finished, ctrl, &QObject::deleteLater);
        ctrl->start();
    };
    auto pkill = [this, pctl, errfmt, emitSignal, fmsg](pid_t pid, int signal) {
        int rc = 0;
        errno = 0;
        ErrorContext ec = {};
        rc = kill(pid, signal);
        if (rc == -1 && errno != 0) {
            if (errno == EPERM) {
                pctl(pid, signal);
            } else {
                ec = errfmt(errno, fmsg(signal), pid, signal, ec);
                Q_EMIT processControlResultReady(ec);
                return;
            }
        } else {
            emitSignal(signal);
        }
    };

    if (signal == SIGTERM || signal == SIGKILL) {
        ec = {};
        errno = 0;
        // send SIGCONT first, otherwise signal will hang
        rc = kill(pid, SIGCONT);
        if (rc == -1 && errno != 0) {
            // not authorized, use pkexec instead
            if (errno == EPERM) {
                pctl(pid, signal);
            } else {
                ec = errfmt(
                         errno,
                         DApplication::translate("Process.Signal", "Failed in sending signal to process"),
                         pid, SIGCONT, ec);
                Q_EMIT processControlResultReady(ec);
                return;
            }
        } else {
            pkill(pid, signal);
        }
    } else {
        pkill(pid, signal);
    }
}

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<SystemMonitor::FilterType>();

    m_statsCollector = new StatsCollector();
    m_statsCollector->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::started, m_statsCollector, &StatsCollector::start);
    connect(&m_workerThread, &QThread::finished, m_statsCollector, &QObject::deleteLater);
    connect(this, &SystemMonitor::filterTypeChanged, m_statsCollector, &StatsCollector::setFilterType);
}

SystemMonitor::~SystemMonitor()
{
    m_workerThread.quit();
    m_workerThread.wait();
}
