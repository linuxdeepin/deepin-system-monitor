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
#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include <QList>
#include <mutex>
#include <thread>

#include "dbus/dbus_common.h"

class ErrorContext;
class ServiceManager;
class SystemServiceEntry;
class ServiceManagerWorker;

using namespace dbus::common;

// temporary solution to fix status column not shown final state after service start/stop/restart
class CustomTimer : public QObject
{
    Q_OBJECT

public:
    explicit CustomTimer(ServiceManager *mgr, QObject *parent = nullptr);

public:
    void start(const QString &path);

private:
    int __padding__ {0};
    int m_cnt {0};
    QTimer *m_timer {};
    ServiceManager *m_mgr {};
};

class ServiceManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ServiceManager)

public:
    inline static ServiceManager *instance()
    {
        ServiceManager *sin = m_instance.load();
        if (!sin) {
            std::lock_guard<std::mutex> lock(m_mutex);
            sin = m_instance.load();
            if (!sin) {
                sin = new ServiceManager();
                m_instance.store(sin);
            }
        }
        return sin;
    }

    void updateServiceList();

    inline static QString getServiceStartupType(const QString &id, const QString &state)
    {
        QString startupType {};
        if (isUnitNoOp(state) || id.endsWith("@")) {
            startupType = kServiceNAStartup;
        } else {
            bool b = isServiceAutoStartup(id, state);
            if (b) {
                startupType = kServiceAutoStartup;
            } else {
                startupType = kServiceManualStartup;
            }
        }
        return startupType;
    }

Q_SIGNALS:
    void errorOccurred(const ErrorContext &ec);
    void beginUpdateList();
    void serviceListUpdated(const QList<SystemServiceEntry> &list);
    void serviceStatusUpdated(const SystemServiceEntry &entry);

public:
    SystemServiceEntry updateServiceEntry(const QString &opath);
    QString normalizeServiceId(const QString &id, const QString &param = {});

public Q_SLOTS:
    ErrorContext startService(const QString &id, const QString &param = {});
    ErrorContext stopService(const QString &id);
    ErrorContext restartService(const QString &id, const QString &param = {});
    ErrorContext setServiceStartupMode(const QString &id, bool autoStart);

private:
    explicit ServiceManager(QObject *parent = nullptr);
    ~ServiceManager();

    QThread m_workerThread;
    ServiceManagerWorker *m_worker {};

    static std::atomic<ServiceManager *> m_instance;
    static std::mutex m_mutex;
};

#endif  // SERVICE_MANAGER_H
