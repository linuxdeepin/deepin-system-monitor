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
#include "system/system_monitor.h"
#include "system/block_device_info_db.h"
#include "system/device_db.h"
#include <QThread>
#include <QGridLayout>
#include <QTimer>

using namespace core::system;

BlockStatViewWidget::BlockStatViewWidget(QWidget *parent) : QScrollArea(parent)
{
    auto *monitor = SystemMonitor::instance();
    m_listDevice = monitor->deviceDB()->blockDeviceInfoDB()->deviceList();

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_centralWidget = new QWidget(this);
    this->setWidget(m_centralWidget);
 //   updateWidgetGeometry();

    m_centralWidget->setMinimumHeight(300);
    m_centralWidget->setWindowFlags(Qt::FramelessWindowHint);
    this->setFrameShape(QFrame::NoFrame);
    onUpdateData();
    connect(monitor, &SystemMonitor::statInfoUpdated, this, &BlockStatViewWidget::onUpdateData);
}

void BlockStatViewWidget::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    updateItemWidget();
}

void BlockStatViewWidget::updateItemWidget()
{
    int deviceCount  = m_listDevice.size();
    if(deviceCount <=0 ){
        return ;
    }

    int Width = this->width();
    int height = this->height();
    int fontHeight = QFontMetrics(m_font).height();

    int avgWidth = this->width() / 2 - 10;
    int avgheight = height - fontHeight/2;
    // 界面和数据相等。
    int m_list_len = m_listBlockItemWidget.size();
    if( m_list_len < deviceCount)
    {
        for( int i = 0 ; i < deviceCount - m_list_len ;i++){
               BlockDevItemWidget *item = new BlockDevItemWidget(m_listDevice[i+m_list_len],m_centralWidget);
               m_listBlockItemWidget << item;
               connect(item,&BlockDevItemWidget::clicked,this, &BlockStatViewWidget::onSetItemStatus);
              // m_mapDeviceItemWidget.insert();
        }
    }
    if(deviceCount == 1) {
        m_listBlockItemWidget[0]->setMode(0);
        m_listBlockItemWidget[0]->setGeometry(0,0,avgWidth,avgheight);
        m_listBlockItemWidget[0]->update();
    } else if(deviceCount ==2) {
        m_listBlockItemWidget[0]->setMode(1);
        m_listBlockItemWidget[1]->setMode(1);
        m_listBlockItemWidget[0]->setGeometry(0, 0, avgWidth, avgheight-20);
        m_listBlockItemWidget[1]->setGeometry(m_listBlockItemWidget[0]->geometry().right() + 10, 0,avgWidth, avgheight-20);
        m_listBlockItemWidget[0]->update();
        m_listBlockItemWidget[1]->update();

    } else {
         for(int i = 0 ; i < m_listBlockItemWidget.size();i++)
         {
             int page = i /4;
             BlockDevItemWidget *item1 = m_listBlockItemWidget.at(i);
             item1->setMode(2);
             m_mapDeviceItemWidget.insert(m_listDevice[i].deviceName(),item1);
             if(i%4 == 0){
                  item1->setGeometry(page*Width,0,avgWidth,avgheight/2);
             }else if(i%4 == 1){
                  item1->setGeometry(page*Width+avgWidth+10,0,avgWidth,avgheight/2);
             }else if(i%4 == 2){
                  item1->setGeometry(page*Width,avgheight/2,avgWidth,avgheight/2);
             }else if(i%4 == 3){
                  item1->setGeometry(page*Width+avgWidth+10,avgheight/2,avgWidth,avgheight/2);
             }
             item1->update();
         }
    }
}

void BlockStatViewWidget::fontChanged(const QFont &font) {
    for(int i =0; i < m_listBlockItemWidget.size(); ++i) {
        m_listBlockItemWidget[i]->fontChanged(font);
    }
}

void BlockStatViewWidget::updateWidgetGeometry()
{
    int deviceCount  = m_listDevice.size();
    if(deviceCount <=0){
        return ;
    }
    int m_list_len = m_listBlockItemWidget.size();
    if( m_list_len < deviceCount)
    {
        // 创建新的Item
        for( int i = 0 ; i < deviceCount - m_list_len ;i++){
               BlockDevItemWidget *item = new BlockDevItemWidget(m_listDevice[i+m_list_len],m_centralWidget);
               m_listBlockItemWidget << item;
               connect(item,&BlockDevItemWidget::clicked,this, &BlockStatViewWidget::onSetItemStatus);
              // m_mapDeviceItemWidget.insert();
        }
    }
    if(deviceCount == 1){
        showItem1();
    }else if(deviceCount == 2){
        showItem2();
    }else if(deviceCount > 2){
        showItemLg2(deviceCount);
    }
}

void BlockStatViewWidget::onSetItemStatus(const QString& deviceName)
{
    auto it = m_mapDeviceItemWidget.begin();
    for(; it != m_mapDeviceItemWidget.end(); ++it) {
        if(it.key() == deviceName) {

        } else {
            it.value()->showBackGround(false);
        }
    }
    emit changeInfo(deviceName);
}

void BlockStatViewWidget::showItem1()
{
    int avgWidth = this->width();
    int avgheight = this->height();
    BlockDevItemWidget *item = m_listBlockItemWidget.at(0);
    item->setMode(0);
    item->show();
    m_mapDeviceItemWidget.insert(m_listDevice[0].deviceName(),item);
    // int fontHeight = QFontMetrics(m_font).height();
    item->setGeometry(0,0,avgWidth,avgheight);
    item->showBackGround(false);

    m_centralWidget->resize(avgWidth, avgheight);
    item->updateData(m_listDevice[0]);
    item->update();
    for(int i = 1 ; i < m_listBlockItemWidget.size();i++){
        m_listBlockItemWidget.at(i)->hide();
        m_listBlockItemWidget.at(i)->setMode(0);
     }
     emit changeInfo(m_listDevice[0].deviceName());
}
void BlockStatViewWidget::showItem2()
{
    int avgWidth = this->width();
    int avgheight = this->height();
    avgWidth = this->width() / 2 - 10;
    BlockDevItemWidget *item1 = m_listBlockItemWidget.at(0);
    BlockDevItemWidget *item2 = m_listBlockItemWidget.at(1);
    item1->show();
    item2->show();
    item1->setMode(1);
    item2->setMode(1);
    m_mapDeviceItemWidget.insert(m_listDevice[0].deviceName(),item1);
    m_mapDeviceItemWidget.insert(m_listDevice[1].deviceName(),item2);
    //   unsigned int fontHeight = QFontMetrics(m_font).height();

    item1->setGeometry(0, 0, avgWidth, avgheight-20);
    item2->setGeometry(item1->geometry().right() + 10, 0,avgWidth, avgheight-20);
    if(!item1->backGround() && !item2->backGround()) {
        item1->showBackGround(true);
    }

    m_centralWidget->resize(item2->geometry().right(), avgheight - 20);
    item1->updateData(m_listDevice[0]);
    item2->updateData(m_listDevice[1]);

    item1->update();
    item2->update();
    bool  haveSelect = false;
    for(int i = 2 ; i < m_listBlockItemWidget.size();i++){
    m_listBlockItemWidget.at(i)->hide();
    if( m_listBlockItemWidget.at(i)->backGround()) {
            haveSelect = true;
      }
     m_listBlockItemWidget.at(i)->setMode(0);
    }
    if(haveSelect) {
         m_listBlockItemWidget.at(0)->showBackGround(true);
         emit changeInfo(m_listDevice[0].deviceName());
    }
}
void BlockStatViewWidget::showItemLg2(int count)
{
    int Width = this->width();
    int height = this->height();
    int fontHeight = QFontMetrics(m_font).height();

    int avgWidth = this->width() / 2 - 10;
    int avgheight = height - fontHeight/2;
    bool noSelect = true;
    for(int i =0 ;i < count;i++){
        int page = i /4;
        BlockDevItemWidget *item1 = m_listBlockItemWidget.at(i);
        item1->setMode(2);
        item1->show();
         m_mapDeviceItemWidget.insert(m_listDevice[i].deviceName(),item1);
        if(i%4 == 0){
             item1->setGeometry(page*Width,0,avgWidth,avgheight/2);
        }else if(i%4 == 1){
              item1->setGeometry(page*Width+avgWidth+10,0,avgWidth,avgheight/2);
        }else if(i%4 == 2){
              item1->setGeometry(page*Width,avgheight/2,avgWidth,avgheight/2);
        }else if(i%4 == 3){
              item1->setGeometry(page*Width+avgWidth+10,avgheight/2,avgWidth,avgheight/2);
        }
        if(item1->backGround()) {
            noSelect = false;
        }
        item1->updateData(m_listDevice[i]);
        item1->update();
    }
    if(noSelect) {
        m_listBlockItemWidget.at(0)->showBackGround(true);
        emit changeInfo(m_listDevice[0].deviceName());
    }
    m_centralWidget->resize(Width*(((count-1)/4)+1), height);

    bool  haveSelect = false;
    for(int i = count ; i < m_listBlockItemWidget.size();i++){
       m_listBlockItemWidget.at(i)->hide();
       if( m_listBlockItemWidget.at(i)->backGround()) {
               haveSelect = true;
           }
       m_listBlockItemWidget.at(i)->setMode(0);
    }
    if(haveSelect) {
         m_listBlockItemWidget.at(0)->showBackGround(true);
         emit changeInfo(m_listDevice[0].deviceName());
    }
}


void BlockStatViewWidget::onUpdateData()
{
    auto *monitor =  SystemMonitor::instance();;
    m_listDevice = monitor->deviceDB()->blockDeviceInfoDB()->deviceList();
    m_mapDeviceItemWidget.clear();
    updateWidgetGeometry();
}
