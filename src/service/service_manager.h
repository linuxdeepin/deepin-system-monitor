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

class QDBusObjectPath;
class ErrorContext;

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

    QPair<ErrorContext, QList<SystemServiceEntry>> getServiceEntryList() const;

Q_SIGNALS:
    void errorOccurred(const ErrorContext &ec);
    void serviceStartupModeChanged(const QString &service, const QString &state);

public Q_SLOTS:
    QPair<ErrorContext, bool> startService(SystemServiceEntry &entry, const QString &param = {});
    QPair<ErrorContext, bool> stopService(SystemServiceEntry &entry);
    QPair<ErrorContext, bool> restartService(SystemServiceEntry &entry, const QString &param = {});
    void setServiceStartupMode(const QString &service, bool autoStart);

private:
    ServiceManager(QObject *parent = nullptr);
    ~ServiceManager() = default;

    QString normalizeServiceId(const QString &id, const QString &param = {});
    void updateServiceEntry(SystemServiceEntry &entry, const QDBusObjectPath &o);

    static std::atomic<ServiceManager *> m_instance;
    static std::mutex m_mutex;
};

#endif  // SERVICE_MANAGER_H
