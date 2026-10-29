// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_DBusAlarmNotify：DBusAlarmNotify 单元测试（qt-autotest-generator 生成）
//
// 被测文件：deepin-system-monitor-main/dbus/dbusalarmnotify.cpp
//
// 打桩说明（外部依赖全部 stub，不依赖真实 DBus/进程环境）：
// - QDBusConnection::sessionBus   → 返回未连接连接（不触碰真实会话总线）
// - QDBusConnection::call         → 受控回复（ReplyMessage / ErrorMessage）+ 计数
// - QProcess 拉起进程接口        → 计数并捕获命令（不派生真实进程）
//
// ============================ 用例计数声明 ============================
// 方法                                  | level | 最少用例 | actual | 说明
// DBusAlarmNotify() 私有构造             | low   | 1 | 1 | 经 getInstance 单例创建覆盖
// getInstance()                         | low   | 1 | 1 | 两次调用同实例（覆盖创建路径+快路径）
// showAlarmNotify(const QStringList&)   | mid   | 2 | 6 | TEST_P：cpu 有效/无效、CPU 大写、memory 有效/无效、未知类型
// showAlarmNotify(topic,msg,timeout)    | mid   | 2 | 2 | 错误回复/成功回复（重载均为 public）
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例
// [x] 2 输入按等价类划分：类型 cpu/memory/未知 × 取值 有效整数/非法串/大小写变体
// [x] 3 边界值显式覆盖：toInt 成功/失败（isok true/false 两分支）
// [x] 4 同质多组输入用 TEST_P 参数化（6 组同质断言输入）
// [x] 5 分支清单已列出并映射到用例名
// [x] 6 每条 if/else 分支有触发用例（类型分派 3 分支 + 数值校验 2 分支 + 回复错误/成功 2 分支）
// [x] 7 本类无显式 throw，不适用 EXPECT_THROW
// [x] 8 负面场景（非法数值、未知类型、DBus 错误回复）有专门用例
// [x] 9 负面用例验证强异常安全（非法输入后无任何外部副作用：call/startDetached 计数为 0）
// [x] 10 项目内接口类用 gMock，其他用 stub_ext（本项目 Qt/静态依赖统一用项目自带 stub.h 打桩）
//
// ============================== 分支清单 ==============================
// （来源：工作目录源码 dbusalarmnotify.cpp）
// showAlarmNotify(const QStringList&):
//   B1  [1]=="cpu"(ci) 且 toInt 成功 → 私有重载发通知        → ShowAlarmNotify_RoutedByTypeAndValue_NotifiesPerRouting(cpu/CPU 有效)
//   B2  [1]=="cpu"(ci) 且 toInt 失败 → 仅告警无副作用         → 同上(cpu 无效)
//   B3  [1]=="memory"(ci) 且成功 → 私有重载发通知             → 同上(memory 有效)
//   B4  [1]=="memory"(ci) 且失败 → 仅告警无副作用             → 同上(memory 无效)
//   B5  其他类型 → 仅告警无副作用                             → 同上(unknown)
// showAlarmNotify(topic,msg,timeout) 私有:
//   B6  回复 ErrorMessage → gdbus 置告警时间戳 0              → ShowAlarmNotify_Private_ReplyError_ResetsAlarmTimestampToZero
//   B7  回复非 Error → gdbus 更新告警时间戳为当前毫秒         → ShowAlarmNotify_Private_ReplySuccess_UpdatesAlarmTimestamp

#include "dbus/dbusalarmnotify.h"
//qt
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVariant>
//gtest
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
//stub
#include "stub.h"

using namespace testing;

// ============================== 打桩实现 ==============================
namespace {
int g_dbusCallCount = 0;
int g_detachCount = 0;
QString g_lastDetachCmd;

enum class StubReplyKind { Success, Error };
StubReplyKind g_replyKind = StubReplyKind::Success;
}  // namespace

// static QDBusConnection QDBusConnection::sessionBus()
static QDBusConnection stub_QDBusConnection_sessionBus()
{
    // 返回未注册名字的连接：不会触碰真实 DBus 守护进程
    return QDBusConnection(QStringLiteral("ut-unconnected-bus"));
}

// QDBusMessage QDBusConnection::call(const QDBusMessage&, CallMode, int) const
static QDBusMessage stub_QDBusConnection_call(void *, const QDBusMessage &, QDBus::CallMode, int)
{
    ++g_dbusCallCount;
    if (g_replyKind == StubReplyKind::Success)
        return QDBusMessage::createMethodCall(QStringLiteral("ut.svc"), QStringLiteral("/ut"),
                                              QStringLiteral("ut.if"), QStringLiteral("Notify"));
    return QDBusMessage::createError(QDBusError::ServiceUnknown,
                                     QStringLiteral("stub: service unknown"));
}

// QProcess 静态拉起接口打桩（Qt6 签名：startDetached(QString, QStringList, QString, qint64*)）：仅计数，不派生真实进程
static bool stub_QProcess_startDetached(const QString &cmd, const QStringList & = { },
                                        const QString & = QString(), qint64 * = nullptr)
{
    ++g_detachCount;
    g_lastDetachCmd = cmd;
    return true;
}

namespace {
void resetAlarmStubState()
{
    g_dbusCallCount = 0;
    g_detachCount = 0;
    g_lastDetachCmd.clear();
    g_replyKind = StubReplyKind::Success;
}
}  // namespace

// ============================== 夹具 ==============================
class UT_DBusAlarmNotify : public ::testing::Test
{
public:
    UT_DBusAlarmNotify() {}
    ~UT_DBusAlarmNotify() override {}

    void SetUp() override
    {
        resetAlarmStubState();
    }

    void TearDown() override {}
};

// 参数化夹具：类型/取值/是否期望发起通知
namespace {
struct AlarmArgParam {
    QString type;        // allArguments[1]
    QString value;       // allArguments[2]
    bool expectNotify;   // 是否期望发起 DBus 通知与时间戳更新
};
}  // namespace

class UT_DBusAlarmNotify_Param : public UT_DBusAlarmNotify,
                                 public ::testing::WithParamInterface<AlarmArgParam>
{
};

// ============================== 用例 ==============================
// B-getInstance: 两次调用返回同一实例（首次走创建路径，二次走快路径）
TEST_F(UT_DBusAlarmNotify, GetInstance_TwiceCalled_SameInstance)
{
    // Arrange: 环境就绪（单例从零开始，无需打桩）
    // Act: 连续两次获取单例
    DBusAlarmNotify &first = DBusAlarmNotify::getInstance();
    DBusAlarmNotify &second = DBusAlarmNotify::getInstance();
    // Assert: 均为有效对象且同一实例
    EXPECT_NE(&first, nullptr);
    EXPECT_EQ(&first, &second);
    EXPECT_EQ(&first, &second);
}

// B1-B5: 类型/取值等价类路由（TEST_P 参数化）
TEST_P(UT_DBusAlarmNotify_Param, ShowAlarmNotify_RoutedByTypeAndValue_NotifiesPerRouting)
{
    const AlarmArgParam param = GetParam();
    // Arrange: 打桩并置受控回复
    Stub stub;
    stub.set(ADDR(QDBusConnection, sessionBus), stub_QDBusConnection_sessionBus);
    stub.set((QDBusMessage (QDBusConnection::*)(const QDBusMessage &, QDBus::CallMode, int) const)
                 ADDR(QDBusConnection, call), stub_QDBusConnection_call);
    stub.set((bool (*)(const QString &, const QStringList &, const QString &, qint64 *))
                 ADDR(QProcess, startDetached), stub_QProcess_startDetached);
    resetAlarmStubState();

    // Act: 调度告警参数
    DBusAlarmNotify::getInstance().showAlarmNotify(QStringList() << "alarm" << param.type << param.value);

    // Assert: 有效输入发起一次通知并更新时间戳；无效输入零副作用
    const int expectCount = param.expectNotify ? 1 : 0;
    EXPECT_EQ(g_dbusCallCount, expectCount) << "type=" << qPrintable(param.type);
    EXPECT_EQ(g_detachCount, expectCount) << "type=" << qPrintable(param.type);
    if (param.expectNotify) {
        // 成功回复：时间戳为当前毫秒（>0），而非错误路径的 0
        EXPECT_FALSE(g_lastDetachCmd.contains("setAlaramLastTimeInterval 0"));
        const long long ts = g_lastDetachCmd.section(' ', -1).toLongLong();
        EXPECT_GT(ts, 0);
    } else {
        EXPECT_TRUE(g_lastDetachCmd.isEmpty());
    }
}

INSTANTIATE_TEST_SUITE_P(AlarmArgRouting, UT_DBusAlarmNotify_Param, ::testing::Values(
                             AlarmArgParam { "cpu", "90", true },        // B1 cpu 有效
                             AlarmArgParam { "CPU", "85", true },        // B1 大小写不敏感
                             AlarmArgParam { "cpu", "abc", false },      // B2 cpu 无效
                             AlarmArgParam { "memory", "75", true },     // B3 memory 有效
                             AlarmArgParam { "memory", "-1x", false },   // B4 memory 无效
                             AlarmArgParam { "disk", "50", false }       // B5 未知类型
                             ));

// B6: 三参重载 + 错误回复 → gdbus 命令置告警时间戳为 0
TEST_F(UT_DBusAlarmNotify, ShowAlarmNotify_ThreeArgs_ReplyError_ResetsAlarmTimestampToZero)
{
    // Arrange: 打桩并置错误回复
    Stub stub;
    stub.set(ADDR(QDBusConnection, sessionBus), stub_QDBusConnection_sessionBus);
    stub.set((QDBusMessage (QDBusConnection::*)(const QDBusMessage &, QDBus::CallMode, int) const)
                 ADDR(QDBusConnection, call), stub_QDBusConnection_call);
    stub.set((bool (*)(const QString &, const QStringList &, const QString &, qint64 *))
                 ADDR(QProcess, startDetached), stub_QProcess_startDetached);
    resetAlarmStubState();
    g_replyKind = StubReplyKind::Error;

    // Act: 调用三参重载（错误路径）
    DBusAlarmNotify::getInstance().showAlarmNotify(QStringLiteral("Warning"),
                                                   QStringLiteral("Your CPU usage is higher than 90%!"),
                                                   10000);

    // Assert: 通知发出且时间戳复位为 0
    EXPECT_EQ(g_dbusCallCount, 1);
    EXPECT_EQ(g_detachCount, 1);
    EXPECT_TRUE(g_lastDetachCmd.contains("setAlaramLastTimeInterval 0"));
}

// B7: 三参重载 + 成功回复 → gdbus 命令更新告警时间戳为当前毫秒
TEST_F(UT_DBusAlarmNotify, ShowAlarmNotify_ThreeArgs_ReplySuccess_UpdatesAlarmTimestamp)
{
    // Arrange: 打桩并置成功回复
    Stub stub;
    stub.set(ADDR(QDBusConnection, sessionBus), stub_QDBusConnection_sessionBus);
    stub.set((QDBusMessage (QDBusConnection::*)(const QDBusMessage &, QDBus::CallMode, int) const)
                 ADDR(QDBusConnection, call), stub_QDBusConnection_call);
    stub.set((bool (*)(const QString &, const QStringList &, const QString &, qint64 *))
                 ADDR(QProcess, startDetached), stub_QProcess_startDetached);
    resetAlarmStubState();

    // Act: 调用三参重载（成功路径）
    DBusAlarmNotify::getInstance().showAlarmNotify(QStringLiteral("Warning"),
                                                   QStringLiteral("Your memory usage is higher than 75%!"),
                                                   10000);

    // Assert: 通知发出且时间戳为正数（当前毫秒）
    EXPECT_EQ(g_dbusCallCount, 1);
    EXPECT_EQ(g_detachCount, 1);
    EXPECT_FALSE(g_lastDetachCmd.contains("setAlaramLastTimeInterval 0"));
    const long long ts = g_lastDetachCmd.section(' ', -1).toLongLong();
    EXPECT_GT(ts, 0);
}
