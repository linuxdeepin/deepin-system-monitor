#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include <QList>
#include <mutex>
#include <thread>

#include "dbus/dbus_properties_interface.h"
#include "dbus/systemd1_manager_interface.h"
#include "dbus/systemd1_service_interface.h"
#include "dbus/unit_info.h"
#include "service/system_service_entry_data.h"
#include "service/service_manager_worker.h"

class QDBusObjectPath;
class ErrorContext;

class ServiceManager;
// temporary solution to fix status column not shown final state after service start/stop/restart
class CustomTimer : public QObject
{
    Q_OBJECT

public:
    explicit CustomTimer(ServiceManager *mgr, QObject *parent = nullptr);

public:
    void start(const QString &path);

private:
    char __padding__[4];
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

public Q_SLOTS:
    void startService(const QString &id, const QString &param = {});
    void stopService(const QString &id);
    void restartService(const QString &id, const QString &param = {});
    void setServiceStartupMode(const QString &id, bool autoStart);

private:
    ServiceManager(QObject *parent = nullptr);
    ~ServiceManager();

    QString normalizeServiceId(const QString &id, const QString &param = {});

    QThread m_workerThread;
    ServiceManagerWorker *m_worker {};

    static std::atomic<ServiceManager *> m_instance;
    static std::mutex m_mutex;
};

#endif  // SERVICE_MANAGER_H
