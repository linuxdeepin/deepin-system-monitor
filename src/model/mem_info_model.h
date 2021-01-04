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
#ifndef MEM_INFO_MODEL_H
#define MEM_INFO_MODEL_H

// local
#include "common/time_period.h"
#include "system/mem.h"

// qt
#include <QObject>

// c++11
#include <memory>

using namespace core::system;
using namespace common::core;

class MemStatModel;

class MemInfoModel : public QObject
{
    Q_OBJECT

public:
    explicit MemInfoModel(const TimePeriod &period, QObject *parent = nullptr);
    virtual ~MemInfoModel();

    std::weak_ptr<MemStatModel> memStatModel() const;

    qulonglong memTotal() const;
    qulonglong memFree() const;
    qulonglong memAvailable() const;
    qulonglong buffers() const;
    qulonglong cached() const;
    qulonglong active() const;
    qulonglong inactive() const;

    qulonglong swapTotal() const;
    qulonglong swapFree() const;
    qulonglong swapCached() const;
    qulonglong shmem() const;
    qulonglong slab() const;
    qulonglong dirty() const;
    qulonglong mapped() const;

signals:
    void modelUpdated();

public slots:
    void updateModel();

private:
    std::shared_ptr<MemStatModel> m_statModel;

    MemInfo *m_info;
};

inline std::weak_ptr<MemStatModel> MemInfoModel::memStatModel() const
{
    return m_statModel;
}

inline qulonglong MemInfoModel::memTotal() const
{
    return m_info->memTotal();
}

inline qulonglong MemInfoModel::memFree() const
{
    return m_info->memFree();
}

inline qulonglong MemInfoModel::memAvailable() const
{
    return m_info->memAvailable();
}

inline qulonglong MemInfoModel::buffers() const
{
    return m_info->buffers();
}

inline qulonglong MemInfoModel::cached() const
{
    return m_info->cached();
}

inline qulonglong MemInfoModel::active() const
{
    return m_info->active();
}

inline qulonglong MemInfoModel::inactive() const
{
    return m_info->inactive();
}

inline qulonglong MemInfoModel::swapTotal() const
{
    return m_info->swapTotal();
}

inline qulonglong MemInfoModel::swapFree() const
{
    return m_info->swapFree();
}

inline qulonglong MemInfoModel::swapCached() const
{
    return m_info->swapCached();
}

inline qulonglong MemInfoModel::shmem() const
{
    return m_info->shmem();
}

inline qulonglong MemInfoModel::slab() const
{
    return m_info->slab();
}

inline qulonglong MemInfoModel::dirty() const
{
    return m_info->dirty();
}

inline qulonglong MemInfoModel::mapped() const
{
    return m_info->mapped();
}

#endif // MEM_INFO_MODEL_H
