#include "accounts_info_model.h"
#include "helper.hpp"
#include "ddlog.h"
#include <QDBusReply>
#include <QDBusPendingReply>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <pwd.h>
#include <unistd.h>

using namespace DDLog;

#define PKEXEC_PATH "/usr/bin/pkexec"
#define PKILL_PATH "/usr/bin/pkill"

const QString LoginService = "org.freedesktop.login1";
const QString LoginPath = "/org/freedesktop/login1";
const QString LoginInterface = "org.freedesktop.login1.Manager";

AccountsInfoModel::AccountsInfoModel(QObject *parent)
    : QObject(parent), m_accountsInter(new QDBusInterface(common::systemInfo().AccountsService, common::systemInfo().AccountsPath, common::systemInfo().AccountsInterface, QDBusConnection::systemBus(), this)), m_LoginInter(new QDBusInterface(LoginService, LoginPath, LoginInterface, QDBusConnection::systemBus(), this)), m_controlInter(new QDBusInterface(common::systemInfo().ControlCenterService, common::systemInfo().ControlCenterPath, common::systemInfo().ControlCenterInterface, QDBusConnection::sessionBus(), this)), m_currentUserType(-1)
{
    qDBusRegisterMetaType<SessionInfo>();
    qDBusRegisterMetaType<SessionInfoList>();
    struct passwd *pws;
    pws = getpwuid(getuid());
    m_currentUserName = QString(pws->pw_name);
    //获取online userList
    updateUserOnlineStatus();

    QStringList userList = m_accountsInter->property("UserList").toStringList();
    updateUserList(userList);

    QDBusConnection::systemBus().connect(common::systemInfo().AccountsService, common::systemInfo().AccountsPath, common::systemInfo().AccountsInterface, "UserListChanged",
                                         this, SLOT(onUserListChanged(QStringList)));
    QDBusConnection::systemBus().connect(LoginService, LoginPath, LoginInterface, "SessionNew",
                                         this, SLOT(onSessionNew(QString, QDBusObjectPath)));
    QDBusConnection::systemBus().connect(LoginService, LoginPath, LoginInterface, "SessionRemoved",
                                         this, SLOT(onSessionNew(QString, QDBusObjectPath)));
}

AccountsInfoModel::~AccountsInfoModel()
{
    qDeleteAll(m_userMap.values());
    m_userMap.clear();
}

void AccountsInfoModel::onUserListChanged(const QStringList &userPathList)
{
    updateUserList(userPathList);
    qCInfo(app) << "get update:" << userPathList;
}

void AccountsInfoModel::onSessionNew(const QString &in0, const QDBusObjectPath &in1)
{
    Q_UNUSED(in0);
    Q_UNUSED(in1);
    updateUserOnlineStatus();
}

void AccountsInfoModel::onSessionRemoved(const QString &in0, const QDBusObjectPath &in1)
{
    Q_UNUSED(in0);
    Q_UNUSED(in1);
    updateUserOnlineStatus();
}

void AccountsInfoModel::updateUserList(const QStringList &userPathList)
{
    // 释放构造对象
    qDeleteAll(m_userMap.values());
    m_userMap.clear();

    for (QString userPath : userPathList) {
        QDBusInterface *userDBus = new QDBusInterface(common::systemInfo().AccountsService, userPath, common::systemInfo().UserInterface, QDBusConnection::systemBus(), this);
        User *newUser = new User;

        newUser->setName(userDBus->property("UserName").toString());
        newUser->setFullname(userDBus->property("FullName").toString());
        newUser->setIconFile(userDBus->property("IconFile").toString());
        newUser->setUserType(userDBus->property("AccountType").toInt());
        newUser->setUserUid(userDBus->property("Uid").toString());

        //当前用户
        if (newUser->name() == m_currentUserName) {
            newUser->setIsCurrentUser(true);
            m_currentUserType = newUser->userType();
            qCInfo(app) << "Current user type set to:" << m_currentUserType;
        }
        m_userMap.insert(newUser->name(), newUser);

        delete userDBus;
    }
}

void AccountsInfoModel::updateUserOnlineStatus()
{
    m_onlineUsers.clear();

    //异步获取SessionList
    QDBusMessage message = m_LoginInter->call("ListSessions");
    QDBusPendingReply<SessionInfoList> reply = m_LoginInter->asyncCall(QStringLiteral("ListSessions"));
    reply.waitForFinished();
    if (reply.isValid()) {
        m_sessionList = reply.value();
        qCInfo(app) << "Retrieved" << m_sessionList.size() << "active sessions";
    } else {
        qCWarning(app) << "Failed to retrieve session list:" << reply.error().message();
    }
    
    for (auto session : m_sessionList) {
        m_onlineUsers << session.userName;
    }

    Q_EMIT signalUserOnlineStatusUpdated();
}

QList<User *> AccountsInfoModel::userList() const
{
    QList<User *> onlineUsers;
    //筛选已登录用户
    for (auto user : m_userMap.values()) {
        if (m_onlineUsers.contains(user->name())) {
            user->setOnline(true);
            onlineUsers << user;
        }
    }

    return onlineUsers;
}

User::UserType AccountsInfoModel::getCurrentUserType() const
{
    if (m_currentUserType == -1) {
        return User::UserType::Customized;
    } else if (m_currentUserType == 0) {
        return User::UserType::StandardUser;
    } else if (m_currentUserType == 1) {
        return User::UserType::Administrator;
    } else {
        return User::UserType::StandardUser;
    }
}

bool AccountsInfoModel::lockSessionByUserName(const QString &userName)
{
    qCInfo(app) << "Attempting to lock session for user:" << userName;
    if (m_sessionList.size() > 0) {
        for (SessionInfo si : m_sessionList) {
            if (si.userName == userName) {
                m_LoginInter->call("LockSession", si.sessionId);
                return true;
            }
        }
        qCWarning(app) << "Cannot find session for username:" << userName;
        return false;
    } else {
        qCWarning(app) << "No active sessions found";
        return false;
    }
}

bool AccountsInfoModel::activateSessionByUserName(const QString &userName)
{
    qCInfo(app) << "Attempting to activate session for user:" << userName;
    if (m_sessionList.size() > 0) {
        for (SessionInfo si : m_sessionList) {
            if (si.userName == userName) {

                m_LoginInter->call("ActivateSession", si.sessionId);
                return true;
            }
        }
        qCWarning(app) << "Cannot find session for username:" << userName;
        return false;
    } else {
        qCWarning(app) << "No active sessions found";
        return false;
    }
}

bool AccountsInfoModel::LogoutByUserName(const QString &userName)
{
    qCInfo(app) << "Attempting to logout user:" << userName;
    if (userName == m_currentUserName) {
        if (m_sessionList.size() > 0) {
            qCInfo(app) << "Processing logout for current user";
            for (SessionInfo si : m_sessionList) {
                if (si.userName == userName) {
                    qCInfo(app) << "Found session for current user:" << userName << "Session ID:" << si.sessionId;
                    return true;
                }
            }
            qCWarning(app) << "Cannot find session for current username:" << userName;
        } else {
            qCWarning(app) << "No active sessions found for logout";
        }
    } else {
        QStringList params;

        // check pkexec existance
        if (!QFile::exists({ PKEXEC_PATH })) {
            qCWarning(app) << "pkexec not found at:" << PKEXEC_PATH;
            return false;
        }

        // check kill existance
        if (!QFile::exists({ PKILL_PATH })) {
            qCWarning(app) << "pkill not found at:" << PKILL_PATH;
            return false;
        }

        params << QString(PKILL_PATH) << QString("-u") << userName;
        qCInfo(app) << "Executing pkexec with params:" << params.join(" ");

        QProcess proc;
        proc.start({ PKEXEC_PATH }, params);
        proc.waitForFinished();
        return true;
    }
    return false;
}

void AccountsInfoModel::EditAccount()
{
    qCInfo(app) << "Opening account settings";
    if (m_controlInter) {
        bool version = common::systemInfo().isOldVersion();
        if (version) {
            qCInfo(app) << "Using old version interface (v20)";
            m_controlInter->call("ShowPage", "accounts", "Accounts Detail");
        } else {
            qCInfo(app) << "Using new version interface (v23)";
            m_controlInter->call("ShowPage", "accounts");
        }
    } else {
        qCWarning(app) << "Control center interface not available";
    }
}
