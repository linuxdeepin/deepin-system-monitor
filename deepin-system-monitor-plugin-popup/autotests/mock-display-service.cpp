// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// dsm-popup-mock-service:popup 单测专用 Mock 服务进程
//
// 独立进程的原因:PrimaryRect 属性(4×qint16 结构)必须注册具体元类型才能
// 跨总线编组;而消费侧(popup gui/mainwindow.cpp)要求 Get 回复解组后是
// read 模式 QDBusArgument —— 只有当消费进程内"未注册"该类型时 Qt 才会
// 回退到 QDBusArgument。元类型注册是进程级的,因此编组方(本进程)与
// 消费方(测试进程)必须隔离。
//
// 在私有总线上提供:
//   org.deepin.dde.Display1    @ /org/deepin/dde/Display1
//       - Monitors     (ao)  QList<QDBusObjectPath>
//       - PrimaryRect  (nnnn) ServiceRect
//   org.deepin.dde.Dock1       @ /org/deepin/dde/daemon/Dock1
//       - geometry     (iiii) QRect
//   org.deepin.dde.MockConfig  @ /org/deepin/dde/MockConfig  (knob 控制接口)

#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QList>
#include <QRect>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <cstdio>

namespace {

struct ServiceRect
{
    qint16 x = 0;
    qint16 y = 0;
    qint16 width = 0;
    qint16 height = 0;
};

QDBusArgument &operator<<(QDBusArgument &arg, const ServiceRect &rect)
{
    arg.beginStructure();
    arg << rect.x << rect.y << rect.width << rect.height;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, ServiceRect &rect)
{
    arg.beginStructure();
    arg >> rect.x >> rect.y >> rect.width >> rect.height;
    arg.endStructure();
    return arg;
}

// Display1 mock:属性类型即线上签名,消费侧未注册对应元类型时解组为 QDBusArgument
class MockDisplay1 : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.Display1")
    Q_PROPERTY(QList<QDBusObjectPath> Monitors READ monitors)
    Q_PROPERTY(ServiceRect PrimaryRect READ primaryRect)

public:
    explicit MockDisplay1(QObject *parent = nullptr)
        : QObject(parent) {}

    QList<QDBusObjectPath> monitors() const { return m_monitors; }
    ServiceRect primaryRect() const { return m_rect; }

    void setMonitorPaths(const QStringList &paths)
    {
        m_monitors.clear();
        for (const QString &path : paths)
            m_monitors.append(QDBusObjectPath(path));
    }
    void setPrimaryRect(int x, int y, int width, int height)
    {
        m_rect = ServiceRect { qint16(x), qint16(y), qint16(width), qint16(height) };
    }

private:
    QList<QDBusObjectPath> m_monitors;
    ServiceRect m_rect;
};

// Dock1 mock:geometry 属性 + PropertiesChanged 信号发射
class MockDock1 : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.Dock1")
    Q_PROPERTY(QRect geometry READ geometry)

public:
    explicit MockDock1(QObject *parent = nullptr)
        : QObject(parent) {}

    QRect geometry() const { return m_geometry; }
    void setDockGeometry(int x, int y, int width, int height)
    {
        m_geometry = QRect(x, y, width, height);
    }

    void notifyGeometryChanged()
    {
        QDBusMessage msg = QDBusMessage::createSignal(
            QStringLiteral("/org/deepin/dde/daemon/Dock1"),
            QStringLiteral("org.freedesktop.DBus.Properties"),
            QStringLiteral("PropertiesChanged"));
        msg.setArguments({ QStringLiteral("org.deepin.dde.Dock1"),
                           QVariantMap { { QStringLiteral("geometry"), QVariant::fromValue(m_geometry) } },
                           QStringList() });
        QDBusConnection::sessionBus().send(msg);
    }

private:
    QRect m_geometry;
};

// knob 控制:测试进程通过 Properties.Get 之外的独立接口调整 mock 行为
class MockConfig : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.MockConfig")

public:
    explicit MockConfig(MockDisplay1 *display, MockDock1 *dock, QObject *parent = nullptr)
        : QObject(parent), m_display(display), m_dock(dock) {}

public slots:
    void setMonitorPaths(const QStringList &paths) { m_display->setMonitorPaths(paths); }
    void setPrimaryRect(int x, int y, int width, int height)
    {
        m_display->setPrimaryRect(x, y, width, height);
    }
    void setDockGeometry(int x, int y, int width, int height)
    {
        m_dock->setDockGeometry(x, y, width, height);
    }
    void notifyDockGeometryChanged() { m_dock->notifyGeometryChanged(); }

private:
    MockDisplay1 *m_display = nullptr;
    MockDock1 *m_dock = nullptr;
};

}  // namespace

#include "mock-display-service.moc"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDBusRegisterMetaType<ServiceRect>();

    MockDisplay1 display;
    MockDock1 dock;
    MockConfig config(&display, &dock);

    auto conn = QDBusConnection::sessionBus();
    if (!conn.isConnected()) {
        fprintf(stderr, "[mock-service] no session bus\n");
        return 1;
    }
    if (!conn.registerService(QStringLiteral("org.deepin.dde.Display1"))
        || !conn.registerService(QStringLiteral("org.deepin.dde.Dock1"))
        || !conn.registerService(QStringLiteral("org.deepin.dde.MockConfig"))) {
        fprintf(stderr, "[mock-service] registerService failed: %s\n",
                conn.lastError().message().toUtf8().constData());
        return 1;
    }
    conn.registerObject(QStringLiteral("/org/deepin/dde/Display1"), &display,
                        QDBusConnection::ExportAllProperties);
    conn.registerObject(QStringLiteral("/org/deepin/dde/daemon/Dock1"), &dock,
                        QDBusConnection::ExportAllProperties);
    conn.registerObject(QStringLiteral("/org/deepin/dde/MockConfig"), &config,
                        QDBusConnection::ExportAllSlots);

    fprintf(stdout, "READY\n");
    fflush(stdout);
    return app.exec();
}
