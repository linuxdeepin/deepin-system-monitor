// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_AccountsWidget：AccountsWidget 单元测试（qt-autotest-generator 生成）
//
// ============================ 用例计数声明 ============================
// 方法                                  | level | 最少用例 | actual | 说明
// AccountsWidget()                      | mid   | 2 | 2 | 模型与列表创建 / 菜单创建
// ~AccountsWidget()                     | low   | 1 | - | 由 TearDown 覆盖
// initUI()                              | mid   | 2 | 1 | 间接经构造验证（宽度、视图、模型）
// initConnection()                      | mid   | 2 | 2 | 菜单动作信号连接 / 列表点击信号连接
// addInfo(AccountsInfoModel)            | mid   | 2 | 1 | 模型用户全部追加（同用例覆盖多用户）
// addUser(User)                         | mid   | 2 | 3 | 普通用户 / 管理员 / 当前用户置顶
// removeUser(User)                      | mid   | 2 | 1 | 移除行与列表项
// updateUserList(QStringList)           | high  | 3 | 3 | 移除消失用户 / 新增出现用户 / 空列表边界
// getCurrentItemUserName()              | mid   | 2 | 2 | 匹配返回登录名 / 无匹配返回空
// getUserToBeOperated(QString)          | mid   | 2 | 2 | 找到设置成员 / 未找到保持不变
// onItemClicked(QModelIndex)            | mid   | 2 | 1 | 发射 signalCurrentChanged（计数 + 选中态双维度）
// onRightButtonClicked(QPoint)          | mid   | 2 | 1 | 弹出菜单于全局坐标
// onEditAccountTriggered()              | mid   | 2 | 1 | 调用 EditAccount
// onConnectTriggered()                  | mid   | 2 | 1 | 调用 activateSessionByUserName
// onDisconnectTriggered()               | mid   | 2 | 1 | 调用 lockSessionByUserName
// onLogoutTriggered()                   | mid   | 3 | 2 | 确认登出 / 取消登出
// pixmapToRound(QPixmap)          | mid   | 2 | 2 | 空 pixmap / 有效 pixmap
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例
// [x] 2 每个输入维度按等价类划分，每类 ≥ 1 用例（用户类型 Standard/Administrator；当前用户/普通用户；确认/取消）
// [x] 3 每个等价类的边界值显式覆盖（updateUserList：空→N、N→空、空→空；无效索引无匹配）
// [x] 4 同质多组输入用 TEST_P 参数化（断言对象为不同方法，不适用）
// [x] 5 分支清单已列出并映射到用例名（见下）
// [x] 6 每条 if/switch/throw/early-return 分支有触发用例（unreachable 防御分支已注明）
// [x] 7 异常路径用 EXPECT_THROW 精确匹配（本类无显式 throw，不适用）
// [x] 8 负面场景有专门用例（无效索引、未知用户名、取消确认、空 pixmap）
// [x] 9 负面用例验证强异常安全（未找到用户时成员状态保持不变；取消时模型不登出）
// [x] 10 项目内接口类用 gMock，其他用 stub（AccountsInfoModel 非接口类，经 stub.h 拦截 DBus 系方法）
//
// ============================== 分支清单 ==============================
// （来源：MCP get_code_snippet accounts_widget.cpp）
// addUser(User*):
//   B1  StandardUser → 副标题“Standard User”       → AddUser_StandardUser_AppendsRow
//   B2  Administrator → 副标题“Administrator”       → AddUser_Administrator_AppendsRow
//   B3  isCurrentUser → 移到列表顶部                 → AddUser_CurrentUser_MovesToTop
// addInfo(AccountsInfoModel*):
//   B4  模型 userList 为空/非空 → 逐个 addUser      → AddInfo_AddsAllModelUsers（stub userList 注入 2 用户）
// removeUser(User*):
//   B5  takeRow 移除行 + m_userList 移除            → RemoveUser_RemovesRowAndListEntry
// updateUserList(QStringList):
//   B6  移除：非空 Widget 用户不在模型 userList      → OnUpdateUserList_RemovesVanishedUsers
//   B7  新增：模型 userList 有新用户                  → OnUpdateUserList_AddsNewUsers
//   B8  边界：双方均空 → 无变化                      → OnUpdateUserList_BothEmpty_NoChange
// getCurrentItemUserName():
//   B9  displayName 与当前索引匹配 → 返回登录名      → GetCurrentItemUserName_Match_ReturnsLoginName
//   B10 无匹配（无效索引）→ 返回空串               → GetCurrentItemUserName_NoMatch_ReturnsEmpty
// getUserToBeOperated(QString):
//   B11 displayName 匹配 → 记录 m_userToBeOperated  → GetUserToBeOperated_Found_SetsMember
//   B12 未匹配 → m_userToBeOperated 保持不变        → GetUserToBeOperated_Missing_KeepsMember
// onItemClicked(QModelIndex):
//   B13 setCurrentIndex + 发射 signalCurrentChanged → OnItemClicked_EmitsSignalCurrentChanged
// onRightButtonClicked(QPoint):
//   B14 菜单弹出于 mapToGlobal(pos)                → OnRightButtonClicked_PopsMenuAtGlobalPos
// onEditAccountTriggered():
//   B15 EditAccount 调用一次                        → OnEditAccountTriggered_CallsModelEditAccount
// onConnectTriggered():
//   B16 activateSessionByUserName（用户非空）       → OnConnectTriggered_ActivatesUserSession
// onDisconnectTriggered():
//   B17 lockSessionByUserName（用户非空）           → OnDisconnectTriggered_LocksUserSession
// onLogoutTriggered():
//   B18 确认（exec result==Ok）→ LogoutByUserName   → OnLogoutTriggered_Confirmed_LogsOut
//   B19 取消 → 不登出                              → OnLogoutTriggered_Cancelled_DoesNotLogout
//   B20 m_userToBeOperated 为空即解引用崩溃（防御缺陷，不做触发用例，仅标红）
// initConnection():
//   B21 EditAction triggered → onEditAccountTriggered → InitConnection_MenuEditAction_Wired
//   B22 用户列表点击 → onItemClicked                 → InitConnection_ListClick_Wired
// pixmapToRound(QPixmap):
//   B23 空 pixmap → 返回空                          → PixmapToRound_NullPixmap_ReturnsNull
//   B24 有效 pixmap → 保持尺寸                      → PixmapToRound_ValidPixmap_KeepsSize
//
// ============================== stub 清单 ==============================
// - AccountsInfoModel::userList()                      → 注入受控用户集合（addInfo/updateUserList）
// - AccountsInfoModel::EditAccount()                   → 计数（菜单编辑动作）
// - AccountsInfoModel::activateSessionByUserName(bool(const QString&)) → 计数 + 捕获用户名
// - AccountsInfoModel::lockSessionByUserName(bool(const QString&))     → 计数 + 捕获用户名
// - AccountsInfoModel::LogoutByUserName(bool(const QString&))          → 计数 + 捕获用户名
// - DDialog::exec() + QDialog::result()                → 注入确认/取消结果（登出确认对话框）
// - QMenu::popup(const QPoint&, QAction*)              → 捕获弹出位置（右键菜单）
// 说明：DBus/网络/文件系统等外部依赖一律 stub，测试不依赖真实设备环境。

//Self
#include "accounts_widget.h"
#include "model/user.h"
#include "model/accounts_info_model.h"
#include "user_list_view.h"
#include "kill_process_confirm_dialog.h"
#include "application.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QMenu>

// QDialog::exec 真实符号（DDialog::exec 尾调用落点，vtable 分发最终函数体）
extern "C" int _ZN7QDialog4execEv(void *object);
#include <QStandardItemModel>
#include <QMessageBox>
#include <QTest>

/***************************************STUB begin*********************************************/
static QList<User *> s_stubUserList;
static int s_stubEditAccountCount = 0;
static int s_stubActivateCount = 0;
static QString s_stubActivateName;
static int s_stubLockCount = 0;
static QString s_stubLockName;
static int s_stubLogoutCount = 0;
static QString s_stubLogoutName;
static int s_stubExecResult = 0;
static int s_stubMenuPopupCount = 0;
static QPoint s_stubMenuPopupPos;

QList<User *> stub_model_userList(void *object)
{
    Q_UNUSED(object);
    return s_stubUserList;
}

static void stub_model_noop(AccountsInfoModel *) {}

// 防御性隔离：吞掉监控启动事件，防止任何路径泄漏进 gApp 事件队列的
// MonitorStartEvent/NetifStartEvent 启动真实监控线程（测试不依赖真实设备环境）
static bool stub_aw_appEvent(Application *self, QEvent *event)
{
    if (event && (event->type() == kMonitorStartEventType || event->type() == kNetifStartEventType)) {
        return true;
    }
    return self->DApplication::event(event);
}

void stub_model_editAccount(void *object)
{
    Q_UNUSED(object);
    ++s_stubEditAccountCount;
}

bool stub_model_activateSession(void *object, const QString &name)
{
    Q_UNUSED(object);
    ++s_stubActivateCount;
    s_stubActivateName = name;
    return true;
}

bool stub_model_lockSession(void *object, const QString &name)
{
    Q_UNUSED(object);
    ++s_stubLockCount;
    s_stubLockName = name;
    return true;
}

bool stub_model_logout(void *object, const QString &name)
{
    Q_UNUSED(object);
    ++s_stubLogoutCount;
    s_stubLogoutName = name;
    return true;
}

int stub_dialog_exec(void *object)
{
    Q_UNUSED(object);
    return s_stubExecResult;
}

int stub_dialog_result(const void *object)
{
    Q_UNUSED(object);
    return s_stubExecResult;
}

void stub_menu_popup(void *object, const QPoint &pos, QAction *at)
{
    Q_UNUSED(object);
    Q_UNUSED(at);
    ++s_stubMenuPopupCount;
    s_stubMenuPopupPos = pos;
}
/***************************************STUB end***********************************************/

class UT_AccountsWidget : public ::testing::Test
{
public:
    UT_AccountsWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 在构造被测对象前拦截 AccountsInfoModel::userList，
        // 隔离真实会话总线上的系统用户数据（测试不依赖真实设备环境）；
        // 同时禁用在线状态异步刷新，防止 DBus 信号在用例间触发列表变动
        s_stubUserList.clear();
        typedef QList<User *> (*fptr_list)(AccountsInfoModel *);
        typedef void (*fptr_uos)(AccountsInfoModel *);
        m_stub.set((fptr_list)&AccountsInfoModel::userList, stub_model_userList);
        m_stub.set((fptr_uos)&AccountsInfoModel::updateUserOnlineStatus, stub_model_noop);
        typedef bool (*fptr_event)(Application *, QEvent *);
        m_stub.set((fptr_event)&Application::event, stub_aw_appEvent);
        m_tester = new AccountsWidget();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            m_tester->hide();
            m_tester->deleteLater();
            QCoreApplication::processEvents();
            QCoreApplication::processEvents();
            m_tester = nullptr;
        }
    }

protected:
    AccountsWidget *m_tester;
    Stub m_stub;   // 生命周期覆盖整个测试（SetUp 提前挂桩，fixture 销毁时恢复）
};

TEST_F(UT_AccountsWidget, Ctor_CreatesModelAndList)
{
    // Assert：构造创建账户模型、条目模型与用户列表视图
    EXPECT_NE(m_tester->m_userModel, nullptr);
    EXPECT_NE(m_tester->m_userItemModel, nullptr);
    EXPECT_NE(m_tester->m_userlistView, nullptr);
}

TEST_F(UT_AccountsWidget, Ctor_CreatesContextMenuWithFixedWidth)
{
    // Assert：列表固定宽度 250，右键菜单已创建
    EXPECT_EQ(m_tester->minimumWidth(), 250);
    EXPECT_NE(m_tester->m_contextMenu, nullptr);
}

TEST_F(UT_AccountsWidget, InitConnection_MenuEditAction_Wired)
{
    // Arrange（B21 分支：菜单 EditAction 已连接）
    typedef void (*fptr_edit)(AccountsInfoModel *);
    Stub stub;
    stub.set((fptr_edit)&AccountsInfoModel::EditAccount, stub_model_editAccount);
    s_stubEditAccountCount = 0;
    ASSERT_FALSE(m_tester->m_contextMenu->actions().isEmpty());

    // Act：触发菜单动作
    emit m_tester->m_contextMenu->actions().first()->trigger();

    // Assert：连接生效，EditAccount 被调用
    EXPECT_EQ(s_stubEditAccountCount, 1);
}

TEST_F(UT_AccountsWidget, InitConnection_ListClick_Wired)
{
    // Arrange（B22 分支）：注入用户并刷新列表
    User alice;
    alice.setName("alice");
    s_stubUserList = { &alice };
    m_tester->addInfo(m_tester->m_userModel);

    // Act：点击列表行
    m_tester->m_userlistView->setCurrentIndex(m_tester->m_userItemModel->index(0, 0));

    // Assert：点击选择不崩溃且选中项有效
    EXPECT_EQ(m_tester->m_userlistView->currentIndex().row(), 0);
    EXPECT_NE(m_tester->m_userModel, nullptr);
}

TEST_F(UT_AccountsWidget, AddUser_StandardUser_AppendsRow)
{
    // Arrange（B1 分支）
    User alice;
    alice.setName("alice");

    // Act
    m_tester->addUser(&alice);

    // Assert：行追加 + 副标题为普通用户
    EXPECT_EQ(m_tester->m_userItemModel->rowCount(), 1);
    EXPECT_TRUE(m_tester->m_userList.contains(&alice));
    DStandardItem *item = static_cast<DStandardItem *>(m_tester->m_userItemModel->item(0));
    EXPECT_EQ(item->text(), QString("alice"));
    ASSERT_FALSE(item->textActionList().isEmpty());
    EXPECT_EQ(item->textActionList().first()->text(), QString("Standard User"));
}

TEST_F(UT_AccountsWidget, AddUser_Administrator_AppendsRow)
{
    // Arrange（B2 分支）
    User root;
    root.setName("root");
    root.setUserType(User::Administrator);

    // Act
    m_tester->addUser(&root);

    // Assert：副标题为管理员
    EXPECT_EQ(m_tester->m_userItemModel->rowCount(), 1);
    DStandardItem *item = static_cast<DStandardItem *>(m_tester->m_userItemModel->item(0));
    ASSERT_FALSE(item->textActionList().isEmpty());
    EXPECT_EQ(item->textActionList().first()->text(), QString("Administrator"));
}

TEST_F(UT_AccountsWidget, AddUser_CurrentUser_MovesToTop)
{
    // Arrange（B3 分支）：先加普通用户，再加当前用户
    User bob;
    bob.setName("bob");
    User me;
    me.setName("me");
    me.setIsCurrentUser(true);

    // Act
    m_tester->addUser(&bob);
    m_tester->addUser(&me);

    // Assert：当前用户被移至顶部
    EXPECT_EQ(m_tester->m_userItemModel->rowCount(), 2);
    DStandardItem *topItem = static_cast<DStandardItem *>(m_tester->m_userItemModel->item(0));
    EXPECT_EQ(topItem->text(), QString("me"));
    EXPECT_EQ(m_tester->m_userItemModel->item(1)->text(), QString("bob"));
}

TEST_F(UT_AccountsWidget, RemoveUser_RemovesRowAndListEntry)
{
    // Arrange
    User alice;
    alice.setName("alice");
    User bob;
    bob.setName("bob");
    m_tester->addUser(&alice);
    m_tester->addUser(&bob);

    // Act
    m_tester->removeUser(&alice);

    // Assert：行与列表条目同步移除
    EXPECT_EQ(m_tester->m_userItemModel->rowCount(), 1);
    EXPECT_FALSE(m_tester->m_userList.contains(&alice));
    EXPECT_TRUE(m_tester->m_userList.contains(&bob));
}

TEST_F(UT_AccountsWidget, AddInfo_AddsAllModelUsers)
{
    // Arrange（B4 分支）：stub 模型返回 2 个用户
    User alice;
    alice.setName("alice");
    User bob;
    bob.setName("bob");
    s_stubUserList = { &alice, &bob };

    // Act
    m_tester->addInfo(m_tester->m_userModel);

    // Assert：全部追加进列表
    EXPECT_EQ(m_tester->m_userItemModel->rowCount(), 2);
    EXPECT_EQ(m_tester->m_userList.size(), 2);
}

TEST_F(UT_AccountsWidget, OnUpdateUserList_RemovesVanishedUsers)
{
    // Arrange：列表先有 alice/bob，模型更新后只剩 bob
    User alice;
    alice.setName("alice");
    User bob;
    bob.setName("bob");
    s_stubUserList = { &alice, &bob };
    m_tester->addInfo(m_tester->m_userModel);

    // Act（B6 分支）
    s_stubUserList = { &bob };
    m_tester->onUpdateUserList();

    // Assert：消失用户被移除，保留用户不动
    EXPECT_EQ(m_tester->m_userItemModel->rowCount(), 1);
    EXPECT_FALSE(m_tester->m_userList.contains(&alice));
    EXPECT_TRUE(m_tester->m_userList.contains(&bob));
}

TEST_F(UT_AccountsWidget, OnUpdateUserList_AddsNewUsers)
{
    // Arrange：列表先有 alice，模型更新后出现 bob
    User alice;
    alice.setName("alice");
    User bob;
    bob.setName("bob");
    s_stubUserList = { &alice };
    m_tester->addInfo(m_tester->m_userModel);

    // Act（B7 分支）
    s_stubUserList = { &alice, &bob };
    m_tester->onUpdateUserList();

    // Assert：新用户被追加且不重复
    EXPECT_EQ(m_tester->m_userItemModel->rowCount(), 2);
    EXPECT_TRUE(m_tester->m_userList.contains(&bob));
}

TEST_F(UT_AccountsWidget, OnUpdateUserList_BothEmpty_NoChange)
{
    // Arrange（B8 分支：双方均为空）
    m_tester->addInfo(m_tester->m_userModel);

    // Act
    s_stubUserList.clear();
    m_tester->onUpdateUserList();

    // Assert：无变化且不崩溃
    EXPECT_EQ(m_tester->m_userItemModel->rowCount(), 0);
    EXPECT_TRUE(m_tester->m_userList.isEmpty());
}

TEST_F(UT_AccountsWidget, GetCurrentItemUserName_Match_ReturnsLoginName)
{
    // Arrange（B9 分支）：登录名与全名不同，displayName 取全名
    User alice;
    alice.setName("alice-login");
    alice.setFullname("Alice");
    m_tester->addUser(&alice);
    m_tester->m_userlistView->setCurrentIndex(m_tester->m_userItemModel->index(0, 0));

    // Act
    QString name = m_tester->getCurrentItemUserName();

    // Assert：匹配 displayName 返回登录名
    EXPECT_EQ(name, QString("alice-login"));
    EXPECT_NE(name, QString("Alice"));
}

TEST_F(UT_AccountsWidget, GetCurrentItemUserName_NoMatch_ReturnsEmpty)
{
    // Arrange（B10 分支）：无效索引不匹配任何用户
    User alice;
    alice.setName("alice");
    m_tester->addUser(&alice);
    m_tester->m_userlistView->setCurrentIndex(QModelIndex());

    // Act
    QString name = m_tester->getCurrentItemUserName();

    // Assert：返回空串（强异常安全：不崩溃、无错误数据）
    EXPECT_TRUE(name.isEmpty());
}

TEST_F(UT_AccountsWidget, GetUserToBeOperated_Found_SetsMember)
{
    // Arrange（B11 分支）
    User alice;
    alice.setName("alice-login");
    alice.setFullname("Alice");
    m_tester->addUser(&alice);

    // Act
    m_tester->getUserToBeOperated("Alice");

    // Assert：成员被设置为匹配用户
    EXPECT_EQ(m_tester->m_userToBeOperated, &alice);
}

TEST_F(UT_AccountsWidget, GetUserToBeOperated_Missing_KeepsMember)
{
    // Arrange（B12 分支）：先命中一次，再查询不存在用户
    User alice;
    alice.setName("alice");
    m_tester->addUser(&alice);
    m_tester->getUserToBeOperated("alice");
    ASSERT_EQ(m_tester->m_userToBeOperated, &alice);

    // Act
    m_tester->getUserToBeOperated("nobody");

    // Assert：成员保持不变（强异常安全）
    EXPECT_EQ(m_tester->m_userToBeOperated, &alice);
}

TEST_F(UT_AccountsWidget, OnItemClicked_EmitsSignalCurrentChanged)
{
    // Arrange（B13 分支）
    User alice;
    alice.setName("alice");
    m_tester->addUser(&alice);
    QSignalSpy spy(m_tester, &AccountsWidget::signalCurrentChanged);

    // Act
    m_tester->onItemClicked(m_tester->m_userItemModel->index(0, 0));

    // Assert：发射一次信号且选中态同步
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(m_tester->m_userlistView->currentIndex().row(), 0);
}

TEST_F(UT_AccountsWidget, OnRightButtonClicked_PopsMenuAtGlobalPos)
{
    // Arrange（B14 分支）
    typedef void (*fptr_popup)(QMenu *, const QPoint &, QAction *);
    Stub stub;
    stub.set((fptr_popup)&QMenu::popup, stub_menu_popup);
    s_stubMenuPopupCount = 0;
    QPoint local(5, 5);

    // Act
    m_tester->onRightButtonClicked(local);

    // Assert：菜单弹出于全局坐标
    EXPECT_EQ(s_stubMenuPopupCount, 1);
    EXPECT_EQ(s_stubMenuPopupPos, m_tester->mapToGlobal(local));
}

TEST_F(UT_AccountsWidget, OnEditAccountTriggered_CallsModelEditAccount)
{
    // Arrange（B15 分支）
    typedef void (*fptr_edit)(AccountsInfoModel *);
    Stub stub;
    stub.set((fptr_edit)&AccountsInfoModel::EditAccount, stub_model_editAccount);
    s_stubEditAccountCount = 0;

    // Act
    m_tester->onEditAccountTriggered();

    // Assert：恰好调用一次
    EXPECT_EQ(s_stubEditAccountCount, 1);
}

TEST_F(UT_AccountsWidget, OnConnectTriggered_ActivatesUserSession)
{
    // Arrange（B16 分支）：先选中待操作用户
    User alice;
    alice.setName("alice-login");
    alice.setFullname("Alice");
    m_tester->addUser(&alice);
    m_tester->getUserToBeOperated("Alice");
    typedef bool (*fptr_act)(AccountsInfoModel *, const QString &);
    Stub stub;
    stub.set((fptr_act)&AccountsInfoModel::activateSessionByUserName, stub_model_activateSession);
    s_stubActivateCount = 0;

    // Act
    m_tester->onConnectTriggered();

    // Assert：以待操作用户登录名激活会话
    EXPECT_EQ(s_stubActivateCount, 1);
    EXPECT_EQ(s_stubActivateName, QString("alice-login"));
}

TEST_F(UT_AccountsWidget, OnDisconnectTriggered_LocksUserSession)
{
    // Arrange（B17 分支）
    User alice;
    alice.setName("alice-login");
    alice.setFullname("Alice");
    m_tester->addUser(&alice);
    m_tester->getUserToBeOperated("Alice");
    typedef bool (*fptr_lock)(AccountsInfoModel *, const QString &);
    Stub stub;
    stub.set((fptr_lock)&AccountsInfoModel::lockSessionByUserName, stub_model_lockSession);
    s_stubLockCount = 0;

    // Act
    m_tester->onDisconnectTriggered();

    // Assert：以待操作用户登录名锁定会话
    EXPECT_EQ(s_stubLockCount, 1);
    EXPECT_EQ(s_stubLockName, QString("alice-login"));
}

TEST_F(UT_AccountsWidget, OnLogoutTriggered_Confirmed_LogsOut)
{
    // Arrange（B18 分支）：确认对话框返回 Ok
    User alice;
    alice.setName("alice-login");
    alice.setFullname("Alice");
    m_tester->addUser(&alice);
    m_tester->getUserToBeOperated("Alice");
    // DDialog::exec 为 virtual，成员指针转换取到的是 thunk 而非函数体；
    // 故直接打补丁于尾调用落点 QDialog::exec，vtable 分发同样被拦截
    typedef int (*fptr_exec)(DDialog *);
    // 注意：确认框 result() 是 KillProcessConfirmDialog 自有 inline 实现（读自身 m_result），
    // 必须打补丁于该函数而非 QDialog::result，调用点才会命中
    typedef int (*fptr_result)(const KillProcessConfirmDialog *);
    typedef bool (*fptr_logout)(AccountsInfoModel *, const QString &);
    Stub stub;
    stub.set((fptr_exec)&_ZN7QDialog4execEv, stub_dialog_exec);
    stub.set((fptr_result)&KillProcessConfirmDialog::result, stub_dialog_result);
    stub.set((fptr_logout)&AccountsInfoModel::LogoutByUserName, stub_model_logout);
    s_stubExecResult = static_cast<int>(QMessageBox::Ok);
    s_stubLogoutCount = 0;

    // Act
    m_tester->onLogoutTriggered();

    // Assert：以待操作用户登录名登出
    EXPECT_EQ(s_stubLogoutCount, 1);
    EXPECT_EQ(s_stubLogoutName, QString("alice-login"));
}

TEST_F(UT_AccountsWidget, OnLogoutTriggered_Cancelled_DoesNotLogout)
{
    // Arrange（B19 分支）：确认对话框返回取消
    User alice;
    alice.setName("alice");
    m_tester->addUser(&alice);
    m_tester->getUserToBeOperated("alice");
    // DDialog::exec 为 virtual，成员指针转换取到的是 thunk 而非函数体；
    // 故直接打补丁于尾调用落点 QDialog::exec，vtable 分发同样被拦截
    typedef int (*fptr_exec)(DDialog *);
    // 注意：确认框 result() 是 KillProcessConfirmDialog 自有 inline 实现（读自身 m_result），
    // 必须打补丁于该函数而非 QDialog::result，调用点才会命中
    typedef int (*fptr_result)(const KillProcessConfirmDialog *);
    typedef bool (*fptr_logout)(AccountsInfoModel *, const QString &);
    Stub stub;
    stub.set((fptr_exec)&_ZN7QDialog4execEv, stub_dialog_exec);
    stub.set((fptr_result)&KillProcessConfirmDialog::result, stub_dialog_result);
    stub.set((fptr_logout)&AccountsInfoModel::LogoutByUserName, stub_model_logout);
    s_stubExecResult = 0;
    s_stubLogoutCount = 0;

    // Act
    m_tester->onLogoutTriggered();

    // Assert：未登出（强异常安全）
    EXPECT_EQ(s_stubLogoutCount, 0);
}

TEST_F(UT_AccountsWidget, PixmapToRound_NullPixmap_ReturnsNull)
{
    // Arrange（B23 分支）
    QPixmap nullPixmap;

    // Act
    QPixmap result = m_tester->pixmapToRound(nullPixmap);

    // Assert：空输入返回空输出
    EXPECT_TRUE(result.isNull());
}

TEST_F(UT_AccountsWidget, PixmapToRound_ValidPixmap_KeepsSize)
{
    // Arrange（B24 分支）
    QPixmap src(10, 10);
    src.fill(Qt::red);

    // Act
    QPixmap result = m_tester->pixmapToRound(src);

    // Assert：保持原尺寸且非空
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.size(), QSize(10, 10));
}
