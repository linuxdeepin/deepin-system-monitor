// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thread_manager.h"
#include "base_thread.h"
#include "ddlog.h"

#include <QThread>

namespace common {
namespace core {

using namespace DDLog;

Q_GLOBAL_STATIC(ThreadManager, theInstance)
ThreadManager *ThreadManager::instance()
{
    // qCDebug(app) << "ThreadManager instance";
    return theInstance();
}

void ThreadManager::attach(BaseThread *thread)
{
    // qCDebug(app) << "ThreadManager attach thread with key:" << thread->threadKey();
    if (m_threadDB.contains(thread->threadKey())) {
        // qCDebug(app) << "thread already exists";
        return;
    }

    m_threadDB[thread->threadKey()] = thread;
}

void ThreadManager::cleanup()
{
    // qCDebug(app) << "ThreadManager cleanup, stopping and deleting all threads";
    for (auto it = m_threadDB.begin(); it != m_threadDB.end(); ++it) {
        BaseThread *thread = it.value();
        if (!thread)
            continue;

        if (thread->isRunning()) {
            thread->quit();
            if (!thread->wait(200)) {
                qCWarning(app) << "Thread" << it.key()
                               << "failed to quit in time, terminating";
                thread->terminate();
                thread->wait(100);
            }
        }

        delete thread;
        it.value() = nullptr;
    }
    m_threadDB.clear();
}

} // namespace core
} // namespace common
