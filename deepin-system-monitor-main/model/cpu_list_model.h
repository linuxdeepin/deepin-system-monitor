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
#ifndef CPU_LIST_MODEL_H
#define CPU_LIST_MODEL_H

#include "cpu_stat_model.h"
#include "common/time_period.h"

#include <QAbstractTableModel>
#include <QByteArray>

#include <memory>

using namespace common::core;

class CPUInfoModel;

class CPUListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Property {
        kPropCPUIndex,
        kPropCPUUsage,

        kPropCount
    };

    enum DataRole {
        kValueRole = Qt::UserRole + 0x0001
    };

    explicit CPUListModel(QObject *parent);
    virtual ~CPUListModel() override = default;

    std::weak_ptr<CPUStatModel> statModel(const QByteArray &cpu) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void fetchMore(const QModelIndex &parent) override;
    bool canFetchMore(const QModelIndex &parent) const override;

private:
    QList<CPUInfo> m_cpuInfoDB;
    QMap<QByteArray, std::shared_ptr<CPUStatModel>> m_statModelDB;

    int m_nr;

    friend class CPUInfoModel;
};

inline std::weak_ptr<CPUStatModel> CPUListModel::statModel(const QByteArray &cpu) const
{
    return m_statModelDB[cpu];
}

#endif // CPU_LIST_MODEL_H
