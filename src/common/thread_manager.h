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

#include <QThread>
#include <QMap>

namespace common {
namespace core {

/**
 * @brief Thread Manager class to keep tracking of long running threads
 */
class BaseThread;
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

public:
    explicit ThreadManager() {}
    virtual ~ThreadManager() {}

private:
    QMap<int, BaseThread *> m_threadDB;
};

template<typename T>
inline T *ThreadManager::thread(int key)
{
    auto *thr = m_threadDB[key];
    return static_cast<T *>(thr);
}

} // namespace core
} // namespace common

#endif // THREAD_MANAGER_H
