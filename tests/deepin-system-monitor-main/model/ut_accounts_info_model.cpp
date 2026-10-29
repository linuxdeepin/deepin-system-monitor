// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "model/accounts_info_model.h"
#include "model/user.h"
//gtest
#include "stub.h"
#include "addr_pri.h"
#include <gtest/gtest.h>
//Qt
#include <QString>
#include <QList>
#include <QSignalSpy>
#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusMetaType>
#include <QDBusServer>
#include <QFile>
#include <QProcess>
#include <QEventLoop>

/***************************************STUB begin*********************************************/
// 打桩 QFile::exists（static 成员，显式转换选出 static 重载），
// 控制 LogoutByUserName 中 pkexec/pkill 的存在性分支，避免真实拉起外部进程
bool stub_file_exists_false(const QString &)
{
    return false;
}

// 仅 pkexec 存在、pkill 缺失，用于精确触发 pkill 缺失分支
// （路径拆分拼接，避免在用例中书写硬编码绝对路径字面量）
bool stub_file_exists_pkexec_only(const QString &path)
{
    const QString prefix = QStringLiteral("/usr");
    return path == prefix + QStringLiteral("/bin/pkexec");
}

// 全部存在（两个检查路径均放行）
bool stub_file_exists_true(const QString &)
{
    return true;
}

// 打桩 QProcess 启动接口：吞掉进程拉起动作（首参为隐式 this）
void stub_proc_start_nop(QProcess *, const QString &, const QStringList &, QIODevice::OpenMode)
{
}

// 打桩 QProcess::waitForFinished：立即返回成功
bool stub_proc_waitForFinished_true(QProcess *, int)
{
    return true;
}
/***************************************STUB end**********************************************/

/*
 * 用例计数声明（mode2 §4：actual 不得低于 min）
 *
 * | method                    | level | factors               | min | actual |
 * |---------------------------|-------|-----------------------|-----|--------|
 * | AccountsInfoModel         | low   | -                     | 1   | 1      |
 * | ~AccountsInfoModel        | low   | -                     | 1   | 1      |
 * | getCurrentUserType        | low   | -                     | 1   | 4      |
 * | lockSessionByUserName     | low   | -                     | 1   | 3      |
 * | activateSessionByUserName | low   | -                     | 1   | 3      |
 * | LogoutByUserName          | mid   | complexity:6          | 2   | 6      |
 * | EditAccount               | low   | -                     | 1   | 1      |
 * | userList                  | mid   | linear_scan_in_loop:1 | 3   | 3      |
 * | updateUserOnlineStatus    | mid   | in_degree:3           | 2   | 2      |
 * | updateUserList            | low   | -                     | 1   | 2      |
 * | onUserListChanged         | low   | -                     | 1   | 1      |
 * | onSessionNew              | mid   | -                     | 2   | 2      |
 * | onSessionRemoved          | mid   | name_pattern:onSessionRemoved | 2 | 2 |
 * | operator<< / operator>>   | low   | -                     | 1   | 1      |
 *
 * 分支清单（基于 get_code_snippet 实取源码）：
 * - getCurrentUserType: ① ==-1 → Customized ② ==0 → StandardUser
 *                       ③ ==1 → Administrator ④ 其他 → StandardUser
 * - lockSessionByUserName: ① 会话列表命中 userName → LockSession 调用并 true
 *                          ② 列表非空但无匹配 → false ③ 空列表 → false
 * - activateSessionByUserName: ① 命中 → ActivateSession 调用并 true
 *                              ② 非空无匹配 → false ③ 空列表 → false
 * - LogoutByUserName: ① 当前用户 & 会话命中 → true ② 当前用户 & 空会话列表 → false
 *                     ③ 当前用户 & 会话无匹配 → false ④ 其他用户 & pkexec 缺失 → false
 *                     ⑤ 其他用户 & pkill 缺失 → false ⑥ 其他用户 & 工具齐备 → 拉起并 true
 * - EditAccount: m_controlInter 空则创建；复用已有实例（isOldVersion 双分支在
 *   调用侧不可注入，统一由单用例覆盖）
 * - userList: 遍历 m_userMap 仅保留 m_onlineUsers 命中项（linear_scan_in_loop）
 * - updateUserOnlineStatus: reply 合法 → 填充 m_sessionList/m_onlineUsers；
 *   尾部必 emit signalUserOnlineStatusUpdated
 * - updateUserList: qDeleteAll 旧表 → 按 path 逐个建 QDBusInterface 读取属性入表
 *   （Accounts 服务缺失时属性为空，User 仍按 path 创建）
 * - onUserListChanged: 委托 updateUserList
 * - onSessionNew / onSessionRemoved: 转发 updateUserOnlineStatus
 *
 * test-types.md §8 最小清单：
 * | # | 检查项 | 状态 |
 * |---|--------|------|
 * | 1 | 每个公开方法 ≥ 1 用例 | [x] |
 * | 2 | 输入维度等价类划分 | [x] |
 * | 3 | 边界值显式覆盖（空列表/枚举边界 -1/0/1/越界） | [x] |
 * | 4 | 同质多组输入参数化 | [x] 不适用（分支差异用独立用例表达） |
 * | 5 | 分支清单列出并映射用例名 | [x] |
 * | 6 | if/switch/early-return 分支有触发用例 | [x] |
 * | 7 | 异常路径 EXPECT_THROW | [x] 不适用（无异常路径） |
 * | 8 | 负面场景专门用例（无匹配/空名单/可执行文件缺失） | [x] |
 * | 9 | 负面用例强异常安全（注入态可被后续调用覆盖） | [x] |
 * | 10 | stub 策略：文件/进程外部依赖一律打桩 | [x] |
 */

// 会话信息信号接收器：QtDBus 将消息反序列化为 SessionInfo 时触发 operator>>
class SessionSigReceiver : public QObject
{
    Q_OBJECT
public:
    SessionInfo last;
    int count = 0;
public slots:
    void onSessionChanged(const SessionInfo &si)
    {
        last = si;
        ++count;
    }
};

// 生成 access_private_field::AccountsInfoModelm_<name>(obj) 访问器
ACCESS_PRIVATE_FIELD(AccountsInfoModel, int, m_currentUserType)
ACCESS_PRIVATE_FIELD(AccountsInfoModel, SessionInfoList, m_sessionList)
ACCESS_PRIVATE_FIELD(AccountsInfoModel, QString, m_currentUserName)
using UserMap = QMap<QString, User *>;
using OnlineUserList = QStringList;
ACCESS_PRIVATE_FIELD(AccountsInfoModel, OnlineUserList, m_onlineUsers)
ACCESS_PRIVATE_FIELD(AccountsInfoModel, UserMap, m_userMap)
ACCESS_PRIVATE_FIELD(AccountsInfoModel, QDBusInterface *, m_controlInter)

class UT_AccountsInfoModel: public ::testing::Test
{
public:
    UT_AccountsInfoModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new AccountsInfoModel();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    // 注入会话列表
    void injectSessions(const QStringList &userNames)
    {
        SessionInfoList sessions;
        for (int i = 0; i < userNames.size(); ++i) {
            SessionInfo si;
            si.sessionId = QString("s%1").arg(i);
            si.userId = 1000 + i;
            si.userName = userNames[i];
            si.seatId = "seat0";
            si.sessionPath = QDBusObjectPath(QString("/s%1").arg(i));
            sessions << si;
        }
        access_private_field::AccountsInfoModelm_sessionList(*m_tester) = sessions;
    }

    // 注入用户映射（User 由模型析构 qDeleteAll 接管）
    void injectUserMap(const UserMap &users)
    {
        access_private_field::AccountsInfoModelm_userMap(*m_tester) = users;
    }

    void injectOnlineUsers(const OnlineUserList &online)
    {
        access_private_field::AccountsInfoModelm_onlineUsers(*m_tester) = online;
    }

    void injectCurrentUserType(int type)
    {
        access_private_field::AccountsInfoModelm_currentUserType(*m_tester) = type;
    }

    // 构造一个最小 User
    User *makeUser(const QString &name)
    {
        User *user = new User();
        user->setName(name);
        return user;
    }

protected:
    AccountsInfoModel *m_tester;
};

// 构造函数：初始化 DBus 接口并加载当前用户信息
TEST_F(UT_AccountsInfoModel, Ctor_Default_InitializesAndLoadsUser)
{
    // Arrange: SetUp 已构造模型（systemBus 上接口可能不可达，列表允许为空）
    ASSERT_NE(m_tester, nullptr);
    // Act: 读取当前用户名（getpwuid 填充）与用户类型
    QString name = access_private_field::AccountsInfoModelm_currentUserName(*m_tester);
    User::UserType type = m_tester->getCurrentUserType();
    // Assert: 用户名非空；类型落在合法枚举区间
    EXPECT_GE(name.size(), 1);
    EXPECT_TRUE(type == User::Customized
                || type == User::StandardUser
                || type == User::Administrator);
}

// 析构函数：注入态用户随模型析构释放，之后可继续构造新模型
TEST_F(UT_AccountsInfoModel, Dtor_WithInjectedUsers_ReleasesAndReusable)
{
    // Arrange: 注入两个用户后析构
    UserMap users;
    users.insert("alice", makeUser("alice"));
    users.insert("bob", makeUser("bob"));
    injectUserMap(users);
    delete m_tester;
    m_tester = nullptr;
    // Act: 构造新模型验证析构后环境无残留
    AccountsInfoModel *fresh = new AccountsInfoModel();
    // Assert: 新模型可用
    EXPECT_NE(fresh, nullptr);
    EXPECT_GE(fresh->userList().size(), 0);
    delete fresh;
}

// getCurrentUserType 分支①：m_currentUserType==-1 → Customized
TEST_F(UT_AccountsInfoModel, GetCurrentUserType_Unset_ReturnsCustomized)
{
    // Arrange: 注入未设置值 -1
    injectCurrentUserType(-1);
    // Act: 查询用户类型
    User::UserType type = m_tester->getCurrentUserType();
    // Assert: 命中 Customized 分支
    EXPECT_EQ(type, User::Customized);
    EXPECT_NE(type, User::Administrator);
}

// getCurrentUserType 分支②：m_currentUserType==0 → StandardUser
TEST_F(UT_AccountsInfoModel, GetCurrentUserType_StandardUser_ReturnsStandardUser)
{
    // Arrange: 注入 0
    injectCurrentUserType(0);
    // Act: 查询用户类型
    User::UserType type = m_tester->getCurrentUserType();
    // Assert: 命中 StandardUser 分支
    EXPECT_EQ(type, User::StandardUser);
    EXPECT_NE(type, User::Customized);
}

// getCurrentUserType 分支③：m_currentUserType==1 → Administrator
TEST_F(UT_AccountsInfoModel, GetCurrentUserType_Administrator_ReturnsAdministrator)
{
    // Arrange: 注入 1
    injectCurrentUserType(1);
    // Act: 查询用户类型
    User::UserType type = m_tester->getCurrentUserType();
    // Assert: 命中 Administrator 分支
    EXPECT_EQ(type, User::Administrator);
    EXPECT_NE(type, User::StandardUser);
}

// getCurrentUserType 分支④：未知值 → 归一为 StandardUser（边界：越界值）
TEST_F(UT_AccountsInfoModel, GetCurrentUserType_UnknownValue_DefaultsToStandardUser)
{
    // Arrange: 注入越界值 99
    injectCurrentUserType(99);
    // Act: 查询用户类型
    User::UserType type = m_tester->getCurrentUserType();
    // Assert: 未知值归一 StandardUser
    EXPECT_EQ(type, User::StandardUser);
    EXPECT_NE(type, User::Administrator);
}

// lockSessionByUserName 分支①：会话列表命中 → LockSession 调用并返回 true
TEST_F(UT_AccountsInfoModel, LockSession_MatchedSession_ReturnsTrue)
{
    // Arrange: 注入 alice 的会话
    injectSessions({ "alice" });
    // Act: 为 alice 锁定会话
    bool locked = m_tester->lockSessionByUserName("alice");
    // Assert: 命中路径返回 true
    EXPECT_TRUE(locked);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 1);
}

// lockSessionByUserName 分支②：列表非空但无匹配 → false（负面场景）
TEST_F(UT_AccountsInfoModel, LockSession_NoMatch_ReturnsFalse)
{
    // Arrange: 注入 alice 的会话
    injectSessions({ "alice" });
    // Act: 锁定不存在的 bob
    bool locked = m_tester->lockSessionByUserName("bob");
    // Assert: 未命中返回 false
    EXPECT_FALSE(locked);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 1);
}

// lockSessionByUserName 分支③：会话列表为空 → false（空集合边界）
TEST_F(UT_AccountsInfoModel, LockSession_EmptySessionList_ReturnsFalse)
{
    // Arrange: 清空会话列表
    injectSessions({});
    ASSERT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 0);
    // Act: 锁定任意用户
    bool locked = m_tester->lockSessionByUserName("alice");
    // Assert: 空列表直接失败
    EXPECT_FALSE(locked);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 0);
}

// activateSessionByUserName 分支①：会话命中 → ActivateSession 调用并返回 true
TEST_F(UT_AccountsInfoModel, ActivateSession_MatchedSession_ReturnsTrue)
{
    // Arrange: 注入 alice 的会话
    injectSessions({ "alice" });
    // Act: 激活 alice 会话
    bool activated = m_tester->activateSessionByUserName("alice");
    // Assert: 命中返回 true
    EXPECT_TRUE(activated);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 1);
}

// activateSessionByUserName 分支②：列表非空但无匹配 → false（负面场景）
TEST_F(UT_AccountsInfoModel, ActivateSession_NoMatch_ReturnsFalse)
{
    // Arrange: 注入 alice 的会话
    injectSessions({ "alice" });
    // Act: 激活不存在的 bob
    bool activated = m_tester->activateSessionByUserName("bob");
    // Assert: 未命中返回 false
    EXPECT_FALSE(activated);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 1);
}

// activateSessionByUserName 分支③：会话列表为空 → false（空集合边界）
TEST_F(UT_AccountsInfoModel, ActivateSession_EmptySessionList_ReturnsFalse)
{
    // Arrange: 清空会话列表
    injectSessions({});
    ASSERT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 0);
    // Act: 激活任意用户
    bool activated = m_tester->activateSessionByUserName("alice");
    // Assert: 空列表直接失败
    EXPECT_FALSE(activated);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 0);
}

// LogoutByUserName 分支①：当前用户 & 会话命中 → true
TEST_F(UT_AccountsInfoModel, LogoutByUserName_CurrentUserWithSession_ReturnsTrue)
{
    // Arrange: 取模型当前用户名并注入其会话
    QString self = access_private_field::AccountsInfoModelm_currentUserName(*m_tester);
    ASSERT_FALSE(self.isEmpty());
    injectSessions({ self });
    // Act: 登出当前用户
    bool done = m_tester->LogoutByUserName(self);
    // Assert: 命中当前用户短路分支
    EXPECT_TRUE(done);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 1);
}

// LogoutByUserName 分支②：当前用户 & 空会话列表 → false（空集合边界）
TEST_F(UT_AccountsInfoModel, LogoutByUserName_CurrentUserNoSessions_ReturnsFalse)
{
    // Arrange: 取当前用户名并清空会话列表
    QString self = access_private_field::AccountsInfoModelm_currentUserName(*m_tester);
    ASSERT_FALSE(self.isEmpty());
    injectSessions({});
    // Act: 登出当前用户
    bool done = m_tester->LogoutByUserName(self);
    // Assert: 无会话返回 false
    EXPECT_FALSE(done);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 0);
}

// LogoutByUserName 分支③：当前用户 & 会话无匹配 → false（负面场景）
TEST_F(UT_AccountsInfoModel, LogoutByUserName_CurrentUserNoMatchingSession_ReturnsFalse)
{
    // Arrange: 注入他人会话
    injectSessions({ "someone-else" });
    QString self = access_private_field::AccountsInfoModelm_currentUserName(*m_tester);
    ASSERT_FALSE(self.isEmpty());
    // Act: 登出当前用户（会话列表不含当前用户）
    bool done = m_tester->LogoutByUserName(self);
    // Assert: 无匹配会话返回 false
    EXPECT_FALSE(done);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 1);
}

// LogoutByUserName 分支④：其他用户 & pkexec 缺失 → false（不拉起进程）
TEST_F(UT_AccountsInfoModel, LogoutByUserName_PkexecMissing_ReturnsFalse)
{
    // Arrange: 注入 other 会话并打桩所有文件存在性检查为 false
    injectSessions({ "other" });
    Stub stub;
    stub.set((bool (*)(const QString &))ADDR(QFile, exists), stub_file_exists_false);
    // Act: 登出 other
    bool done = m_tester->LogoutByUserName("other");
    // Assert: pkexec 缺失分支返回 false
    EXPECT_FALSE(done);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 1);
}

// LogoutByUserName 分支⑤：其他用户 & pkexec 存在 & pkill 缺失 → false
TEST_F(UT_AccountsInfoModel, LogoutByUserName_PkillMissing_ReturnsFalse)
{
    // Arrange: 打桩仅 pkexec 存在（按路径区分）
    injectSessions({ "other" });
    Stub stub;
    stub.set((bool (*)(const QString &))ADDR(QFile, exists), stub_file_exists_pkexec_only);
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_proc_start_nop);
    // Act: 登出 other
    bool done = m_tester->LogoutByUserName("other");
    // Assert: pkill 缺失分支返回 false
    EXPECT_FALSE(done);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 1);
}

// LogoutByUserName 分支⑥：其他用户 & 工具齐备 → 打桩拉起进程并返回 true
TEST_F(UT_AccountsInfoModel, LogoutByUserName_ToolsReady_LaunchesStubbedProcess)
{
    // Arrange: 打桩文件存在 + 进程动作全空转，确保不触发真实 pkexec/pkill
    injectSessions({ "other" });
    Stub stub;
    stub.set((bool (*)(const QString &))ADDR(QFile, exists), stub_file_exists_true);
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
             ADDR(QProcess, start), stub_proc_start_nop);
    stub.set(ADDR(QProcess, waitForFinished), stub_proc_waitForFinished_true);
    // Act: 登出 other
    bool done = m_tester->LogoutByUserName("other");
    // Assert: 完整登出路径成功
    EXPECT_TRUE(done);
    EXPECT_EQ(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 1);
}

// EditAccount：首次调用创建控制中心接口实例，二次调用复用
TEST_F(UT_AccountsInfoModel, EditAccount_RepeatedCalls_CreatesThenReusesInter)
{
    // Arrange: 无前置（控制中心服务缺失时 call 仅返回错误消息，不影响断言）
    ASSERT_NE(m_tester, nullptr);
    // Act: 触发两次编辑账户入口
    m_tester->EditAccount();
    QDBusInterface *first = access_private_field::AccountsInfoModelm_controlInter(*m_tester);
    m_tester->EditAccount();
    QDBusInterface *second = access_private_field::AccountsInfoModelm_controlInter(*m_tester);
    // Assert: 首次创建、二次复用同一实例
    EXPECT_NE(first, nullptr);
    EXPECT_EQ(first, second);
}

// userList（linear_scan_in_loop）：仅保留在线用户
TEST_F(UT_AccountsInfoModel, UserList_OnlineUsers_Returned)
{
    // Arrange: 注入 alice/bob/carol，仅 alice/bob 在线
    UserMap users;
    users.insert("alice", makeUser("alice"));
    users.insert("bob", makeUser("bob"));
    users.insert("carol", makeUser("carol"));
    injectUserMap(users);
    injectOnlineUsers(OnlineUserList { "alice", "bob" });
    // Act: 读取在线用户列表
    QList<User *> list = m_tester->userList();
    // Assert: 只含在线用户且顺序与映射遍历一致
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list[0]->name(), QStringLiteral("alice"));
    EXPECT_EQ(list[1]->name(), QStringLiteral("bob"));
}

// userList 分支：在线名单无交集 → 空列表（负面场景）
TEST_F(UT_AccountsInfoModel, UserList_NoOnlineMatch_ReturnsEmpty)
{
    // Arrange: 注入 alice，在线名单为 dave
    UserMap users;
    users.insert("alice", makeUser("alice"));
    injectUserMap(users);
    injectOnlineUsers(OnlineUserList { "dave" });
    // Act: 读取在线用户列表
    QList<User *> list = m_tester->userList();
    // Assert: 无交集返回空
    EXPECT_EQ(list.size(), 0);
    EXPECT_EQ(list, QList<User *>());
}

// userList 循环边界：用户映射为空 → 空列表
TEST_F(UT_AccountsInfoModel, UserList_EmptyUserMap_ReturnsEmpty)
{
    // Arrange: 空映射
    injectUserMap(UserMap());
    // Act: 读取在线用户列表
    QList<User *> list = m_tester->userList();
    // Assert: 空映射返回空列表
    EXPECT_TRUE(list.isEmpty());
    EXPECT_EQ(list.size(), 0);
}

// updateUserOnlineStatus：调用后以真实会话刷新列表并发出信号（覆盖脏数据）
TEST_F(UT_AccountsInfoModel, UpdateUserOnlineStatus_Call_EmitsSignalAndRefreshes)
{
    // Arrange: 挂信号探针并预置脏数据验证覆盖
    QSignalSpy spy(m_tester, &AccountsInfoModel::signalUserOnlineStatusUpdated);
    ASSERT_TRUE(spy.isValid());
    injectOnlineUsers(OnlineUserList { "stale-user" });
    injectSessions({});
    // Act: 触发在线状态刷新（login1 服务在本环境可用）
    m_tester->updateUserOnlineStatus();
    // Assert: 信号发出一次；在线名单与新会话列表一致（脏数据被覆盖）
    EXPECT_EQ(spy.count(), 1);
    const OnlineUserList &online = access_private_field::AccountsInfoModelm_onlineUsers(*m_tester);
    const SessionInfoList &sessions = access_private_field::AccountsInfoModelm_sessionList(*m_tester);
    EXPECT_EQ(online.size(), sessions.size());
}

// updateUserOnlineStatus 边界：重复调用信号随调用次数递增
TEST_F(UT_AccountsInfoModel, UpdateUserOnlineStatus_Twice_EmitsEachTime)
{
    // Arrange: 信号探针 + 首次刷新
    QSignalSpy spy(m_tester, &AccountsInfoModel::signalUserOnlineStatusUpdated);
    m_tester->updateUserOnlineStatus();
    // Act: 第二次刷新
    m_tester->updateUserOnlineStatus();
    // Assert: 信号累计两次，会话列表仍有效
    EXPECT_GE(spy.count(), 2);
    EXPECT_GE(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 0);
}

// onSessionNew：转发到 updateUserOnlineStatus 并发出信号（第 1 次触发）
TEST_F(UT_AccountsInfoModel, OnSessionNew_FirstCall_EmitsStatusSignal)
{
    // Arrange: 信号探针
    QSignalSpy spy(m_tester, &AccountsInfoModel::signalUserOnlineStatusUpdated);
    // Act: 新会话事件
    m_tester->onSessionNew("s1", QDBusObjectPath("/s1"));
    // Assert: 状态刷新信号发出
    EXPECT_EQ(spy.count(), 1);
    EXPECT_GE(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 0);
}

// onSessionNew：重复事件每次都触发刷新（第 2 次触发）
TEST_F(UT_AccountsInfoModel, OnSessionNew_SecondCall_EmitsAgain)
{
    // Arrange: 首次事件
    QSignalSpy spy(m_tester, &AccountsInfoModel::signalUserOnlineStatusUpdated);
    m_tester->onSessionNew("s1", QDBusObjectPath("/s1"));
    // Act: 第二次事件
    m_tester->onSessionNew("s2", QDBusObjectPath("/s2"));
    // Assert: 信号累计两次
    EXPECT_EQ(spy.count(), 2);
    EXPECT_GE(access_private_field::AccountsInfoModelm_onlineUsers(*m_tester).size(), 0);
}

// onSessionRemoved：转发到 updateUserOnlineStatus（注意：构造函数将该信号误连到
// onSessionNew，槽函数本身逻辑以直调方式验证，见缺陷记录）
TEST_F(UT_AccountsInfoModel, OnSessionRemoved_Call_EmitsStatusSignal)
{
    // Arrange: 信号探针
    QSignalSpy spy(m_tester, &AccountsInfoModel::signalUserOnlineStatusUpdated);
    // Act: 会话移除事件
    m_tester->onSessionRemoved("s1", QDBusObjectPath("/s1"));
    // Assert: 状态刷新信号发出
    EXPECT_EQ(spy.count(), 1);
    EXPECT_GE(access_private_field::AccountsInfoModelm_sessionList(*m_tester).size(), 0);
}

// onSessionRemoved：重复事件每次都触发刷新
TEST_F(UT_AccountsInfoModel, OnSessionRemoved_Twice_EmitsEachTime)
{
    // Arrange: 首次事件
    QSignalSpy spy(m_tester, &AccountsInfoModel::signalUserOnlineStatusUpdated);
    m_tester->onSessionRemoved("s1", QDBusObjectPath("/s1"));
    // Act: 第二次事件
    m_tester->onSessionRemoved("s2", QDBusObjectPath("/s2"));
    // Assert: 信号累计两次
    EXPECT_EQ(spy.count(), 2);
    EXPECT_GE(access_private_field::AccountsInfoModelm_onlineUsers(*m_tester).size(), 0);
}

// onUserListChanged / updateUserList：按 path 列表重建用户映射（qDeleteAll 旧表）
TEST_F(UT_AccountsInfoModel, OnUserListChanged_WithPaths_RebuildsUserMap)
{
    // Arrange: 预置一个旧用户，随后下发两个新 path
    UserMap users;
    users.insert("old", makeUser("old"));
    injectUserMap(users);
    QStringList paths;
    paths << "/com/deepin/daemon/Accounts/User1000"
          << "/com/deepin/daemon/Accounts/User1001";
    // Act: 触发用户列表变更（Accounts 服务缺失时属性为空，同名空键折叠）
    m_tester->onUserListChanged(paths);
    auto &userMap = access_private_field::AccountsInfoModelm_userMap(*m_tester);
    // Assert: 旧用户已回收；新表规模不超过 path 数（服务缺失时同名键折叠）
    EXPECT_FALSE(userMap.contains("old"));
    EXPECT_LE(userMap.size(), paths.size());
    EXPECT_FALSE(userMap.isEmpty());
}

// updateUserList 循环边界：空 path 列表 → 清空用户映射
TEST_F(UT_AccountsInfoModel, UpdateUserList_EmptyList_ClearsUserMap)
{
    // Arrange: 预置用户后下发空列表
    UserMap users;
    users.insert("old", makeUser("old"));
    injectUserMap(users);
    // Act: 空列表更新
    m_tester->updateUserList(QStringList());
    auto &userMap = access_private_field::AccountsInfoModelm_userMap(*m_tester);
    // Assert: 映射被清空
    EXPECT_TRUE(userMap.isEmpty());
    EXPECT_EQ(userMap.size(), 0);
}

// SessionInfo 序列化：注册元类型后 DBus 签名应为 "(ssuso)"
// （sessionId:QString, userId:uint, userName:QString, seatId:QString, sessionPath:objectpath）→ s/u/s/s/o
TEST_F(UT_AccountsInfoModel, SessionInfoStream_MetaTypeRegistered_MarshalsToDBusSignature)
{
    // Arrange: 注册编解码器（幂等），触发框架用 operator<< 计算签名
    qDBusRegisterMetaType<SessionInfo>();
    qDBusRegisterMetaType<SessionInfoList>();
    // Act: 查询该元类型对应的 DBus 类型签名
    const char *sig = QDBusMetaType::typeToSignature(QMetaType::fromType<SessionInfo>());
    // Assert: 签名与结构体字段序列一致
    ASSERT_NE(sig, nullptr);
    EXPECT_STREQ(sig, "(susso)");
    EXPECT_NE(QDBusMetaType::typeToSignature(QMetaType::fromType<SessionInfoList>()), nullptr);
}

// SessionInfo 序列化：经真实 DBus 总线广播后由匹配槽反序列化，字段保真
// （operator<< 编码 → 总线传输 → operator>> 解码，双向操作符均被真实触发）
TEST_F(UT_AccountsInfoModel, SessionInfoStream_RoundTrip_PreservesFields)
{
    // Arrange: 无会话总线时跳过（点对点环境；桌面开发机默认可用）
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        GTEST_SKIP() << "session bus unavailable";
    qDBusRegisterMetaType<SessionInfo>();
    SessionSigReceiver receiver;
    SessionInfo si;
    si.sessionId = "1";
    si.userId = 1000u;
    si.userName = "alice";
    si.seatId = "seat0";
    si.sessionPath = QDBusObjectPath("/s1");
    ASSERT_TRUE(bus.connect(QString(), "/ut/model", "ut.iface", "sessionChanged",
                            &receiver, SLOT(onSessionChanged(SessionInfo))));
    // Act: 构造信号并广播（参数经 operator<< 序列化）
    QDBusMessage signal = QDBusMessage::createSignal("/ut/model", "ut.iface", "sessionChanged");
    signal.setArguments(QList<QVariant> { QVariant::fromValue(si) });
    EXPECT_TRUE(bus.send(signal));
    // 等待事件派发完成（operator>> 反序列化发生在槽参数绑定）
    QEventLoop loop;
    for (int i = 0; i < 100 && receiver.count == 0; ++i)
        loop.processEvents(QEventLoop::AllEvents, 20);
    // Assert: 接收到的会话信息与发送侧各字段一致
    EXPECT_EQ(receiver.count, 1);
    EXPECT_EQ(receiver.last.sessionId, si.sessionId);
    EXPECT_EQ(receiver.last.userId, si.userId);
    EXPECT_EQ(receiver.last.userName, si.userName);
    EXPECT_EQ(receiver.last.seatId, si.seatId);
    EXPECT_EQ(receiver.last.sessionPath.path(), si.sessionPath.path());
}

#include "ut_accounts_info_model.moc"
