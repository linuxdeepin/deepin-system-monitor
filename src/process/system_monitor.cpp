/*
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

#include "system_monitor.h"

#include "stats_collector.h"
#include "priority_controller.h"
#include "process/process_entry.h"
#include "process_controller.h"
#include "common/error_context.h"
#include "utils.h"

#include <QApplication>
#include <QDebug>
#include <QIcon>

#include <errno.h>
#include <sys/resource.h>
#include <signal.h>

std::atomic<SystemMonitor *> SystemMonitor::m_instance;
std::mutex SystemMonitor::m_mutex;

void SystemMonitor::startMonitorJob()
{
    m_workerThread.start();
    Q_EMIT monitorJobStarted(m_statsCollector);
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
            QApplication::translate("Process.Priority", "Failed to change process priority"));
        QString errmsg = QString("PID: %1, Error: [%2] %3").arg(pid).arg(err).arg(strerror(err));
        errorContext.setErrorMessage(errmsg);
        return errorContext;
    };

    errno = 0;
    int rc = sched_getparam(pid, &param);
    if (rc == -1) {
        return errfmt(errno, ec);
    }
    // we dont support adjust realtime sched process's priority
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
                auto *ctrl = new PriorityController(pid, priority, this);
                connect(ctrl, &PriorityController::resultReady, this, [ = ](int code) {
                    if (code == 0) {
                        Q_EMIT processPriorityChanged(pid, priority);
                    } else {
                        ErrorContext ec1 {};
                        ec1.setCode(ErrorContext::kErrorTypeSystem);
                        ec1.setSubCode(code);
                        ec1.setErrorName(QApplication::translate("Process.Priority",
                                                                "Failed to change process priority"));
                        ec1.setErrorMessage(
                            QApplication::translate("Process.Priority", "PID: %1, Error: [%2] %3")
                            .arg(pid)
                            .arg(code)
                            .arg(strerror(code)));
                        Q_EMIT priorityPromoteResultReady(ec);
                    }
                });
                connect(ctrl, &PriorityController::finished, ctrl, &QObject::deleteLater);
                ctrl->execute();
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
            return QApplication::translate("Process.Signal", "Failed to end process");
        } else if (signal == SIGSTOP)
        {
            return QApplication::translate("Process.Signal", "Failed to pause process");
        } else if (signal == SIGCONT)
        {
            return QApplication::translate("Process.Signal", "Failed to resume process");
        } else if (signal == SIGKILL)
        {
            return QApplication::translate("Process.Signal", "Failed to kill process");
        } else
        {
            return QApplication::translate("Process.Signal", "Unknown error");
        }
    };
    auto pctl = [ = ](pid_t pid, int signal) {
        // call pkexec to promote
        auto *ctrl = new ProcessController(pid, signal, this);
        connect(ctrl, &ProcessController::resultReady, this,
        [this, errfmt, emitSignal, pid, signal, fmsg](int code) {
            if (code != 0) {
                ErrorContext ec1 = {};
                ec1 = errfmt(code, fmsg(signal), pid, signal, ec1);
                Q_EMIT processControlResultReady(ec1);
            } else {
                emitSignal(signal);
            }
        });
        connect(ctrl, &ProcessController::finished, ctrl, &QObject::deleteLater);
        ctrl->execute();
    };
    auto pkill = [this, pctl, errfmt, emitSignal, fmsg](pid_t pid, int signal) {
        int rc = 0;
        errno = 0;
        ErrorContext ec1 = {};
        rc = kill(pid, signal);
        if (rc == -1 && errno != 0) {
            if (errno == EPERM) {
                pctl(pid, signal);
            } else {
                ec1 = errfmt(errno, fmsg(signal), pid, signal, ec1);
                Q_EMIT processControlResultReady(ec1);
                return;
            }
        } else {
            emitSignal(signal);
        }
    };

    if (signal == SIGTERM || signal == SIGKILL) {
        ec = {};
        errno = 0;
        int rc = 0;
        // send SIGCONT first, otherwise signal will hang
        rc = kill(pid, SIGCONT);
        if (rc == -1 && errno != 0) {
            // not authorized, use pkexec instead
            if (errno == EPERM) {
                pctl(pid, signal);
            } else {
                ec = errfmt(
                         errno,
                         QApplication::translate("Process.Signal", "Failed in sending signal to process"),
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
    qRegisterMetaType<SystemMonitor::FilterType>("SystemMonitor::FilterType");

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
