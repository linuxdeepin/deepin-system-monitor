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
#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "application.h"
#include "base_thread.h"

#include <QThread>
#include <QThreadStorage>
#include <QMutex>
#include <QReadWriteLock>
#include <QApplication>
#include <QMap>

#include <mutex>

namespace common {
namespace core {

/**
 * @brief Thread Manager class to keep tracking of long running threads
 */
class ThreadManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ThreadManager)

public:
    static ThreadManager *instance();

    // TODO: no safe to return raw pointer here, since threads can quit by itself while being used in other threads
    // need a better way, eg: add a read/write lock & reference counter in base thread implemention...
    template<typename T>
    T *thread(int key);

public slots:
    void attach(BaseThread *thread);
    void detach(int key);

private:
    explicit ThreadManager(QObject *parent = nullptr);
    virtual ~ThreadManager();

private:
    mutable QReadWriteLock m_rwlock;
    QMap<int, BaseThread *> m_threadDB;

    static std::atomic<ThreadManager *> m_instance;
    static std::mutex m_mutex;
};

inline ThreadManager *ThreadManager::instance()
{
    ThreadManager *sin = m_instance.load();
    if (!sin) {
        std::lock_guard<std::mutex> lock(m_mutex);
        sin = m_instance.load();
        if (!sin) {
            sin = new ThreadManager();
            connect(gApp, &QApplication::aboutToQuit, sin, &QObject::deleteLater);
            m_instance.store(sin);
        }
    }
    return sin;
}

template<typename T>
inline T *ThreadManager::thread(int key)
{
    QReadLocker lock(&m_rwlock);
    auto *thr = m_threadDB[key];
    return static_cast<T *>(thr);
}

} // namespace core
} // namespace common

#endif // THREAD_MANAGER_H
