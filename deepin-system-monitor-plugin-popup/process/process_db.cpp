// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process_db.h"
#include "ddlog.h"

#include "wm/wm_window_list.h"
#include "process/desktop_entry_cache.h"
#include "process/process_icon.h"
#include "process/process_icon_cache.h"
#include "process/process_name.h"
#include "process/process_name_cache.h"
#include "process/process_controller.h"

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
    m_procSet = new ProcessSet();
    m_windowList = new WMWindowList();
    m_desktopEntryCache = new DesktopEntryCache();

    m_desktopEntryTimeCount = DesktopEntryTimeCount;
    m_euid = geteuid();
}

ProcessDB::~ProcessDB()
{
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
}

ProcessDB *ProcessDB::instance()
{
    auto *thread = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread);
    return thread->systemMonitorInstance()->processDB();
}

uid_t ProcessDB::processEuid()
{
    return m_euid;
}

ProcessSet *ProcessDB::processSet()
{
    return m_procSet;
}

WMWindowList *ProcessDB::windowList()
{
    return m_windowList;
}

DesktopEntryCache *ProcessDB::desktopEntryCache()
{
    return m_desktopEntryCache;
}

bool ProcessDB::isCurrentProcess(pid_t pid)
{
    pid_t cur = getpid();
    return pid == cur;
}

void ProcessDB::endProcess(pid_t pid)
{
    qCDebug(app) << "Ending process" << pid;
    sendSignalToProcess(pid, SIGTERM);
}

void ProcessDB::pauseProcess(pid_t pid)
{
    qCDebug(app) << "Pausing process" << pid;
    sendSignalToProcess(pid, SIGSTOP);
}

void ProcessDB::resumeProcess(pid_t pid)
{
    qCDebug(app) << "Resuming process" << pid;
    sendSignalToProcess(pid, SIGCONT);
}

void ProcessDB::killProcess(pid_t pid)
{
    qCDebug(app) << "Killing process" << pid;
    sendSignalToProcess(pid, SIGKILL);
}

void ProcessDB::update()
{
    if (m_desktopEntryTimeCount++ && m_desktopEntryTimeCount >= DesktopEntryTimeCount) {
        m_desktopEntryTimeCount = 0;
        m_desktopEntryCache->updateCache();
    }

    m_windowList->updateWindowListCache();
    m_procSet->refresh();
}

void ProcessDB::setProcessPriority(pid_t pid, int priority)
{
    qCDebug(app) << "Setting process" << pid << "priority to" << priority;
    emit signalProcessPrioritysetChanged(pid, priority);
}

void ProcessDB::sendSignalToProcess(pid_t pid, int signal)
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
        qCWarning(app) << "Error formatting:" << errmsg;
        return ectx;
    };
    auto emitSignal = [ = ](int signal) {
        if (signal == SIGTERM) {
            qCDebug(app) << "Emitting processEnded signal for PID" << pid;
            Q_EMIT processEnded(pid);
        } else if (signal == SIGSTOP) {
            qCDebug(app) << "Emitting processPaused signal for PID" << pid;
            Q_EMIT processPaused(pid, 'T');
        } else if (signal == SIGCONT) {
            qCDebug(app) << "Emitting processResumed signal for PID" << pid;
            Q_EMIT processResumed(pid, 'R');
        } else if (signal == SIGKILL) {
            qCDebug(app) << "Emitting processKilled signal for PID" << pid;
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
                qCWarning(app) << "ProcessController failed with code" << code;
                ErrorContext ec1 = {};
                ec1 = errfmt(code, fmsg(signal), pid, signal, ec1);
                Q_EMIT processControlResultReady(ec1);
            } else {
                qCDebug(app) << "ProcessController succeeded for PID" << pid;
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
        qCDebug(app) << "Attempting to kill process" << pid << "with signal" << signal;
        rc = kill(pid, signal);
        if (rc == -1 && errno != 0) {
            if (errno == EPERM) {
                qCDebug(app) << "Permission denied, using ProcessController for PID" << pid;
                pctl(pid, signal);
            } else {
                qCWarning(app) << "Failed to kill process" << pid << "with error:" << strerror(errno);
                ec1 = errfmt(errno, fmsg(signal), pid, signal, ec1);
                Q_EMIT processControlResultReady(ec1);
                return;
            }
        } else {
            qCDebug(app) << "Successfully sent signal" << signal << "to process" << pid;
            emitSignal(signal);
        }
    };

    if (signal == SIGTERM || signal == SIGKILL) {
        ec = {};
        errno = 0;
        int rc = 0;
        // send SIGCONT first, otherwise signal will hang
        qCDebug(app) << "Sending SIGCONT to process" << pid << "before" << signal;
        rc = kill(pid, SIGCONT);
        if (rc == -1 && errno != 0) {
            // not authorized, use pkexec instead
            if (errno == EPERM) {
                qCDebug(app) << "Permission denied for SIGCONT, using ProcessController";
                pctl(pid, signal);
            } else {
                qCWarning(app) << "Failed to send SIGCONT to process" << pid << "with error:" << strerror(errno);
                ec = errfmt(
                         errno,
                         QApplication::translate("Process.Signal", "Failed in sending signal to process"),
                         pid, SIGCONT, ec);
                Q_EMIT processControlResultReady(ec);
                return;
            }
        } else {
            qCDebug(app) << "Successfully sent SIGCONT to process" << pid;
            pkill(pid, signal);
        }
    } else {
        pkill(pid, signal);
    }
}

} // namespace process
} // namespace core
