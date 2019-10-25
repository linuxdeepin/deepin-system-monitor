#ifndef SYSTEMD1_MANAGER_INTERFACE_H
#define SYSTEMD1_MANAGER_INTERFACE_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtDBus>

#include "common/error_context.h"
#include "dbus_common.h"
#include "unit_file_info.h"
#include "unit_info.h"

using namespace DBus::Common;

/*
 * Proxy class for interface org.freedesktop.systemd1.Manager
 */
class Systemd1ManagerInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName() { return "org.freedesktop.systemd1.Manager"; }

public:
    Systemd1ManagerInterface(const QString &service, const QString &path,
                             const QDBusConnection &connection, QObject *parent = nullptr);

    ~Systemd1ManagerInterface();

public Q_SLOTS:  // METHODS
    inline QPair<ErrorContext, UnitFileInfoList> ListUnitFiles()
    {
        UnitFileInfoList list;
        QList<QVariant> args;
        ErrorContext ec;

        QDBusMessage reply = callWithArgumentList(QDBus::Block, "ListUnitFiles", args);

        if (reply.type() == QDBusMessage::ErrorMessage) {
            ec.setCode(ErrorContext::kErrorTypeDBus);
            ec.setSubCode(lastError().type());
            ec.setErrorName(reply.errorName());
            ec.setErrorMessage(reply.errorMessage());
        } else {
            Q_ASSERT(reply.type() == QDBusMessage::ReplyMessage);
            if (reply.signature() == "a(ss)") {
                qvariant_cast<QDBusArgument>(reply.arguments()[0]) >> list;
            }
        }

        return {ec, list};
    }

    inline QPair<ErrorContext, UnitInfoList> ListUnits()
    {
        UnitInfoList list;
        QList<QVariant> args;
        ErrorContext ec;

        QDBusMessage reply = callWithArgumentList(QDBus::Block, "ListUnits", args);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            ec.setCode(ErrorContext::kErrorTypeDBus);
            ec.setSubCode(lastError().type());
            ec.setErrorName(reply.errorName());
            ec.setErrorMessage(reply.errorMessage());
        } else {
            Q_ASSERT(reply.type() == QDBusMessage::ReplyMessage);
            if (reply.signature() == "a(ssssssouso)") {
                qvariant_cast<QDBusArgument>(reply.arguments()[0]) >> list;
            }
        }

        return {ec, list};
    }

    inline QPair<ErrorContext, QDBusObjectPath> GetUnit(const QString &path)
    {
        ErrorContext ec;
        QDBusObjectPath o;
        QList<QVariant> args;
        args << path;

        QDBusMessage reply = callWithArgumentList(QDBus::Block, "GetUnit", args);

        if (reply.type() == QDBusMessage::ErrorMessage) {
            ec.setCode(ErrorContext::kErrorTypeDBus);
            ec.setSubCode(lastError().type());
            ec.setErrorName(reply.errorName());
            ec.setErrorMessage(reply.errorMessage());
        } else {
            Q_ASSERT(reply.type() == QDBusMessage::ReplyMessage);
            if (reply.signature() == "o") {
                o = qvariant_cast<QDBusObjectPath>(reply.arguments()[0]);
            }
        }

        return {ec, o};
    }

    inline QPair<ErrorContext, QDBusObjectPath> StartUnit(const QString &name, const QString &mode)
    {
        ErrorContext ec;
        QDBusObjectPath o;
        QList<QVariant> args;
        args << name << mode;

        QDBusMessage reply = callWithArgumentList(QDBus::Block, "StartUnit", args);

        if (reply.type() == QDBusMessage::ErrorMessage) {
            ec.setCode(ErrorContext::kErrorTypeDBus);
            ec.setSubCode(lastError().type());
            ec.setErrorName(reply.errorName());
            ec.setErrorMessage(reply.errorMessage());
        } else {
            Q_ASSERT(reply.type() == QDBusMessage::ReplyMessage);
            if (reply.signature() == "o") {
                o = qvariant_cast<QDBusObjectPath>(reply.arguments()[0]);
            }
        }

        return {ec, o};
    }

    inline QPair<ErrorContext, QDBusObjectPath> StopUnit(const QString &name, const QString &mode)
    {
        ErrorContext ec;
        QDBusObjectPath o;
        QList<QVariant> args;
        args << name << mode;

        QDBusMessage reply = callWithArgumentList(QDBus::Block, "StopUnit", args);

        if (reply.type() == QDBusMessage::ErrorMessage) {
            ec.setCode(ErrorContext::kErrorTypeDBus);
            ec.setSubCode(lastError().type());
            ec.setErrorName(reply.errorName());
            ec.setErrorMessage(reply.errorMessage());
        } else {
            Q_ASSERT(reply.type() == QDBusMessage::ReplyMessage);
            if (reply.signature() == "o") {
                o = qvariant_cast<QDBusObjectPath>(reply.arguments()[0]);
            }
        }

        return {ec, o};
    }

    inline QPair<ErrorContext, QDBusObjectPath> RestartUnit(const QString &name,
                                                            const QString &mode)
    {
        ErrorContext ec;
        QDBusObjectPath o;
        QList<QVariant> args;
        args << name << mode;

        QDBusMessage reply = callWithArgumentList(QDBus::Block, "RestartUnit", args);

        if (reply.type() == QDBusMessage::ErrorMessage) {
            ec.setCode(ErrorContext::kErrorTypeDBus);
            ec.setSubCode(lastError().type());
            ec.setErrorName(reply.errorName());
            ec.setErrorMessage(reply.errorMessage());
        } else {
            Q_ASSERT(reply.type() == QDBusMessage::ReplyMessage);
            if (reply.signature() == "o") {
                o = qvariant_cast<QDBusObjectPath>(reply.arguments()[0]);
            }
        }

        return {ec, o};
    }

    inline QPair<ErrorContext, QString> GetUnitFileState(const QString &unit)
    {
        ErrorContext ec;
        QString state;
        QList<QVariant> args;
        args << unit;

        QDBusMessage reply = callWithArgumentList(QDBus::Block, "GetUnitFileState", args);

        if (reply.type() == QDBusMessage::ErrorMessage) {
            ec.setCode(ErrorContext::kErrorTypeDBus);
            ec.setSubCode(lastError().type());
            ec.setErrorName(reply.errorName());
            ec.setErrorMessage(reply.errorMessage());
        } else {
            Q_ASSERT(reply.type() == QDBusMessage::ReplyMessage);
            if (reply.signature() == "s") {
                state = qvariant_cast<QString>(reply.arguments()[0]);
            }
        }

        return {ec, state};
    }

Q_SIGNALS:  // SIGNALS
};

namespace org {
namespace freedesktop {
namespace systemd1 {
typedef ::Systemd1ManagerInterface Manager;
}
}  // namespace freedesktop
}  // namespace org
#endif
