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
#include "netif_summary_view_widget.h"
#include "system/device_db.h"
#include "common/common.h"
#include "system/netif.h"

#include <QHeaderView>
#include <QAbstractTableModel>

using namespace core::system;
using namespace common::format;


class NetInfoModel :public QAbstractTableModel
{
public:
    explicit NetInfoModel(QObject *parent = nullptr);
    virtual ~NetInfoModel();
protected:
    int rowCount(const QModelIndex &) const
    {
        return 7;
    }

    int columnCount(const QModelIndex &) const
    {
        return 2;
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant();


        return QVariant();
    }
};
NetInfoModel::NetInfoModel(QObject *parent): QAbstractTableModel(parent)
{
    //m_memInfo = DeviceDB::instance()->memInfo();
}

NetInfoModel::~NetInfoModel()
{

}




NetifSummaryViewWidget::NetifSummaryViewWidget(QWidget *parent)
    : DTableView(parent),
      m_index(0)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  this->horizontalHeader()->setVisible(false);
  this->verticalHeader()->setVisible(false);
  this->horizontalHeader()->setSectionResizeMode(DHeaderView::Stretch);
  this->verticalHeader()->setSectionResizeMode(DHeaderView::Stretch);
  this->setGridStyle(Qt::NoPen);
  this->setFrameShape(QFrame::NoFrame);
   m_netInfoModel = new NetInfoModel(this);
   this->setModel(m_netInfoModel);
}

void NetifSummaryViewWidget::updateWidgetGeometry(DeviceDB *info)
{
    // update model

}

void NetifSummaryViewWidget::fontChanged(const QFont &font)
{

}

void NetifSummaryViewWidget::paintEvent(QPaintEvent *event)
{

}


