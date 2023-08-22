// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBUSDOCK_H
#define DBUSDOCK_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

#ifdef OS_BUILD_V23
const QString DockService = "org.deepin.dde.Dock1";
const QString DockPath = "/org/deepin/dde/Dock1";
const QString DockInterface = "org.freedesktop.DBus.Properties";
#else
const QString DockService = "com.deepin.dde.Dock";
const QString DockPath = "/com/deepin/dde/Dock";
const QString DockInterface = "org.freedesktop.DBus.Properties";
#endif

/*
 * Proxy class for interface local.MainWindow
 */
class DBusDockInterface: public QDBusAbstractInterface
{
    Q_OBJECT

    Q_SLOT void __propertyChanged__(const QDBusMessage& msg)
    {
        QList<QVariant> arguments = msg.arguments();
        if (3 != arguments.count())
            return;
        QString interfaceName = msg.arguments().at(0).toString();
        if (interfaceName != DockService)
            return;
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        foreach(const QString &prop, changedProps.keys()) {
        const QMetaObject* self = metaObject();
            for (int i=self->propertyOffset(); i < self->propertyCount(); ++i) {
                QMetaProperty p = self->property(i);
                if (p.name() == prop) {
                Q_EMIT p.notifySignal().invoke(this);
                }
            }
        }
   }
public:
    static inline const char *staticInterfaceName()
    { return DockService.toStdString().c_str(); }

public:
    explicit DBusDockInterface( QObject *parent = nullptr);
    ~DBusDockInterface();
    Q_PROPERTY(QRect geometry READ geometry)
    inline QRect geometry() const
    {
        return qvariant_cast<QRect>(property("geometry"));
    }
Q_SIGNALS: // SIGNALS
    void geometryChanged(const QRect &rect);
};
#endif
