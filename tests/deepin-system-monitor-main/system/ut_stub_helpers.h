// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Shared stub helpers for system module unit tests.
// Provides:
//   ut_helpers::ProcFiles  - fopen-based /proc file simulation (no real files touched)
//   ut_helpers::DBusEnv    - QDBusInterface environment simulation (isValid/doCall/property)
//   ut_helpers::makeDBusReply / makeDBusError - QDBusMessage crafting helpers
#ifndef UT_STUB_HELPERS_H
#define UT_STUB_HELPERS_H

#include <QByteArray>
#include <QDBusAbstractInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>

#include <dlfcn.h>
#include <cstdio>
#include <cstring>

#include "3rdparty/stub-ext/stubext.h"

namespace ut_helpers {

// ---------------------------------------------------------------------------
// qtFileOpenSymbol: resolves the real code address of
// QFile::open(QIODevice::OpenMode) via dlsym on the linked Qt core library.
// Taking &QFile::open in source yields a vcall index for this virtual
// function (pmf.ptr = 2*index+1), which the stub engine cannot patch; the
// symbol lookup sidesteps that.
// Returns nullptr when the symbol cannot be located.
// ---------------------------------------------------------------------------
inline void *qtFileOpenSymbol()
{
    // dlsym is immune to the fopen stub (AddrAny would fail here: it reads
    // /proc/self/maps via fopen, which ProcFiles has already patched).
    // Versioned mangled names: Qt6 uses QIODeviceBase, Qt5 uses QIODevice.
    void *p = dlsym(RTLD_DEFAULT, "_ZN5QFile4openE6QFlagsIN13QIODeviceBase12OpenModeFlagEE");
    if (!p)
        p = dlsym(RTLD_DEFAULT, "_ZN5QFile4openE6QFlagsIN9QIODevice12OpenModeFlagEE");
    return p;
}

// ---------------------------------------------------------------------------
// ProcFiles: stubs fopen() so that each registered path yields a FILE* over an
// in-memory buffer (fmemopen). Unknown paths return nullptr (simulates missing
// files). Tests never touch the real /proc file system.
// ---------------------------------------------------------------------------
class ProcFiles
{
public:
    void add(const QString &path, const QByteArray &content)
    {
        m_files.insert(path, content);
    }

    void clear() { m_files.clear(); }

    void install(stub_ext::StubExt &stub)
    {
        using FopenFn = FILE *(*)(const char *, const char *);
        auto *self = this;
        stub.set_lamda(static_cast<FopenFn>(&fopen),
                       [self](const char *path, const char *mode) -> FILE * {
                           Q_UNUSED(mode);
                           const QByteArray data = self->m_files.value(QString::fromLocal8Bit(path));
                           if (data.isEmpty()) {
                               return nullptr;
                           }
                           // NOTE: QByteArray::value() above treats empty content as
                           // "missing" as well; tests must use non-empty contents.
                           std::string *buf = &self->m_buffers[path];
                           buf->assign(data.constData(), size_t(data.size()));
                           return fmemopen(buf->data(), buf->size(), "r");
                       });
    }

private:
    QHash<QString, QByteArray> m_files;
    QHash<QString, std::string> m_buffers;
};

// ---------------------------------------------------------------------------
// DBusEnv: simulates the QDBusInterface world used by wireless.cpp and
// SysInfo::readSysInfoStatic (hostname1). All calls are answered in-process.
// ---------------------------------------------------------------------------
struct DBusState
{
    bool valid = true;                          // isValid() result
    QString hostname = "ut-test-host";          // hostname1 property
    bool nmGetDevicesOk = true;                 // GetDevices succeeds?
    QStringList devicePaths;                    // device object paths
    QString deviceInterface = "wlan0";          // device "Interface" property
    bool nmGetApsOk = true;                     // GetAccessPoints succeeds?
    QStringList apPaths;                        // AP object paths
    QString apSsid = "Deepin-WiFi";             // AP "Ssid" property
    uint apStrength = 77;                       // AP "Strength" property
    // per-call log (method name), for assertion on call sequence
    QStringList calledMethods;
};

inline QDBusMessage makeDBusReply(const QVariant &arg)
{
    QDBusMessage req = QDBusMessage::createMethodCall("ut.service", "/ut/obj",
                                                      "ut.iface", "ut.method");
    QDBusMessage reply = req.createReply();
    reply.setArguments(QList<QVariant>{ arg });
    return reply;
}

inline QDBusMessage makeDBusError(const QString &name)
{
    QDBusMessage req = QDBusMessage::createMethodCall("ut.service", "/ut/obj",
                                                      "ut.iface", "ut.method");
    QDBusMessage reply = req.createErrorReply(name, "unit-test injected error");
    return reply;
}

inline void installDBusStubs(stub_ext::StubExt &stub, DBusState &state)
{
    using IsValidFn = bool (QDBusAbstractInterface::*)() const;
    auto *self = &state;
    stub.set_lamda(static_cast<IsValidFn>(&QDBusAbstractInterface::isValid),
                   [self](const QDBusAbstractInterface *) -> bool {
                       return self->valid;
                   });

    using PropertyFn = QVariant (QObject::*)(const char *) const;
    stub.set_lamda(static_cast<PropertyFn>(&QObject::property),
                   [self](const QObject *, const char *name) -> QVariant {
                       const QString key = QString::fromLatin1(name ? name : "");
                       if (key == "Hostname")
                           return self->hostname;
                       if (key == "Interface")
                           return self->deviceInterface;
                       if (key == "Ssid")
                           return self->apSsid;
                       if (key == "Strength")
                           return self->apStrength;
                       return QVariant();
                   });

    using DoCallFn = QDBusMessage (QDBusAbstractInterface::*)(QDBus::CallMode,
                                                              const QString &,
                                                              const QVariant *,
                                                              size_t);
    stub.set_lamda(static_cast<DoCallFn>(&QDBusAbstractInterface::doCall),
                   [self](QDBusAbstractInterface *, QDBus::CallMode,
                          const QString &method, const QVariant *, size_t) -> QDBusMessage {
                       self->calledMethods.append(method);
                       if (method == "GetDevices") {
                           if (!self->nmGetDevicesOk)
                               return makeDBusError("org.freedesktop.DBus.Error.AccessDenied");
                           QList<QDBusObjectPath> pathList;
                           for (const QString &p : self->devicePaths)
                               pathList.append(QDBusObjectPath(p));
                           return makeDBusReply(QVariant::fromValue(pathList));
                       }
                       if (method == "GetAccessPoints") {
                           if (!self->nmGetApsOk)
                               return makeDBusError("org.freedesktop.DBus.Error.AccessDenied");
                           QList<QDBusObjectPath> pathList;
                           for (const QString &p : self->apPaths)
                               pathList.append(QDBusObjectPath(p));
                           return makeDBusReply(QVariant::fromValue(pathList));
                       }
                       return makeDBusError("org.freedesktop.DBus.Error.UnknownMethod");
                   });
}

} // namespace ut_helpers

#endif // UT_STUB_HELPERS_H
