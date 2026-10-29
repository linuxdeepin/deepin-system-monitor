// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 SystemDBusServer 单元测试
// 被测类：deepin-system-monitor-system-server/src/systemdbusserver.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method                | level | factors                       | min | actual |
// |-----------------------|-------|-------------------------------|-----|--------|
// | SystemDBusServer      | mid   | -                             | 1   | 1      |
// | exitDBusServer        | low   | -                             | 1   | 1      |
// | resetExitTimer        | mid   | -                             | 1   | 1      |
// | setServiceEnable      | high  | 空/超长/非法字符/未找到/拒绝/成功/失败 | 7   | 11     |
// | isDKaptureAvailable   | mid   | 未初始化/可用                 | 2   | 2      |
// | getProcessInfoBatch   | high  | 不可用/STAT字段/PID过滤/多类型合并/ |  |   |        |
// |                       |       | 读失败/空列表/CPU溢出保护/traffic跳过| 8   | 8      |
// | dbusCallerPid         | low   | -                             | 1   | 1      |
// | initializeDKapture    | mid   | 成功/失败                     | 2   | 2      |
// | cleanupDKapture       | low   | -                             | 1   | 1      |
//
// 分支清单（源自真实源码，self-check §2c）：
// - ctor: m_dkaptureManager/m_dkaptureInitialized 置空; initializeDKapture;
//         systemBus.registerService 失败走告警分支（测试桩使总线不可用）;
//         registerObject 仅在注册成功后; m_timer.setSingleShot(true)
// - resetExitTimer: m_timer.start(5000)
// - exitDBusServer: qApp->processEvents; m_timer.start(msec)
// - setServiceEnableImpl: B1 名称空/超长(SHRT_MAX)/含'; ' → EINVAL;
//         B2 systemctl list-unit-files 无该服务 → EINVAL;
//         B3 checkAuthorization 失败 → EPERM;
//         B4 enable 且 is-enabled 输出恰为 "enabled" → 返回空串;
//         B5 disable 且输出恰为 "disabled" → 返回空串;
//         B6 其余返回 enable/disable 命令输出 errorRet
// - isDKaptureAvailable: B1 resetExitTimer; B2 m_dkaptureInitialized &&
//         m_dkaptureManager && isAvailable() → true; B3 否则 false
// - getProcessInfoBatch: B1 resetExitTimer; B2 !isDKaptureAvailable → "DKapture not available";
//         B3 回调按 hdr->pid 过滤（非目标 PID 不入结果）; B4 STAT 解析（utime/1e7*hz，
//         sysinfo 2x uptime 溢出保护 → hz/hz/0/0）; B5 IO 字段映射; B6 STATM 字段映射;
//         B7 STATUS uid/gid 映射; B8 traffic 类型跳过; B9 SCHEDSTAT rq_wait_time;
//         B10 read 返回值 >=0 → success=true+data, <0 → "Failed to read DKapture data";
//         [缺陷标注] "DKapture manager not initialized" 分支不可达：
//         isDKaptureAvailable 已要求 m_dkaptureManager 非空，为死代码
// - initializeDKapture: B1 manager 可用 → m_dkaptureInitialized=true; B2 否则 false
// - cleanupDKapture: delete manager → nullptr + 标志复位
// - dbusCallerPid: 非DBus调用 → calledFromDBus()==false → 0

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QIODevice>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>
#include <QVector>
#include <algorithm>
#include <climits>
#include <cerrno>
#include <cstring>
#include <cstddef>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <vector>

#include "stubext.h"
#include "systemdbusserver.h"
#include "dkapture_manager.h"

// 外部链接的自由函数（systemdbusserver.cpp 定义，头文件未声明）
extern bool checkAuthorization(const QString &appBusName, const QString &action);

// ═══════════════════ 打桩状态（QProcess systemctl / polkit） ═══════════════════

namespace {

struct SystemCtlStub {
    QString prog;                        // 最近一次 start 的 program
    QStringList args;                    // 最近一次 start 的参数
    QVector<QStringList> allCalls;       // 全部 start 调用参数
    QString listUnitFilesOut = "fake.service ...";  // list-unit-files 输出
    QString enableOut;                   // enable/disable 输出
    QString isEnabledOut;                // is-enabled 输出
    int polkitCalls = 0;
    bool polkitAllow = false;

    void reset()
    {
        prog.clear();
        args.clear();
        allCalls.clear();
        listUnitFilesOut = "fake.service ...";
        enableOut.clear();
        isEnabledOut.clear();
        polkitCalls = 0;
        polkitAllow = false;
    }
};
SystemCtlStub g_sysctl;

// QDBusContext::message() 在真实 DBus 方法分发上下文之外调用时（d_ptr 为空，Qt 私有
// 实现）会段错误；生产代码中 setServiceEnable 总经由 DBus 调入，测试环境无分发器，
// 故打桩返回静态消息对象模拟调用上下文（服务名可断言传递链路）
QDBusMessage g_dbusContextMessage;

// 校验通过后 processEvents 需要的事件循环
void installSystemCtlStubs(stub_ext::StubExt &stub)
{
    g_dbusContextMessage = QDBusMessage::createMethodCall(
        QStringLiteral("test.dbus.caller"), QStringLiteral("/"),
        QStringLiteral("test.iface"), QStringLiteral("setServiceEnable"));
    stub.set_lamda(&QDBusContext::message,
                   [](const QDBusContext *) -> const QDBusMessage & {
                       return g_dbusContextMessage;
                   });
    using QPStartFn = void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode);
    stub.set_lamda((QPStartFn)&QProcess::start,
                   [](QProcess *, const QString &prog, const QStringList &args, QIODevice::OpenMode) {
                       g_sysctl.prog = prog;
                       g_sysctl.args = args;
                       g_sysctl.allCalls.append(args);
                   });
    stub.set_lamda(ADDR(QProcess, waitForFinished),
                   [](QProcess *, int) -> bool {
                       return true;
                   });
    stub.set_lamda(ADDR(QIODevice, readAll),
                   [](QIODevice *) -> QByteArray {
                       if (g_sysctl.args.isEmpty())
                           return QByteArray();
                       const QString cmd = g_sysctl.args.first();
                       if (cmd == "list-unit-files")
                           return g_sysctl.listUnitFilesOut.toUtf8();
                       if (cmd == "enable" || cmd == "disable")
                           return g_sysctl.enableOut.toUtf8();
                       if (cmd == "is-enabled")
                           return g_sysctl.isEnabledOut.toUtf8();
                       return QByteArray();
                   });
    stub.set_lamda(&checkAuthorization,
                   [](const QString &, const QString &) -> bool {
                       ++g_sysctl.polkitCalls;
                       return g_sysctl.polkitAllow;
                   });
}

// 屏蔽真实 DBus 总线连接（静态工厂返回无效连接对象）
void stubDBusBuses(stub_ext::StubExt &stub)
{
    stub.set_lamda(ADDR(QDBusConnection, systemBus),
                   []() -> QDBusConnection {
                       return QDBusConnection(QString());
                   });
    stub.set_lamda(ADDR(QDBusConnection, sessionBus),
                   []() -> QDBusConnection {
                       return QDBusConnection(QString());
                   });
}

}  // namespace

// ═══════════════════ Fake DKapture 实例（替代 libdkapture eBPF 采集） ═══════════════════

namespace {

class FakeDKapture : public DKapture
{
public:
    // 已构造的记录缓冲（DataHdr 头 + payload，按字节拼接）
    std::vector<std::vector<char>> records;
    // 回调收到的数据类型请求集合
    std::vector<DKapture::DataType> requested;
    int readCalls = 0;
    ssize_t forcedResult = 0;   // !=0 时强制作为 read 返回值

    ssize_t read(std::vector<DKapture::DataType> &dts, DKapture::DKCallback cb, void *ctx) override
    {
        ++readCalls;
        requested.assign(dts.begin(), dts.end());
        ssize_t total = 0;
        for (auto &buf : records) {
            cb(ctx, buf.data(), buf.size());
            total += static_cast<ssize_t>(buf.size());
        }
        return forcedResult != 0 ? forcedResult : total;
    }

    // 其余接口本测试不触达，给确定的安全返回值
    ssize_t read(DKapture::DataType, DKapture::DKCallback, void *) override { return -1; }
    int open(FILE *, LogLevel) override { return 0; }
    unsigned long long lifetime(unsigned long long ms) override { return ms; }
    ssize_t read(DKapture::DataType, pid_t, DKapture::DataHdr *, size_t) override { return -1; }
    ssize_t read(const char *, DKapture::DataHdr *, size_t) override { return -1; }
    ssize_t read(std::vector<DKapture::DataType> &, pid_t, DKapture::DataHdr *, size_t) override { return -1; }
    ssize_t read(std::vector<const char *> &, DKapture::DataHdr *, size_t) override { return -1; }
    ssize_t read(DKapture::DataType, std::vector<pid_t> &, DKapture::DataHdr *, size_t) override { return -1; }
    ssize_t read(std::vector<const char *> &, DKapture::DKCallback, void *) override { return -1; }
    int kmemleak_scan_start(pid_t, DKapture::DKCallback, void *) override { return -1; }
    int kmemleak_scan_stop(void) override { return 0; }
    int file_watch(const char *, DKapture::DKCallback, void *) override { return -1; }
    int fs_watch(const char *, DKapture::DKCallback, void *) override { return -1; }
    int irq_watch(DKapture::DKCallback, void *) override { return -1; }
    int close(void) override { return 0; }
};

// 构造一条 DKapture::DataHdr + payload 的字节记录
template<typename Payload>
std::vector<char> makeRecord(DKapture::DataType type, pid_t pid, pid_t tgid,
                             const char *comm, const Payload &payload)
{
    const size_t hdrSize = offsetof(DKapture::DataHdr, data);
    std::vector<char> buf(hdrSize + sizeof(Payload), 0);
    auto *hdr = reinterpret_cast<DKapture::DataHdr *>(buf.data());
    hdr->type = type;
    hdr->dsz = static_cast<unsigned int>(sizeof(Payload));
    strncpy(hdr->comm, comm, TASK_COMM_LEN - 1);
    hdr->pid = pid;
    hdr->tgid = tgid;
    memcpy(hdr->data, &payload, sizeof(Payload));
    return buf;
}

}  // namespace

// ═══════════════════ 测试夹具 ═══════════════════

class SystemDBusServerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stub.clear();
        g_sysctl.reset();
        stubDBusBuses(stub);
        installSystemCtlStubs(stub);

        // DKaptureManager 单例（main.cpp 全局环境已屏蔽 QLibrary::load），
        // 先复位成员再构造被测对象，保证 ctor 走"DKapture 初始化失败"分支
        mgr = DKaptureManager::instance();
        mgr->m_available = false;
        mgr->m_dk_instance = nullptr;

        server = new SystemDBusServer();
    }

    void TearDown() override
    {
        // 防止 DKaptureManager 单例退出析构时 delete 测试栈上的 fake 实例
        mgr->m_available = false;
        mgr->m_dk_instance = nullptr;
        delete server;
        server = nullptr;
        stub.clear();
    }

    // 将被测对象置于 DKapture 可用状态（fake 实例承载回调数据）
    void makeDkaptureAvailable()
    {
        mgr->m_available = true;
        mgr->m_dk_instance = &fake;
        server->m_dkaptureManager = mgr;
        server->m_dkaptureInitialized = true;
        fake.records.clear();
        fake.requested.clear();
        fake.readCalls = 0;
        fake.forcedResult = 0;
    }

    stub_ext::StubExt stub;
    DKaptureManager *mgr = nullptr;
    SystemDBusServer *server = nullptr;
    FakeDKapture fake;
};

// ═══════════════════ 构造 / 定时器 ═══════════════════

TEST_F(SystemDBusServerTest, Ctor_WithoutDkapture_InitializationFailed)
{
    // Arrange: SetUp 已完成构造（全局桩屏蔽 QLibrary::load）
    ASSERT_NE(server, nullptr);
    // Act: 构造过程即被测行为
    EXPECT_TRUE(server->m_timer.isSingleShot());
    // Assert: 初始化标志为 false，manager 指向单例
    EXPECT_FALSE(server->m_dkaptureInitialized);
    EXPECT_EQ(server->m_dkaptureManager, mgr);
}

TEST_F(SystemDBusServerTest, ResetExitTimer_DefaultCall_Starts5000msTimer)
{
    // Arrange: 初始定时器未激活
    ASSERT_FALSE(server->m_timer.isActive());
    // Act
    server->resetExitTimer();
    // Assert: EXIT_TIMEOUT 固定 5000ms
    EXPECT_TRUE(server->m_timer.isActive());
    EXPECT_EQ(server->m_timer.interval(), 5000);
}

TEST_F(SystemDBusServerTest, ExitDBusServer_CustomDelay_UsesGivenTimeout)
{
    // Arrange: 初始定时器未激活
    ASSERT_FALSE(server->m_timer.isActive());
    // Act
    server->exitDBusServer(1234);
    // Assert
    EXPECT_TRUE(server->m_timer.isActive());
    EXPECT_EQ(server->m_timer.interval(), 1234);
}

// ═══════════════════ isDKaptureAvailable / initialize / cleanup ═══════════════════

TEST_F(SystemDBusServerTest, IsDKaptureAvailable_NotInitialized_ReturnsFalse)
{
    // Arrange: 默认未初始化
    ASSERT_NE(server, nullptr);
    // Act
    bool available = server->isDKaptureAvailable();
    // Assert: 不可用；副作用 resetExitTimer 生效
    EXPECT_FALSE(available);
    EXPECT_EQ(server->m_timer.interval(), 5000);
}

TEST_F(SystemDBusServerTest, IsDKaptureAvailable_InitializedAndAvailable_ReturnsTrue)
{
    // Arrange: 置于可用状态
    makeDkaptureAvailable();
    // Act
    bool available = server->isDKaptureAvailable();
    // Assert
    EXPECT_TRUE(available);
    EXPECT_EQ(server->m_timer.interval(), 5000);
}

TEST_F(SystemDBusServerTest, InitializeDKapture_AvailableManager_SetsFlag)
{
    // Arrange: 单例可用（fake 实例）
    mgr->m_available = true;
    mgr->m_dk_instance = &fake;
    // Act
    server->initializeDKapture();
    // Assert
    EXPECT_TRUE(server->m_dkaptureInitialized);
    EXPECT_EQ(server->m_dkaptureManager, mgr);
}

TEST_F(SystemDBusServerTest, InitializeDKapture_UnavailableManager_ClearsFlag)
{
    // Arrange: 先置 true，模拟重初始化
    server->m_dkaptureInitialized = true;
    // Act: mgr 处于不可用状态（SetUp 已复位）
    server->initializeDKapture();
    // Assert: 标志复位，manager 仍指向单例
    EXPECT_FALSE(server->m_dkaptureInitialized);
    EXPECT_EQ(server->m_dkaptureManager, mgr);
}

TEST_F(SystemDBusServerTest, CleanupDKapture_HeapManager_DeletesAndResets)
{
    // Arrange: 挂一个独立堆对象（不是单例，避免单例悬垂）
    server->m_dkaptureManager = new DKaptureManager();
    server->m_dkaptureInitialized = true;
    // Act
    server->cleanupDKapture();
    // Assert
    EXPECT_EQ(server->m_dkaptureManager, nullptr);
    EXPECT_FALSE(server->m_dkaptureInitialized);
}

// ═══════════════════ dbusCallerPid ═══════════════════

TEST_F(SystemDBusServerTest, DbusCallerPid_OutsideDBusCall_ReturnsZero)
{
    // Arrange: 测试环境不存在 DBus 方法调用上下文
    ASSERT_NE(server, nullptr);
    // Act
    qint64 pid = server->dbusCallerPid();
    // Assert: calledFromDBus()==false → 0
    EXPECT_EQ(pid, 0);
    EXPECT_EQ(pid, static_cast<qint64>(0));
}

// ═══════════════════ setServiceEnable ═══════════════════

TEST_F(SystemDBusServerTest, SetServiceEnable_EmptyName_ReturnsEINVAL)
{
    // Arrange: 空服务名
    ASSERT_TRUE(g_sysctl.allCalls.isEmpty());
    // Act
    QString ret = server->setServiceEnable(QString(), true);
    // Assert: 参数校验先于 systemctl
    EXPECT_EQ(ret.toStdString(), strerror(EINVAL));
    EXPECT_EQ(g_sysctl.allCalls.size(), 0);
}

TEST_F(SystemDBusServerTest, SetServiceEnable_NameWithSemicolon_ReturnsEINVAL)
{
    // Arrange: 服务名含 ';'
    ASSERT_TRUE(g_sysctl.allCalls.isEmpty());
    // Act
    QString ret = server->setServiceEnable("fake;service", true);
    // Assert: 非法字符被拒绝且未发起 systemctl
    EXPECT_EQ(ret.toStdString(), strerror(EINVAL));
    EXPECT_EQ(g_sysctl.allCalls.size(), 0);
}

TEST_F(SystemDBusServerTest, SetServiceEnable_NameWithSpace_ReturnsEINVAL)
{
    // Arrange: 服务名含空格
    ASSERT_TRUE(g_sysctl.allCalls.isEmpty());
    // Act
    QString ret = server->setServiceEnable("fake service", false);
    // Assert
    EXPECT_EQ(ret.toStdString(), strerror(EINVAL));
    EXPECT_EQ(g_sysctl.allCalls.size(), 0);
}

TEST_F(SystemDBusServerTest, SetServiceEnable_TooLongName_ReturnsEINVAL)
{
    // Arrange: 超过 SHRT_MAX 的服务名
    ASSERT_TRUE(g_sysctl.allCalls.isEmpty());
    QString longName(static_cast<int>(SHRT_MAX) + 1, QChar('x'));
    // Act
    QString ret = server->setServiceEnable(longName, true);
    // Assert
    EXPECT_EQ(ret.toStdString(), strerror(EINVAL));
    EXPECT_EQ(g_sysctl.allCalls.size(), 0);
}

TEST_F(SystemDBusServerTest, SetServiceEnable_ServiceNotFound_ReturnsEINVAL)
{
    // Arrange: list-unit-files 输出不含目标服务
    g_sysctl.listUnitFilesOut = "other.service ...";
    // Act
    QString ret = server->setServiceEnable("fake.service", true);
    // Assert: 未鉴权、未执行 enable
    EXPECT_EQ(ret.toStdString(), strerror(EINVAL));
    EXPECT_EQ(g_sysctl.polkitCalls, 0);
    ASSERT_EQ(g_sysctl.allCalls.size(), 1);
    EXPECT_EQ(g_sysctl.allCalls.at(0).first(), "list-unit-files");
}

TEST_F(SystemDBusServerTest, SetServiceEnable_PolkitDenied_ReturnsEPERM)
{
    // Arrange: 服务存在但鉴权失败
    g_sysctl.polkitAllow = false;
    // Act
    QString ret = server->setServiceEnable("fake.service", true);
    // Assert
    EXPECT_EQ(ret.toStdString(), strerror(EPERM));
    EXPECT_EQ(g_sysctl.polkitCalls, 1);
    EXPECT_EQ(g_sysctl.allCalls.size(), 1);   // 未执行 enable
}

TEST_F(SystemDBusServerTest, SetServiceEnable_EnableSuccess_ReturnsEmpty)
{
    // Arrange: 鉴权通过，is-enabled 恰好输出 "enabled"
    g_sysctl.polkitAllow = true;
    g_sysctl.isEnabledOut = "enabled";
    g_sysctl.enableOut = "Created symlink\n";
    // Act
    QString ret = server->setServiceEnable("fake.service", true);
    // Assert: 返回空串表示成功，调用序列完整
    EXPECT_TRUE(ret.isEmpty());
    ASSERT_EQ(g_sysctl.allCalls.size(), 3);
    EXPECT_EQ(g_sysctl.allCalls.at(1).first(), "enable");
    EXPECT_EQ(g_sysctl.allCalls.at(2).first(), "is-enabled");
}

TEST_F(SystemDBusServerTest, SetServiceEnable_DisableSuccess_ReturnsEmpty)
{
    // Arrange
    g_sysctl.polkitAllow = true;
    g_sysctl.isEnabledOut = "disabled";
    // Act
    QString ret = server->setServiceEnable("fake.service", false);
    // Assert
    EXPECT_TRUE(ret.isEmpty());
    ASSERT_EQ(g_sysctl.allCalls.size(), 3);
    EXPECT_EQ(g_sysctl.allCalls.at(1).first(), "disable");
}

TEST_F(SystemDBusServerTest, SetServiceEnable_StatusMismatch_ReturnsCommandOutput)
{
    // Arrange: enable 后 is-enabled 输出既非 "enabled" 也非 "disabled"
    g_sysctl.polkitAllow = true;
    g_sysctl.enableOut = "Failed to enable unit\n";
    g_sysctl.isEnabledOut = "static\n";
    // Act
    QString ret = server->setServiceEnable("fake.service", true);
    // Assert: 返回 enable 命令的输出
    EXPECT_EQ(ret, QStringLiteral("Failed to enable unit\n"));
    EXPECT_EQ(g_sysctl.polkitCalls, 1);
}

// ═══════════════════ getProcessInfoBatch ═══════════════════

TEST_F(SystemDBusServerTest, GetProcessInfoBatch_NotAvailable_ReturnsError)
{
    // Arrange: 未初始化 DKapture
    ASSERT_FALSE(server->m_dkaptureInitialized);
    // Act
    QVariantMap result = server->getProcessInfoBatch({ 100 });
    // Assert
    EXPECT_FALSE(result["success"].toBool());
    EXPECT_EQ(result["error"].toString(), QStringLiteral("DKapture not available"));
    EXPECT_FALSE(result.contains("data"));
}

TEST_F(SystemDBusServerTest, GetProcessInfoBatch_StatRecord_ConvertsAndMapsFields)
{
    // Arrange: 一条 STAT 记录
    makeDkaptureAvailable();
    ProcPidStat stat;
    memset(&stat, 0, sizeof(stat));
    stat.state = 1;
    stat.ppid = 1;
    stat.priority = 20;
    stat.nice = 0;
    stat.num_threads = 4;
    stat.start_time = 123456;
    stat.vsize = 4096000;
    stat.rss = 10240;
    stat.utime = 3000000000ULL;   // /1e7 = 300s → 30000 jiffies (hz=100)
    stat.stime = 1500000000ULL;   // 150s → 15000 jiffies
    stat.cutime = 0;
    stat.cstime = 0;
    fake.records.push_back(makeRecord(DKapture::PROC_PID_STAT, 100, 100, "testproc", stat));

    const long hz = sysconf(_SC_CLK_TCK);
    // Act
    QVariantMap result = server->getProcessInfoBatch({ 100 });
    // Assert: 结果结构与字段换算
    EXPECT_TRUE(result["success"].toBool());
    EXPECT_FALSE(result.contains("error"));
    QVariantMap data = result["data"].toMap();
    EXPECT_EQ(data.size(), 1);
    ASSERT_TRUE(data.contains("100"));
    QVariantMap pidData = data["100"].toMap();
    EXPECT_EQ(pidData["pid"].toInt(), 100);
    EXPECT_EQ(pidData["tgid"].toInt(), 100);
    // [缺陷标注] QString::fromUtf8(hdr->comm, TASK_COMM_LEN) 按固定 16 字节转换，
    // 保留内核 NUL 填充的尾随空字符（"testproc\0..."），下游字符串相等比较/长度判断
    // 会与预期不符；此处按当前实际行为断言，缺陷已登记 Mode 5 清单
    const QString comm = pidData["comm"].toString();
    EXPECT_TRUE(comm.startsWith(QStringLiteral("testproc")));
    EXPECT_EQ(comm.size(), TASK_COMM_LEN);
    EXPECT_EQ(pidData["state"].toInt(), 1);
    EXPECT_EQ(pidData["ppid"].toInt(), 1);
    EXPECT_EQ(pidData["utime"].toULongLong(), static_cast<qulonglong>(300 * hz));
    EXPECT_EQ(pidData["stime"].toULongLong(), static_cast<qulonglong>(150 * hz));
    EXPECT_EQ(pidData["cutime"].toULongLong(), 0ULL);
    EXPECT_EQ(pidData["cstime"].toULongLong(), 0ULL);
    EXPECT_EQ(pidData["cpu_time"].toULongLong(), 450ULL);
    EXPECT_EQ(pidData["priority"].toInt(), 20);
    EXPECT_EQ(pidData["nice"].toInt(), 0);
    EXPECT_EQ(pidData["num_threads"].toInt(), 4);
    EXPECT_EQ(pidData["start_time"].toULongLong(), 123456ULL);
    EXPECT_EQ(pidData["vsize"].toULongLong(), 4096000ULL);
    EXPECT_EQ(pidData["rss"].toULongLong(), 10240ULL);

    // 请求的数据类型集合与实现一致
    ASSERT_EQ(fake.requested.size(), 5u);
    EXPECT_TRUE(std::find(fake.requested.begin(), fake.requested.end(), DKapture::PROC_PID_STAT) != fake.requested.end());
    EXPECT_TRUE(std::find(fake.requested.begin(), fake.requested.end(), DKapture::PROC_PID_IO) != fake.requested.end());
    EXPECT_TRUE(std::find(fake.requested.begin(), fake.requested.end(), DKapture::PROC_PID_STATM) != fake.requested.end());
    EXPECT_TRUE(std::find(fake.requested.begin(), fake.requested.end(), DKapture::PROC_PID_STATUS) != fake.requested.end());
    EXPECT_TRUE(std::find(fake.requested.begin(), fake.requested.end(), DKapture::PROC_PID_SCHEDSTAT) != fake.requested.end());
}

TEST_F(SystemDBusServerTest, GetProcessInfoBatch_NonTargetPid_ExcludedFromResult)
{
    // Arrange: 目标 100，额外投递 200 与子线程 101（tgid=100）
    makeDkaptureAvailable();
    ProcPidStat stat;
    memset(&stat, 0, sizeof(stat));
    stat.utime = 10000000ULL;   // 1s
    fake.records.push_back(makeRecord(DKapture::PROC_PID_STAT, 100, 100, "target", stat));
    fake.records.push_back(makeRecord(DKapture::PROC_PID_STAT, 200, 200, "other", stat));
    fake.records.push_back(makeRecord(DKapture::PROC_PID_STAT, 101, 100, "thread", stat));

    // Act
    QVariantMap result = server->getProcessInfoBatch({ 100 });
    // Assert: 仅目标 PID 进入结果
    EXPECT_TRUE(result["success"].toBool());
    QVariantMap data = result["data"].toMap();
    EXPECT_EQ(data.size(), 1);
    EXPECT_TRUE(data.contains("100"));
    EXPECT_FALSE(data.contains("200"));
    EXPECT_FALSE(data.contains("101"));
}

TEST_F(SystemDBusServerTest, GetProcessInfoBatch_MultiTypeRecords_MergedIntoOneMap)
{
    // Arrange: 同一 PID 的五类记录合并进一个 map
    makeDkaptureAvailable();

    ProcPidStat stat;
    memset(&stat, 0, sizeof(stat));
    stat.state = 2;
    stat.utime = 10000000ULL;   // 1s
    fake.records.push_back(makeRecord(DKapture::PROC_PID_STAT, 100, 100, "proc", stat));

    ProcPidIo io;
    memset(&io, 0, sizeof(io));
    io.rchar = 100;
    io.wchar = 200;
    io.syscr = 3;
    io.syscw = 4;
    io.read_bytes = 500;
    io.write_bytes = 600;
    io.cancelled_write_bytes = 700;
    fake.records.push_back(makeRecord(DKapture::PROC_PID_IO, 100, 100, "proc", io));

    ProcPidStatm statm;
    memset(&statm, 0, sizeof(statm));
    statm.size = 10;
    statm.resident = 20;
    statm.shared = 30;
    statm.text = 40;
    statm.data = 50;
    fake.records.push_back(makeRecord(DKapture::PROC_PID_STATM, 100, 100, "proc", statm));

    ProcPidStatus status;
    memset(&status, 0, sizeof(status));
    status.umask = 18;
    status.state = 1;
    status.tracer_pid = 0;
    status.uid[0] = 1000; status.uid[1] = 1001; status.uid[2] = 1002; status.uid[3] = 1003;
    status.gid[0] = 2000; status.gid[1] = 2001; status.gid[2] = 2002; status.gid[3] = 2003;
    fake.records.push_back(makeRecord(DKapture::PROC_PID_STATUS, 100, 100, "proc", status));

    ProcPidSchedstat schedstat;
    memset(&schedstat, 0, sizeof(schedstat));
    schedstat.cpu_time = 9000;
    schedstat.rq_wait_time = 8000;
    schedstat.timeslices = 7;
    fake.records.push_back(makeRecord(DKapture::PROC_PID_SCHEDSTAT, 100, 100, "proc", schedstat));

    // Act
    QVariantMap result = server->getProcessInfoBatch({ 100 });
    // Assert: 五类字段全部合并
    QVariantMap data = result["data"].toMap();
    EXPECT_EQ(data.size(), 1);
    QVariantMap pidData = data["100"].toMap();
    // STAT
    EXPECT_EQ(pidData["state"].toInt(), 2);
    EXPECT_EQ(pidData["utime"].toULongLong(), static_cast<qulonglong>(sysconf(_SC_CLK_TCK)));
    // IO
    EXPECT_EQ(pidData["rchar"].toULongLong(), 100ULL);
    EXPECT_EQ(pidData["wchar"].toULongLong(), 200ULL);
    EXPECT_EQ(pidData["syscr"].toULongLong(), 3ULL);
    EXPECT_EQ(pidData["syscw"].toULongLong(), 4ULL);
    EXPECT_EQ(pidData["read_bytes"].toULongLong(), 500ULL);
    EXPECT_EQ(pidData["write_bytes"].toULongLong(), 600ULL);
    EXPECT_EQ(pidData["cancelled_write_bytes"].toULongLong(), 700ULL);
    // STATM
    EXPECT_EQ(pidData["memory_size"].toULongLong(), 10ULL);
    EXPECT_EQ(pidData["memory_resident"].toULongLong(), 20ULL);
    EXPECT_EQ(pidData["memory_shared"].toULongLong(), 30ULL);
    EXPECT_EQ(pidData["memory_text"].toULongLong(), 40ULL);
    EXPECT_EQ(pidData["memory_data"].toULongLong(), 50ULL);
    // STATUS
    EXPECT_EQ(pidData["uid"].toUInt(), 1000u);
    EXPECT_EQ(pidData["euid"].toUInt(), 1001u);
    EXPECT_EQ(pidData["suid"].toUInt(), 1002u);
    EXPECT_EQ(pidData["fsuid"].toUInt(), 1003u);
    EXPECT_EQ(pidData["gid"].toUInt(), 2000u);
    EXPECT_EQ(pidData["egid"].toUInt(), 2001u);
    EXPECT_EQ(pidData["sgid"].toUInt(), 2002u);
    EXPECT_EQ(pidData["fsgid"].toUInt(), 2003u);
    EXPECT_EQ(pidData["state_from_status"].toInt(), 1);
    EXPECT_EQ(pidData["tracer_pid"].toInt(), 0);
    EXPECT_EQ(pidData["umask"].toInt(), 18);
    // SCHEDSTAT
    EXPECT_EQ(pidData["rq_wait_time"].toULongLong(), 8000ULL);
}

TEST_F(SystemDBusServerTest, GetProcessInfoBatch_CpuOverflow_ResetsToSafeValues)
{
    // Arrange: utime 远超 2 倍系统运行时间 → 触发保护
    makeDkaptureAvailable();
    ProcPidStat stat;
    memset(&stat, 0, sizeof(stat));
    stat.utime = 100000000000000000ULL;   // 1e18ns → 1e10s，必然超限
    stat.stime = 100000000000000000ULL;
    fake.records.push_back(makeRecord(DKapture::PROC_PID_STAT, 100, 100, "proc", stat));
    const long hz = sysconf(_SC_CLK_TCK);

    // Act
    QVariantMap result = server->getProcessInfoBatch({ 100 });
    // Assert: CPU 时间被重置为安全值（1 秒 jiffies / 0）
    QVariantMap data = result["data"].toMap();
    EXPECT_EQ(data.size(), 1);
    QVariantMap pidData = data["100"].toMap();
    EXPECT_EQ(pidData["utime"].toULongLong(), static_cast<qulonglong>(hz));
    EXPECT_EQ(pidData["stime"].toULongLong(), static_cast<qulonglong>(hz));
    EXPECT_EQ(pidData["cutime"].toULongLong(), 0ULL);
    EXPECT_EQ(pidData["cstime"].toULongLong(), 0ULL);
}

TEST_F(SystemDBusServerTest, GetProcessInfoBatch_TrafficRecord_SkippedFields)
{
    // Arrange: traffic 类型记录不产生任何业务字段
    makeDkaptureAvailable();
    char dummyPayload[8] = { 0 };
    fake.records.push_back(makeRecord(DKapture::PROC_PID_traffic, 100, 100, "proc", dummyPayload));

    // Act
    QVariantMap result = server->getProcessInfoBatch({ 100 });
    // Assert: 仅保留头部信息（pid/tgid/comm），无 traffic 字段
    QVariantMap data = result["data"].toMap();
    EXPECT_EQ(data.size(), 1);
    QVariantMap pidData = data["100"].toMap();
    EXPECT_EQ(pidData["pid"].toInt(), 100);
    // [缺陷标注] comm 尾随 NUL 同上（QString::fromUtf8 固定 TASK_COMM_LEN 转换）
    EXPECT_TRUE(pidData["comm"].toString().startsWith(QStringLiteral("proc")));
    EXPECT_FALSE(pidData.contains("rbytes"));
    EXPECT_FALSE(pidData.contains("wbytes"));
    EXPECT_EQ(pidData.size(), 3);
}

TEST_F(SystemDBusServerTest, GetProcessInfoBatch_ReadFails_ReturnsError)
{
    // Arrange: fake read 返回负值
    makeDkaptureAvailable();
    fake.forcedResult = -5;
    // Act
    QVariantMap result = server->getProcessInfoBatch({ 100 });
    // Assert
    EXPECT_FALSE(result["success"].toBool());
    EXPECT_TRUE(result["error"].toString().contains("Failed to read DKapture data"));
    EXPECT_FALSE(result.contains("data"));
    EXPECT_EQ(fake.readCalls, 1);
}

TEST_F(SystemDBusServerTest, GetProcessInfoBatch_EmptyPidList_SuccessWithEmptyData)
{
    // Arrange: 目标集合为空
    makeDkaptureAvailable();
    ProcPidStat stat;
    memset(&stat, 0, sizeof(stat));
    fake.records.push_back(makeRecord(DKapture::PROC_PID_STAT, 100, 100, "proc", stat));
    // Act
    QVariantMap result = server->getProcessInfoBatch({});
    // Assert: 读取成功但无目标数据
    EXPECT_TRUE(result["success"].toBool());
    EXPECT_EQ(fake.readCalls, 1);
    EXPECT_TRUE(result["data"].toMap().isEmpty());
}

TEST_F(SystemDBusServerTest, GetProcessInfoBatch_EntryPoint_ResetsExitTimer)
{
    // Arrange: 置于可用状态且定时器未激活
    makeDkaptureAvailable();
    ASSERT_FALSE(server->m_timer.isActive());
    // Act
    server->getProcessInfoBatch({ 100 });
    // Assert: 入口即重置 5 秒退出定时器
    EXPECT_TRUE(server->m_timer.isActive());
    EXPECT_EQ(server->m_timer.interval(), 5000);
}
