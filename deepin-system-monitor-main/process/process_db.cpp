// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process_db.h"
#include "ddlog.h"

#include "wm/wm_window_list.h"
#include "desktop_entry_cache.h"
#include "process_icon.h"
#include "process_icon_cache.h"
#include "process_name.h"
#include "process_name_cache.h"
#include "process_controller.h"
#include "priority_controller.h"

#include <QReadLocker>
#include <QWriteLocker>
#include <QApplication>
#include <QDebug>

#include <sys/resource.h>

using namespace core::wm;
using namespace DDLog;

namespace core {
namespace process {

const int DesktopEntryTimeCount = 150; // 5 minutes interval
ProcessDB::ProcessDB(QObject *parent)
    : QObject(parent)
{
    qCDebug(app) << "ProcessDB object created";
    m_procSet = new ProcessSet();
    m_windowList = new WMWindowList();
    m_desktopEntryCache = new DesktopEntryCache();

    m_desktopEntryTimeCount = DesktopEntryTimeCount;

    m_euid = geteuid();
    connect(this, &ProcessDB::signalProcessPrioritysetChanged, this, &ProcessDB::onProcessPrioritysetChanged);
}

ProcessDB::~ProcessDB()
{
    qCDebug(app) << "ProcessDB object destroyed";
    if (m_procSet) {
        delete m_procSet;
        m_procSet = nullptr;
    }
    if (m_windowList) {
        delete m_windowList;
        m_windowList = nullptr;
    }
    if (m_desktopEntryCache) {
        delete m_desktopEntryCache;
        m_desktopEntryCache = nullptr;
    }
    qCDebug(app) << "ProcessDB object fully destroyed";
}

ProcessDB *ProcessDB::instance()
{
    // qCDebug(app) << "Getting ProcessDB instance";
    auto *thread = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread);
    return thread->systemMonitorInstance()->processDB();
}

uid_t ProcessDB::processEuid()
{
    qCDebug(app) << "Getting process EUID:" << m_euid;
    return m_euid;
}

ProcessSet *ProcessDB::processSet()
{
    qCDebug(app) << "Getting process set";
    return m_procSet;
}

WMWindowList *ProcessDB::windowList()
{
    qCDebug(app) << "Getting window list";
    return m_windowList;
}

DesktopEntryCache *ProcessDB::desktopEntryCache()
{
    qCDebug(app) << "Getting desktop entry cache";
    return m_desktopEntryCache;
}

bool ProcessDB::isCurrentProcess(pid_t pid)
{
    pid_t cur = getpid();
    bool isCurrent = (pid == cur);
    // qCDebug(app) << "Checking if PID" << pid << "is current process:" << isCurrent;
    return isCurrent;
}

void ProcessDB::endProcess(pid_t pid)
{
    qCDebug(app) << "Ending process with PID:" << pid;
    sendSignalToProcess(pid, SIGTERM);
}

void ProcessDB::pauseProcess(pid_t pid)
{
    qCDebug(app) << "Pausing process with PID:" << pid;
    sendSignalToProcess(pid, SIGSTOP);
}

void ProcessDB::resumeProcess(pid_t pid)
{
    qCDebug(app) << "Resuming process with PID:" << pid;
    sendSignalToProcess(pid, SIGCONT);
}

void ProcessDB::killProcess(pid_t pid)
{
    qCDebug(app) << "Killing process with PID:" << pid;
    sendSignalToProcess(pid, SIGKILL);
}

void ProcessDB::update()
{
    qCDebug(app) << "ProcessDB::update() called";
    if (m_desktopEntryTimeCount++ && m_desktopEntryTimeCount >= DesktopEntryTimeCount) {
        qCDebug(app) << "Updating desktop entry cache";
        m_desktopEntryTimeCount = 0;
        m_desktopEntryCache->updateCache();
    }

    m_windowList->updateWindowListCache();
    m_procSet->refresh();
}

void ProcessDB::setProcessPriority(pid_t pid, int priority)
{
    qCDebug(app) << "setProcessPriority called for pid" << pid << "with priority" << priority;
    emit signalProcessPrioritysetChanged(pid, priority);
}

void ProcessDB::onProcessPrioritysetChanged(pid_t pid, int priority)
{
    qCDebug(app) << "onProcessPrioritysetChanged called for pid" << pid << "with priority" << priority;
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
        qCWarning(app) << "Failed to get process scheduling parameters. PID:" << pid << "Error:" << strerror(errno);
        emit processControlResultReady(errfmt(errno, ec));
        return;
    }

    // we dont support adjust realtime sched process's priority
    if (param.sched_priority == 0) {
        qCDebug(app) << "Process has dynamic priority, attempting to change. PID:" << pid;
        // dynamic priority
        if (priority > kVeryLowPriorityMin)
            priority = kVeryLowPriorityMin;
        else if (priority < kVeryHighPriorityMax)
            priority = kVeryHighPriorityMax;

        errno = 0;
        rc = setpriority(PRIO_PROCESS, id_t(pid), priority);
        if (rc == -1 && errno != 0) {
            qCDebug(app) << "setpriority failed with errno" << errno;
            if (errno == EACCES || errno == EPERM) {
                qCInfo(app) << "Permission denied, attempting to change priority using pkexec. PID:" << pid;
                // call pkexec to change priority
                auto *ctrl = new PriorityController(pid, priority, this);
                connect(ctrl, &PriorityController::resultReady, this, [ = ](int code) {
                    if (code == 0) {
                        qCInfo(app) << "Successfully changed process priority using pkexec. PID:" << pid;
                        Q_EMIT processPriorityChanged(pid, priority);
                    } else {
                        qCWarning(app) << "Failed to change process priority using pkexec. PID:" << pid << "Error:" << strerror(code);
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
                return;
            } else {
                qCWarning(app) << "Failed to change process priority. PID:" << pid << "Error:" << strerror(errno);
                Q_EMIT processControlResultReady(errfmt(errno, ec));
            }
        } else {
            qCInfo(app) << "Successfully changed process priority. PID:" << pid;
            Q_EMIT processPriorityChanged(pid, priority);
            Q_EMIT processControlResultReady(ec);
        }
    } else {
        qCInfo(app) << "Process has static priority, skipping priority change. PID:" << pid;
        // static priority
        // TODO: do nothing at this moment, call sched_setparam to change static priority when
        // needed
    }
    Q_EMIT processControlResultReady(ec);
}

void ProcessDB::sendSignalToProcess(pid_t pid, int signal)
{
    qCDebug(app) << "sendSignalToProcess called for pid" << pid << "with signal" << signal;
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
            qCInfo(app) << "Process ended signal emitted for PID:" << pid;
            Q_EMIT processEnded(pid);
        } else if (signal == SIGSTOP) {
            qCInfo(app) << "Process paused signal emitted for PID:" << pid;
            Q_EMIT processPaused(pid, 'T');
        } else if (signal == SIGCONT) {
            qCInfo(app) << "Process resumed signal emitted for PID:" << pid;
            Q_EMIT processResumed(pid, 'R');
        } else if (signal == SIGKILL) {
            qCInfo(app) << "Process killed signal emitted for PID:" << pid;
            Q_EMIT processKilled(pid);
        } else {
            qCWarning(app) << "Unexpected signal in this case:" << signal;
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
                qCWarning(app) << "Failed to send signal using pkexec. PID:" << pid << "Signal:" << signal << "Error:" << strerror(code);
                ErrorContext ec1 = {};
                ec1 = errfmt(code, fmsg(signal), pid, signal, ec1);
                Q_EMIT processControlResultReady(ec1);
            } else {
                qCDebug(app) << "Successfully sent signal using pkexec. PID:" << pid << "Signal:" << signal;
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
        qCDebug(app) << "Calling kill for pid" << pid << "with signal" << signal;
        rc = kill(pid, signal);
        if (rc == -1 && errno != 0) {
            qCDebug(app) << "kill failed with errno" << errno;
            if (errno == EPERM) {
                qCInfo(app) << "Permission denied, using pkexec to send signal. PID:" << pid;
                pctl(pid, signal);
            } else {
                qCWarning(app) << "Failed to send signal. PID:" << pid << "Signal:" << signal << "Error:" << strerror(errno);
                ec1 = errfmt(errno, fmsg(signal), pid, signal, ec1);
                Q_EMIT processControlResultReady(ec1);
                return;
            }
        } else {
            qCDebug(app) << "Successfully sent signal. PID:" << pid << "Signal:" << signal;
            emitSignal(signal);
        }
    };

    if (signal == SIGTERM || signal == SIGKILL) {
        ec = {};
        errno = 0;
        int rc = 0;
        // send SIGCONT first, otherwise signal will hang
        qCInfo(app) << "Sending SIGCONT before" << signal << "to process" << pid;
        rc = kill(pid, SIGCONT);
        if (rc == -1 && errno != 0) {
            qCDebug(app) << "kill(SIGCONT) failed with errno" << errno;
            // not authorized, use pkexec instead
            if (errno == EPERM) {
                qCInfo(app) << "Permission denied for SIGCONT, using pkexec. PID:" << pid;
                pctl(pid, signal);
            } else {
                qCWarning(app) << "Failed to send SIGCONT. PID:" << pid << "Error:" << strerror(errno);
                ec = errfmt(
                         errno,
                         QApplication::translate("Process.Signal", "Failed in sending signal to process"),
                         pid, SIGCONT, ec);
                Q_EMIT processControlResultReady(ec);
                return;
            }
        } else {
            qCDebug(app) << "Successfully sent SIGCONT, proceeding with original signal. PID:" << pid;
            pkill(pid, signal);
        }
    } else {
        pkill(pid, signal);
    }
}

} // namespace process
} // namespace core
