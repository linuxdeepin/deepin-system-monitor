/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     tenghuan <tenghuan@uniontech.com>
*
* Maintainer: tenghuan <tenghuan@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "block_dev_stat_view_widget.h"
#include "block_dev_item_widget.h"
#include "common/thread_manager.h"
#include "system/system_monitor_thread.h"
#include "system/block_device_info_db.h"
#include "system/device_db.h"
#include <QThread>
#include <QGridLayout>
#include <QTimer>

using namespace common::core;
using namespace core::system;

BlockStatViewWidget::BlockStatViewWidget(QWidget *parent) : QWidget(parent)
{
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
//    auto info = monitor->deviceDB()->blockDeviceInfoDB()->deviceList();
//    for(int i =0; i < info.size(); ++i) {
//        BlockDevItemWidget *item  = new BlockDevItemWidget(info[i],this);
//        m_listBlockItemWidget << item;
//    }

   setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(monitor, &SystemMonitor::statInfoUpdated, this, &BlockStatViewWidget::onUpdateData);
   // m_timer->start(2000);
}

void BlockStatViewWidget::resizeEvent(QResizeEvent *event)
{

    QWidget::resizeEvent(event);
    updateWidgetGeometry();
}
void BlockStatViewWidget::updateWidgetGeometry()
{
    int deviceCount  = m_listDevice.size();
    if(deviceCount <=0 ){
        return ;
    }
    // 界面和数据相等。
    int m_list_len = m_listBlockItemWidget.size();
    if( m_list_len < deviceCount)
    {
        for( int i = 0 ; i < deviceCount - m_list_len ;i++){
               BlockDevItemWidget *item = new BlockDevItemWidget(this);
               m_listBlockItemWidget << item;
        }
    }

    if(deviceCount == 1){
        //showItem1();
    }else if(deviceCount == 2){
        //showItem2();
    }else if(deviceCount > 2){
       // showItemLg2(netCount);
    }
}

void BlockStatViewWidget::onUpdateData()
{
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    auto info = monitor->deviceDB()->blockDeviceInfoDB()->deviceList();
    for(int i =0; i < info.size(); ++i) {
        m_listBlockItemWidget[i]->updateData(info[i]);
        m_listBlockItemWidget[i]->update();
    }

}
