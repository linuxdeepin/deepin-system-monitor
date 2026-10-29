// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "detailwidgetmanager.h"
#include "ddlog.h"
#include <QTimer>

using namespace DDLog;
QMutex DetailWidgetManager::mutex;
QAtomicPointer<DetailWidgetManager> DetailWidgetManager::instance;

DetailWidgetManager &DetailWidgetManager::getInstance()
{
    qCDebug(app) << "DetailWidgetManager getInstance";
    if (instance.testAndSetOrdered(nullptr, nullptr))
    {
        qCDebug(app) << "new DetailWidgetManager";
        QMutexLocker locker(&mutex);

        instance.testAndSetOrdered(nullptr, new DetailWidgetManager);
    }
    return *instance;
}

void DetailWidgetManager::jumpDetailWidget(const QString &msgCode)
{
    qCDebug(app) << "jumpDetailWidget with msgCode:" << msgCode;
    internalMutex.lockForRead();
    QString detailWidgetName = msgCode;

//    QTimer::singleShot(50, this, [=]() {
//        emit sigJumpToProcessWidget("MSG_PROCESS");
//    });
    emit sigJumpToDetailWidget(detailWidgetName);
    internalMutex.unlock();
}

void DetailWidgetManager::jumpProcessWidget(const QString &msgCode) {
    qCDebug(app) << "jumpProcessWidget with msgCode:" << msgCode;
    internalMutex.lockForRead();
    QString detailWidgetName = msgCode;
    emit sigJumpToProcessWidget(detailWidgetName);
    internalMutex.unlock();
}


DetailWidgetManager::DetailWidgetManager(QObject *parent)
    :QObject (parent)
{
    qCDebug(app) << "DetailWidgetManager constructor";
//    connect(SystemMonitor::instance(), &SystemMonitor::statInfoUpdated, this, &DetailWidgetManager::sigDataUpdate);
}

DetailWidgetManager::~DetailWidgetManager()
{
    // qCDebug(app) << "DetailWidgetManager destructor";
}
