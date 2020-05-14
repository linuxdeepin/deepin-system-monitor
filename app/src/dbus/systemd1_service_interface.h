#ifndef SYSTEMD1_SERVICE_INTERFACE_H
#define SYSTEMD1_SERVICE_INTERFACE_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtDBus>

#include "dbus_common.h"
#include "dbus_properties_interface.h"
#include "environment_file.h"

using namespace DBus::Common;

/*
 * Proxy class for interface org.freedesktop.systemd1.Service
 */
class ErrorContext;

class Systemd1ServiceInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:
    static inline const char *staticInterfaceName() { return "org.freedesktop.systemd1.Service"; }

public:
    Systemd1ServiceInterface(const QString &service, const QString &path,
                             const QDBusConnection &connection, QObject *parent = nullptr);

    ~Systemd1ServiceInterface();

    QPair<ErrorContext, quint32> getMainPID() const;
    QPair<ErrorContext, quint64> getMemoryCurrent() const;
    QPair<ErrorContext, QString> getControlGroup() const;
    QPair<ErrorContext, EnvironmentFileList> getEnvironmentFiles() const;

public Q_SLOTS:  // METHODS

Q_SIGNALS:  // SIGNALS

private:
    DBusPropertiesInterface *m_DBusPropIface;
};

namespace org {
namespace freedesktop {
namespace systemd1 {
typedef ::Systemd1ServiceInterface Service;
}
}  // namespace freedesktop
}  // namespace org
#endif
