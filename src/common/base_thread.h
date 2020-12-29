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
#ifndef BASE_THREAD_H
#define BASE_THREAD_H

#include <QThread>
#include <QThreadStorage>
#include <QMap>
#include <QVariant>

namespace common {
namespace core {

class BaseThread : public QThread
{
    Q_OBJECT

public:
    enum thread_key_t {
        kUnknowThread,
        kSystemMonitorThread,
        kNetifMonitorThread,

        kUserThread = 0xff
    };

    virtual int threadKey() const { return kUnknowThread; }

    explicit BaseThread(QObject *parent = nullptr)
        : QThread(parent)
    {
    }
    virtual ~BaseThread() {}

    template<typename T>
    T *threadJobInstance() const;

    bool hasLocalData(int key) const;
    void setLocalData(int key, QVariant value);
    QVariant localData(int key);

protected:
    virtual QObject *jobInstance() const = 0;

private:
    QThreadStorage<QMap<int, QVariant>> m_localData;
};

inline bool BaseThread::hasLocalData(int key) const
{
    return m_localData.hasLocalData() && m_localData.localData().contains(key);
}

inline void BaseThread::setLocalData(int key, QVariant value)
{
    if (!m_localData.hasLocalData()) {
        QMap<int, QVariant> map;
        m_localData.setLocalData(map);
    }

    m_localData.localData().insert(key, value);
}

inline QVariant BaseThread::localData(int key)
{
    if (!m_localData.hasLocalData())
        return {};

    return m_localData.localData()[key];
}

template<typename T>
inline T *BaseThread::threadJobInstance() const
{
    return static_cast<T *>(jobInstance());
}

} // namespace core
} // namespace common

#endif // BASE_THREAD_H
