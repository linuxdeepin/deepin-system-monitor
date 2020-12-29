/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "thread_manager.h"

#include <QReadLocker>
#include <QWriteLocker>

namespace common {
namespace core {

std::atomic<ThreadManager *> ThreadManager::m_instance;
std::mutex ThreadManager::m_mutex;

void ThreadManager::attach(BaseThread *thread)
{
    QWriteLocker lock(&m_rwlock);

    if (m_threadDB.contains(thread->threadKey()))
        return;

    m_threadDB[thread->threadKey()] = thread;
    connect(thread, &QThread::finished, this, [=]() {
        detach(thread->threadKey());
    });
}

void ThreadManager::detach(int key)
{
    QWriteLocker lock(&m_rwlock);
    m_threadDB.remove(key);
}

ThreadManager::ThreadManager(QObject *parent)
    : QObject(parent)
{
}
ThreadManager::~ThreadManager()
{
}

} // namespace core
} // namespace common
