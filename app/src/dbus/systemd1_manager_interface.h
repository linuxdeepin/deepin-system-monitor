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

    // =========================================================================
    // TODO: half baked!!
    // call callWithArgumentList directly without calling CheckAuthorization
    // (from org.freedesktop.PolicyKit1.Authority Interface) to auth the user
    // will always end up failing with an "interactive authentication needed" error.
    // considering sysv type services need a different way to enable/disable
    // (e.g. run /lib/systemd/systemd-sysv-install script as root).
    // for simplicity, we use pkexec+systemctl to do the job as now.
    // =========================================================================
    //
    // params: asbb
    // return: ba(sss)
    inline QPair<ErrorContext, EnableUnitFilesResult> EnableUnitFiles(const QStringList &nlist,
                                                                      bool runtime = false, bool force = true)
    {
        ErrorContext ec {};
        EnableUnitFilesResult result {};
        QList<QVariant> args;
        args << nlist << runtime << force;

        QDBusMessage reply = callWithArgumentList(QDBus::Block, "EnableUnitFiles", args);

        if (reply.type() == QDBusMessage::ErrorMessage) {
            ec.setCode(ErrorContext::kErrorTypeDBus);
            ec.setSubCode(lastError().type());
            ec.setErrorName(reply.errorName());
            ec.setErrorMessage(reply.errorMessage());
        } else {
            Q_ASSERT(reply.type() == QDBusMessage::ReplyMessage);
            if (reply.signature() == "ba(sss)") {
                // unfinished...
            }
        }

        return {ec, result};
    }

    // =========================================================================
    // TODO: half baked!!
    // call callWithArgumentList directly without calling CheckAuthorization
    // (from org.freedesktop.PolicyKit1.Authority Interface) to auth the user
    // will always end up failing with an "interactive authentication needed" error.
    // considering sysv type services need a different way to enable/disable
    // (e.g. run /lib/systemd/systemd-sysv-install script as root).
    // for simplicity, we use pkexec+systemctl to do the job as now.
    // =========================================================================
    //
    // params: asb
    // return: a(sss)
    inline QPair<ErrorContext, DisableUnitFilesResult> DisableUnitFiles(const QStringList &nlist,
                                                                        bool runtime = false)
    {
        ErrorContext ec;
        DisableUnitFilesResult result {};
        QList<QVariant> args;
        args << nlist << runtime;

        QDBusMessage reply = callWithArgumentList(QDBus::Block, "DisableUnitFiles", args);

        if (reply.type() == QDBusMessage::ErrorMessage) {
            ec.setCode(ErrorContext::kErrorTypeDBus);
            ec.setSubCode(lastError().type());
            ec.setErrorName(reply.errorName());
            ec.setErrorMessage(reply.errorMessage());
        } else {
            Q_ASSERT(reply.type() == QDBusMessage::ReplyMessage);
            if (reply.signature() == "a(sss)") {
                // unfinished...
            }
        }

        return {ec, result};
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
