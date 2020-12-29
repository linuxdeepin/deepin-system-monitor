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
#ifndef MEM_STAT_MODEL_H
#define MEM_STAT_MODEL_H

#include "common/sample.h"
#include "system/mem.h"

#include <QAbstractTableModel>

using namespace common::core;
using namespace core::system;

template<>
class SampleFrame<mem_stat_t>
{
public:
    SampleFrame()
        : ts()
        , stat(std::make_shared<mem_stat_t>())
    {
    }
    SampleFrame(const struct timeval &tv, const MemStat &st)
        : ts(tv)
        , stat(st)
    {
    }
    SampleFrame(const SampleFrame &other)
        : ts(other.ts)
        , stat(std::make_shared<mem_stat_t>(*(other.stat)))
    {
    }

    struct timeval ts;
    MemStat stat;
};
using MemStatSampleFrame = SampleFrame<mem_stat_t>;
using MemStatSample = Sample<mem_stat_t>;

class MemInfoModel;

/**
 * @brief Memory stat model
 */
class MemStatModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum StatProp {
        kStatUsedMem,
        kStatTotalMem,
        kStatUsedSwap,
        kStatTotalSwap,

        kStatPropMax
    };

    enum DataRole {
        kValueRole = Qt::UserRole + 0x0001
    };

    /**
     * @brief Model constructor
     * @param parent Parent object
     */
    explicit MemStatModel(const TimePeriod &period, QObject *parent = nullptr);
    virtual ~MemStatModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    std::unique_ptr<Sample<mem_stat_t>> m_stat;

    friend class MemInfoModel;
};

#endif // MEM_STAT_MODEL_H
