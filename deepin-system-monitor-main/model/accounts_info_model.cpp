#include "accounts_info_model.h"
#include "helper.hpp"
#include <QDBusReply>
#include <QDBusPendingReply>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <pwd.h>
#include <unistd.h>

#define PKEXEC_PATH "/usr/bin/pkexec"
#define PKILL_PATH "/usr/bin/pkill"

const QString LoginService = "org.freedesktop.login1";
const QString LoginPath = "/org/freedesktop/login1";
const QString LoginInterface = "org.freedesktop.login1.Manager";

AccountsInfoModel::AccountsInfoModel(QObject *parent): QObject(parent)
    , m_accountsInter(new QDBusInterface(common::systemInfo().AccountsService, common::systemInfo().AccountsPath, common::systemInfo().AccountsInterface, QDBusConnection::systemBus(), this))
    , m_LoginInter(new QDBusInterface(LoginService, LoginPath, LoginInterface, QDBusConnection::systemBus(), this))
    , m_controlInter(new QDBusInterface(common::systemInfo().ControlCenterService, common::systemInfo().ControlCenterPath, common::systemInfo().ControlCenterInterface, QDBusConnection::sessionBus(), this))
    , m_currentUserType(-1)
{
    qDBusRegisterMetaType<SessionInfo>();
    qDBusRegisterMetaType<SessionInfoList>();
    struct passwd *pws;
    pws = getpwuid(getuid());
    m_currentUserName = QString(pws->pw_name);
    qInfo() << "AccountsInfoModel constructor line 26" << "m_currentUserName:" << m_currentUserName;
    //获取online userList
    updateUserOnlineStatus();
    qInfo() << "AccountsInfoModel constructor line 29" << "online user list:" << m_onlineUsers;

    QStringList userList = m_accountsInter->property("UserList").toStringList();
    updateUserList(userList);
    qInfo() << "AccountsInfoModel constructor line 31" << "Accounts user list:" << userList;

    QDBusConnection::systemBus().connect(common::systemInfo().AccountsService, common::systemInfo().AccountsPath, common::systemInfo().AccountsInterface, "UserListChanged",
                                         this, SLOT(onUserListChanged(QStringList)));
    QDBusConnection::systemBus().connect(LoginService, LoginPath, LoginInterface, "SessionNew",
                                         this, SLOT(onSessionNew(QString, QDBusObjectPath)));
    QDBusConnection::systemBus().connect(LoginService, LoginPath, LoginInterface, "SessionRemoved",
                                         this, SLOT(onSessionNew(QString, QDBusObjectPath)));
}

void AccountsInfoModel::onUserListChanged(const QStringList &userPathList)
{
    updateUserList(userPathList);
    qInfo() << "get update:" << userPathList;
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
    qInfo() << "AccountsInfoModel updateUserList line 61" << "updateUserList begins!" ;
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
        }
        qInfo() << "AccountsInfoModel updateUserList line 78" << "get user info of :" << newUser->name();
        m_userMap.insert(newUser->name(), newUser);
    }
}


void AccountsInfoModel::updateUserOnlineStatus()
{
    qInfo() << "AccountsInfoModel updateUserOnlineStatus line 88" << "updateUserOnlineStatus begins!" ;
    m_onlineUsers.clear();

    //异步获取SessionList
    QDBusMessage message =  m_LoginInter->call("ListSessions");
    QDBusPendingReply<SessionInfoList> reply = m_LoginInter->asyncCall(QStringLiteral("ListSessions"));
    reply.waitForFinished();
    if (reply.isValid()) {
        m_sessionList = reply.value();
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
    qInfo() << "AccountsInfoModel userList" << "get online users:" << m_onlineUsers;

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
    }
}

bool AccountsInfoModel::lockSessionByUserName(const QString &userName)
{
    if (m_sessionList.size() > 0) {
        for (SessionInfo si : m_sessionList) {
            if (si.userName == userName) {
                m_LoginInter->call("LockSession", si.sessionId);
                return true;
            }
        }
        qDebug() << "cannot find session with this username!";
        return false;
    } else {
        qDebug() << "no session found!";
        return false;
    }

}


bool AccountsInfoModel::activateSessionByUserName(const QString &userName)
{

    if (m_sessionList.size() > 0) {

        for (SessionInfo si : m_sessionList) {
            if (si.userName == userName) {

                m_LoginInter->call("ActivateSession", si.sessionId);
                return true;
            }

        }
        qDebug() << "cannot find session with this username!";
        return false;
    } else {
        qDebug() << "no session found!";
        return false;
    }

}
bool AccountsInfoModel::LogoutByUserName(const QString &userName)
{
    if (userName == m_currentUserName) {
        if (m_sessionList.size() > 0) {
            qInfo() << "m_sessionList is OK";
            for (SessionInfo si : m_sessionList) {
                if (si.userName == userName) {
                    qInfo() << "found" << userName << si.sessionId;
//                    qInfo() << m_LoginInter->TerminateSession(si.sessionId).error();
                    return true;
                }

            }
            qDebug() << "cannot find session with current username!";

        } else {
            qDebug() << "no session found!";

        }
    } else {
        QStringList params;

        // check pkexec existance
        if (!QFile::exists({PKEXEC_PATH})) {
            return false;
        }

        // check kill existance
        if (!QFile::exists({PKILL_PATH})) {
            return false;
        }

        params << QString(PKILL_PATH) << QString("-u") << userName;

        QProcess proc;

        proc.start({PKEXEC_PATH}, params);
        proc.waitForFinished();
        return true;
    }
    return false;

}

void AccountsInfoModel::EditAccount()
{
    if (m_controlInter)
        m_controlInter->call("ShowPage", "accounts", "Accounts Detail");

}



