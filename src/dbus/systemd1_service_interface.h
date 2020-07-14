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

#ifndef SYSTEMD1_SERVICE_INTERFACE_H
#define SYSTEMD1_SERVICE_INTERFACE_H

#include "dbus_common.h"
#include "dbus_properties_interface.h"
#include "environment_file.h"

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtDBus>

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
