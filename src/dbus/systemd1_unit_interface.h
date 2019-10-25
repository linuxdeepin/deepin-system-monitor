#ifndef SYSTEMD1_UNIT_INTERFACE_H
#define SYSTEMD1_UNIT_INTERFACE_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtDBus>

#include "dbus_properties_interface.h"

/*
 * Proxy class for interface org.freedesktop.systemd1.Unit
 */
class Systemd1UnitInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName() { return "org.freedesktop.systemd1.Unit"; }

public:
    Systemd1UnitInterface(const QString &service, const QString &path,
                          const QDBusConnection &connection, QObject *parent = nullptr);
    ~Systemd1UnitInterface();

    QPair<ErrorContext, QString> getId() const;
    QPair<ErrorContext, QString> getLoadState() const;
    QPair<ErrorContext, QString> getActiveState() const;
    QPair<ErrorContext, QString> getSubState() const;
    QPair<ErrorContext, QString> getDescription() const;
    QPair<ErrorContext, bool> canReload() const;
    QPair<ErrorContext, bool> canStart() const;
    QPair<ErrorContext, bool> canStop() const;

Q_SIGNALS:  // SIGNALS

private:
    DBusPropertiesInterface *m_DBusPropIface;
};

namespace org {
namespace freedesktop {
namespace systemd1 {
typedef ::Systemd1UnitInterface Unit;
}
}  // namespace freedesktop
}  // namespace org
#endif
