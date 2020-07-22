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

#ifndef DBUS_PROPERTIES_INTERFACE_H
#define DBUS_PROPERTIES_INTERFACE_H

#include "common/error_context.h"

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtDBus>

/*
 * Proxy class for interface org.freedesktop.DBus.Properties
 */
class DBusPropertiesInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName() { return "org.freedesktop.DBus.Properties"; }

public:
    DBusPropertiesInterface(const QString &service, const QString &path,
                            const QDBusConnection &connection, QObject *parent = nullptr);

    ~DBusPropertiesInterface();

public Q_SLOTS:  // METHODS
    // param: in0 service name
    // param: in1 property name
    inline QPair<ErrorContext, QDBusMessage> Get(const QString &in0, const QString &in1)
    {
        QList<QVariant> args;
        ErrorContext ec;

        args << QVariant::fromValue(in0) << QVariant::fromValue(in1);
        QDBusMessage msg = callWithArgumentList(QDBus::Block, QStringLiteral("Get"), args);
        if (lastError().isValid()) {
            ec.setCode(ErrorContext::kErrorTypeDBus);
            ec.setSubCode(lastError().type());
            ec.setErrorName(lastError().name());
            ec.setErrorMessage(lastError().message());
        }

        return {ec, msg};
    }

Q_SIGNALS:  // SIGNALS
    void PropertiesChanged(const QString &ifname, const QVariantMap &changedProperties,
                           const QStringList &invalidatedProperties);
};

namespace org {
namespace freedesktop {
namespace DBus {
typedef ::DBusPropertiesInterface Properties;
}
}  // namespace freedesktop
}  // namespace org
#endif
