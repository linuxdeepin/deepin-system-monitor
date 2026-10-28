// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_popup_stub.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <DRegionMonitor>
#include <DWindowManagerHelper>
#include <QEvent>
#include <QProcess>
#include <QFileInfo>
#include <unistd.h>
#include <QThread>

#include "../../tests/stub.h"
#include "application.h"
#include "gui/datadealsingleton.h"
#include "helper.hpp"

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE
DGUI_USE_NAMESPACE

// ==================== Mock 服务进程管理 ====================
// 说明:源码 gui/mainwindow.cpp 从 Properties.Get 应答中以
// `v.value<QDBusVariant>().variant().value<QDBusArgument>()` 解组数据,
// 这要求应答在测试进程内解组时"未注册"具体元类型(否则解组为具体类型,
// value<QDBusArgument>() 得到空对象,while(!atEnd()) 死循环)。
// 而 Mock 服务端必须注册具体元类型才能跨总线编组属性值 —— 元类型注册是
// 进程级的,因此 Mock 服务运行在独立进程 dsm-popup-mock-service 中,
// 通过 org.deepin.dde.MockConfig 接口接受 knob 控制。

namespace ut_popup_stub {

// ---- 计数器与观测值 ----
int g_startDetachedCount = 0;
int g_threadStartCount = 0;
QString g_lastDetachedProgram;
int g_registerRegionCount = 0;
int g_jumpWidgetCount = 0;
QString g_lastJumpMessage;

// ---- DataDealSingleton 数据源 knob ----
bool g_readNetInfoResult = true;
QString g_netReceive = "0 B/s";
QString g_netTotalReceive = "0 B";
QString g_netSend = "0 B/s";
QString g_totalSend = "0 B";
bool g_readDiskInfoResult = true;
QString g_diskRead = "0 B/s";
QString g_diskTotalSize = "0 B";
QString g_diskWrite = "0 B/s";
QString g_diskAvailable = "0 B";
bool g_jumpMessageResult = true;

// ---- 行为开关 ----
bool g_hasComposite = false;
bool g_displayInterValid = false;
bool g_monitorLoopValid = false;

// ---- 内部登记表 ----
static QSet<const QObject *> s_validIfaces;
static QHash<const QObject *, QMap<QString, QVariant>> s_ifaceProps;

// ---- Mock 服务进程与私有总线 ----
static QProcess *s_mockService = nullptr;
static QProcess *s_busDaemon = nullptr;
static bool s_mockServiceReady = false;

// MockConfig knob 调用(阻塞式,服务未就绪时静默忽略)
static void callMockConfig(const QString &method, const QList<QVariant> &args)
{
    if (!s_mockServiceReady)
        return;
    QDBusInterface config(QStringLiteral("org.deepin.dde.MockConfig"),
                          QStringLiteral("/org/deepin/dde/MockConfig"),
                          QStringLiteral("org.deepin.dde.MockConfig"),
                          QDBusConnection::sessionBus());
    config.callWithArgumentList(QDBus::NoBlock, method, args);
}

void setInterfaceValid(const QObject *iface, bool valid)
{
    if (valid)
        s_validIfaces.insert(iface);
    else
        s_validIfaces.remove(iface);
}

void setInterfaceProperty(const QObject *iface, const QString &name, const QVariant &value)
{
    s_ifaceProps[iface][name] = value;
}

void setPrimaryRectReply(bool enabled, const QRect &rect)
{
    const QRect target = enabled ? rect : QRect();
    callMockConfig(QStringLiteral("setPrimaryRect"),
                   { target.x(), target.y(), target.width(), target.height() });
}

void setMonitorPathsReply(bool enabled, const QList<QDBusObjectPath> &paths)
{
    QStringList names;
    if (enabled) {
        for (const auto &path : paths)
            names.append(path.path());
    }
    callMockConfig(QStringLiteral("setMonitorPaths"), { QVariant::fromValue(names) });
}

void resetKnobs()
{
    g_startDetachedCount = 0;
    g_threadStartCount = 0;
    g_lastDetachedProgram.clear();
    g_registerRegionCount = 0;
    g_jumpWidgetCount = 0;
    g_lastJumpMessage.clear();
    g_hasComposite = false;
    g_displayInterValid = false;
    g_monitorLoopValid = false;
    g_readNetInfoResult = true;
    g_netReceive = QStringLiteral("0 B/s");
    g_netTotalReceive = QStringLiteral("0 B");
    g_netSend = QStringLiteral("0 B/s");
    g_totalSend = QStringLiteral("0 B");
    g_readDiskInfoResult = true;
    g_diskRead = QStringLiteral("0 B/s");
    g_diskTotalSize = QStringLiteral("0 B");
    g_diskWrite = QStringLiteral("0 B/s");
    g_diskAvailable = QStringLiteral("0 B");
    g_jumpMessageResult = true;
    s_validIfaces.clear();
    s_ifaceProps.clear();
    // 复位 Mock 服务属性(空显示器路径 / 空矩形 / 空 dock 几何)
    callMockConfig(QStringLiteral("setMonitorPaths"), { QVariant::fromValue(QStringList()) });
    callMockConfig(QStringLiteral("setPrimaryRect"), { 0, 0, 0, 0 });
    callMockConfig(QStringLiteral("setDockGeometry"), { 0, 0, 0, 0 });
}

// ==================== 各桩实现 ====================

// QDBusAbstractInterface::isValid —— 登记表优先,内容兜底(Display/显示器监视接口)
static bool stub_isValid(const QDBusAbstractInterface *self)
{
    if (s_validIfaces.contains(self))
        return true;
    if (self != nullptr && self->service() == common::systemInfo().DISPLAY_SERVICE) {
        if (self->interface() == common::systemInfo().DISPLAY_INTERFACE)
            return g_displayInterValid;
        if (self->interface() == common::systemInfo().DISPLAYMONITOR_INTERFACE)
            return g_monitorLoopValid;
    }
    return false;
}

// QObject::property —— 登记表优先;DBus 代理对象未登记的属性返回空
// (避免走 metaobject 读取造成 DBusDockInterface::geometry 递归),
// 普通 QObject 回落 metaobject 原始读取
static QVariant stub_QObject_property(const QObject *self, const char *name)
{
    if (name == nullptr)
        return QVariant();
    auto it = s_ifaceProps.find(self);
    if (it != s_ifaceProps.end()) {
        auto pit = it->find(QString::fromLatin1(name));
        if (pit != it->end())
            return pit.value();
    }
    if (qobject_cast<const QDBusAbstractInterface *>(self) != nullptr)
        return QVariant();
    const QMetaObject *mo = self->metaObject();
    int id = mo != nullptr ? mo->indexOfProperty(name) : -1;
    if (id < 0)
        return QVariant();
    return mo->property(id).read(self);
}

// DWindowManagerHelper::hasComposite —— 无合成器环境默认 false
static bool stub_hasComposite(const DWindowManagerHelper *)
{
    return g_hasComposite;
}

// DRegionMonitor::registerRegion —— 只计数,不触碰真实键盘抓取
static void stub_registerRegion(DRegionMonitor *)
{
    ++g_registerRegionCount;
}

// QProcess::startDetached —— 只记录,不真正拉起进程
static bool stub_startDetached(const QString &program, const QStringList &,
                               const QString &, qint64 *)
{
    ++g_startDetachedCount;
    g_lastDetachedProgram = program;
    return true;
}

static Stub *s_stub = nullptr;

// 线程启动拦截:仅阻止插件监控线程(BaseThread/SystemMonitorThread)真正运行
// (Application::event 是虚函数,成员指针存虚表索引无法补丁,改拦非虚的 QThread::start)。
// 其余 QThread(如 Qt6 并行光栅填充使用的 QThreadPool 工作线程)必须放行,
// 否则填充运行体永远不执行,主线程在 QSemaphore::acquire 上永久死锁。
static void stub_QThread_start(void *self, int priority)
{
    ++g_threadStartCount;
    auto *threadObj = static_cast<QObject *>(self);
    if (threadObj && threadObj->inherits("common::core::BaseThread"))
        return;  // 插件监控线程不真正启动

    // 临时还原真实 QThread::start 后调用,再重新装回桩(装桩/还原仅在主线程发生)
    using StartFn = void (*)(void *, int);
    union {
        void (QThread::*mem)(QThread::Priority);
        void *addr;
    } cast;
    cast.mem = &QThread::start;
    const auto realStart = reinterpret_cast<StartFn>(cast.addr);
    s_stub->reset(ADDR(QThread, start));
    realStart(self, priority);
    s_stub->set(ADDR(QThread, start), stub_QThread_start);
}

// DataDealSingleton::readNetInfo —— 注入固定的网络读数,不依赖真实网卡与 /proc
static bool stub_DataDeal_readNetInfo(void *, QString &netReceive, QString &netTotalReceive,
                                      QString &netSend, QString &totalSend)
{
    netReceive = g_netReceive;
    netTotalReceive = g_netTotalReceive;
    netSend = g_netSend;
    totalSend = g_totalSend;
    return g_readNetInfoResult;
}

// DataDealSingleton::readDiskInfo —— 注入固定的磁盘读数,不依赖真实磁盘
static bool stub_DataDeal_readDiskInfo(void *, QString &diskRead, QString &diskTotalSize,
                                       QString &diskWrite, QString &diskAvailable)
{
    diskRead = g_diskRead;
    diskTotalSize = g_diskTotalSize;
    diskWrite = g_diskWrite;
    diskAvailable = g_diskAvailable;
    return g_readDiskInfoResult;
}

// DataDealSingleton::sendJumpWidgetMessage —— 只记录跳转消息,不唤醒主进程
static bool stub_DataDeal_sendJumpWidgetMessage(void *, const QString &dbusMessage)
{
    ++g_jumpWidgetCount;
    g_lastJumpMessage = dbusMessage;
    return g_jumpMessageResult;
}


void installAll()
{
    if (s_stub != nullptr)
        return;  // 幂等:进程级仅安装一次
    s_stub = new Stub();

    // DBus 接口有效性 / 属性读取
    s_stub->set((bool (QDBusAbstractInterface::*)() const)ADDR(QDBusAbstractInterface, isValid),
                stub_isValid);
    s_stub->set((QVariant (QObject::*)(const char *) const)ADDR(QObject, property),
                stub_QObject_property);
    // 合成器状态 / 键盘区域注册
    s_stub->set((bool (DWindowManagerHelper::*)() const)ADDR(DWindowManagerHelper, hasComposite),
                stub_hasComposite);
    s_stub->set((void (DRegionMonitor::*)())ADDR(DRegionMonitor, registerRegion),
                stub_registerRegion);
    // detached 进程启动
    s_stub->set((bool (*)(const QString &, const QStringList &, const QString &, qint64 *))&QProcess::startDetached,
                stub_startDetached);
    // 监控线程启动拦截
    s_stub->set(ADDR(QThread, start), stub_QThread_start);
    // DataDealSingleton 数据源(读网/读盘/跳转)
    s_stub->set((bool (DataDealSingleton::*)(QString &, QString &, QString &, QString &))ADDR(DataDealSingleton, readNetInfo),
                stub_DataDeal_readNetInfo);
    s_stub->set((bool (DataDealSingleton::*)(QString &, QString &, QString &, QString &))ADDR(DataDealSingleton, readDiskInfo),
                stub_DataDeal_readDiskInfo);
    s_stub->set((bool (DataDealSingleton::*)(const QString &))ADDR(DataDealSingleton, sendJumpWidgetMessage),
                stub_DataDeal_sendJumpWidgetMessage);

    fprintf(stderr, "[stub] all installed\n");
    resetKnobs();
}

bool startPrivateSessionBus()
{
    if (s_busDaemon != nullptr)
        return true;  // 幂等:私有总线仅启动一次

    // 以 --nofork 启动独立的私有 session dbus-daemon,便于退出时随 QProcess 结束,
    // 避免与用户会话总线互相干扰
    s_busDaemon = new QProcess();
    s_busDaemon->start(QStringLiteral("dbus-daemon"),
                       QStringList { QStringLiteral("--session"), QStringLiteral("--nofork"),
                                     QStringLiteral("--print-address=1"), QStringLiteral("--nopidfile") });
    if (!s_busDaemon->waitForStarted(3000)) {
        fprintf(stderr, "[stub] dbus-daemon start failed: %s\n",
                QString::fromUtf8(s_busDaemon->readAllStandardError()).toUtf8().constData());
        delete s_busDaemon;
        s_busDaemon = nullptr;
        return false;
    }
    // 等待地址输出(--nofork 模式下 daemon 持续运行,输出不会随进程结束)
    QString address;
    for (int i = 0; i < 50 && address.isEmpty(); ++i) {
        const QString out = QString::fromUtf8(s_busDaemon->readAllStandardOutput());
        for (const QString &line : out.split(QLatin1Char('\n'))) {
            const QString trimmed = line.trimmed();
            if (trimmed.startsWith(QStringLiteral("unix:"))) {
                address = trimmed;
                break;
            }
        }
        if (address.isEmpty())
            s_busDaemon->waitForReadyRead(100);
    }
    if (address.isEmpty()) {
        fprintf(stderr, "[stub] dbus-daemon printed no address\n");
        return false;
    }
    // 必须在 Application 构造(首次连接总线)之前设置
    qputenv("DBUS_SESSION_BUS_ADDRESS", address.toUtf8());
    fprintf(stderr, "[stub] private session bus: %s\n", address.toUtf8().constData());
    return true;
}

bool startMockService()
{
    if (s_mockServiceReady)
        return true;
    if (s_busDaemon == nullptr) {
        fprintf(stderr, "[stub] private bus not started, mock service skipped\n");
        return false;
    }

    // 拉起 Mock 服务进程(与测试可执行同目录),总线地址经环境变量继承。
    // Application 尚未构造,不能用 QCoreApplication::applicationDirPath(),改读 /proc/self/exe
    QString selfPath;
    {
        char buf[4096];
        ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            selfPath = QFile::decodeName(buf);
        }
    }
    const QString mockPath = QFileInfo(selfPath).absolutePath() + QStringLiteral("/dsm-popup-mock-service");
    s_mockService = new QProcess();
    s_mockService->setProgram(mockPath);
    s_mockService->start();
    if (!s_mockService->waitForStarted(3000)) {
        fprintf(stderr, "[stub] mock service start failed: %s\n",
                s_mockService->errorString().toUtf8().constData());
        return false;
    }
    // 等待 READY 输出
    if (!s_mockService->waitForReadyRead(5000)
        && !QString::fromUtf8(s_mockService->readAllStandardOutput()).contains(QStringLiteral("READY"))) {
        fprintf(stderr, "[stub] mock service not ready\n");
        return false;
    }

    // 等待服务真正注册到总线(阻塞式查询,最长 ~5s)
    QDBusConnectionInterface *busIface = QDBusConnection::sessionBus().interface();
    for (int i = 0; i < 100; ++i) {
        if (busIface->isServiceRegistered(QStringLiteral("org.deepin.dde.Display1")))
            break;
        QThread::msleep(50);
    }
    if (!busIface->isServiceRegistered(QStringLiteral("org.deepin.dde.Display1"))) {
        fprintf(stderr, "[stub] mock service never registered Display1\n");
        return false;
    }
    s_mockServiceReady = true;
    fprintf(stderr, "[stub] mock service ready\n");
    return true;
}

void stopMockService()
{
    s_mockServiceReady = false;
    if (s_mockService != nullptr) {
        s_mockService->terminate();
        if (!s_mockService->waitForFinished(2000))
            s_mockService->kill();
        delete s_mockService;
        s_mockService = nullptr;
    }
    if (s_busDaemon != nullptr) {
        s_busDaemon->terminate();
        if (!s_busDaemon->waitForFinished(2000))
            s_busDaemon->kill();
        delete s_busDaemon;
        s_busDaemon = nullptr;
    }
}

}  // namespace ut_popup_stub
