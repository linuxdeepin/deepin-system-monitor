
#include "netif_stat_view_widget.h"
#include "netif_item_view_widget.h"
#include "chart_view_widget.h"
#include "common/common.h"
#include "common/thread_manager.h"
#include "system/system_monitor_thread.h"

#include "system/netif_info_db.h"
#include <QPainter>

#include <DApplication>
#include <DApplicationHelper>
#include <QGridLayout>
#include <QTimer>

using namespace common::format;
using namespace core::system;

NetifStatViewWidget::NetifStatViewWidget(QWidget *parent) : QScrollArea(parent)
{

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_info = DeviceDB::instance()->netifInfoDB();
    m_centralWidget = new QWidget(this);
    this->setWidget(m_centralWidget);
    m_centralWidget->setWindowFlags(Qt::FramelessWindowHint);
    this->setFrameShape(QFrame::NoFrame);
    onModelUpdate();
     auto *monitor = SystemMonitor::instance();
     connect(monitor, &SystemMonitor::statInfoUpdated, this, &NetifStatViewWidget::onModelUpdate);
}

void NetifStatViewWidget::paintEvent(QPaintEvent *event)
{
    QScrollArea::paintEvent(event);
}
void NetifStatViewWidget::resizeEvent(QResizeEvent *event)
{

    QScrollArea::resizeEvent(event);
    updateWidgetGeometry();
}

void NetifStatViewWidget::fontChanged(const QFont &font)
{
    m_font = font;
    //setFixedHeight(130 + 2 * QFontMetrics(font).height());
}

void NetifStatViewWidget::onModelUpdate()
{
    QMap<QByteArray, NetifInfo>  NetifInfoDB = m_info->infoDB();
    auto iter =  NetifInfoDB.begin();
    while(iter != NetifInfoDB.end()){
       QByteArray mac = iter.key();
      if(!m_mapItemView.contains(mac)){
               NetifItemViewWidget *temp = new NetifItemViewWidget(this,mac);
               temp->updateData(iter.value());
               temp->update();
               m_mapItemView.insert(iter.key(),temp);
       }
      else{
            NetifItemViewWidget *temp = m_mapItemView.value(mac);
            temp->updateData(iter.value());
            temp->update();
      }
       iter++;
    }
    updateWidgetGeometry();
}

//static int t_i = 100;

void NetifStatViewWidget::updateWidgetGeometry()
{

   int netCount  = m_info->infoDB().size();

    if(netCount == 1){
        showItem1();
    }else if(netCount == 2){
        showItem2();
    }else if(netCount > 2){
        showItemLg2();
    }
}

void NetifStatViewWidget::showItem1()
{
    int avgWidth = this->width();
    int avgheight = this->height();

    auto iter = m_mapItemView.begin();
    while(iter !=  m_mapItemView.end()){
        auto itemView = iter.value();
        if(m_info->infoDB().contains(iter.key()) ){
            itemView->show();
            itemView->setMode(1);
            itemView->setGeometry(0,0,avgWidth,avgheight);
        }
        else {
            itemView->hide();
        }
        iter ++;
    }
    m_centralWidget->resize(avgWidth, avgheight);
}

void NetifStatViewWidget::showItem2()
{
    int avgWidth = this->width();
    int avgheight = this->height();
    int itemWidth = avgWidth / 2 - 10;
    auto iter = m_mapItemView.begin();
    int i = 0;
    while(iter !=  m_mapItemView.end()){
        auto itemView = iter.value();
        if(m_info->infoDB().contains(iter.key())){
            if(i == 0 ){
                itemView->show();
                itemView->setMode(1);
                itemView->setGeometry(0,0,itemWidth,avgheight);
            }
            else{
                itemView->show();
                itemView->setMode(1);
                itemView->setGeometry(itemWidth+16,0,itemWidth,avgheight);
            }
            i++;
        }
        else {
            itemView->hide();
        }
        iter ++;
    }
}

void NetifStatViewWidget::showItemLg2()
{
    int Width = this->width();
    int height = this->height();
    int fontHeight = QFontMetrics(m_font).height();
    int avgWidth = this->width() / 2 - 16;
    int avgheight = height - fontHeight/2;
    auto iter = m_mapItemView.begin();
    int i = 0;
    while(iter !=  m_mapItemView.end()){
        int page = i /4;
        auto itemView = iter.value();
        if(m_info->infoDB().contains(iter.key())){
           auto itemView = iter.value();
           itemView->setMode(1);
            if(i%4 == 0){
                 itemView->setGeometry(page*Width,0,avgWidth,avgheight/2);
            }else if(i%4 == 1){
                  itemView->setGeometry(page*Width+avgWidth+16,0,avgWidth,avgheight/2);
            }else if(i%4 == 2){
                  itemView->setGeometry(page*Width,avgheight/2,avgWidth,avgheight/2);
            }else if(i%4 == 3){
                  itemView->setGeometry(page*Width+avgWidth+16,avgheight/2,avgWidth,avgheight/2);
            }
            itemView->update();
            itemView->show();
            i++;
        }
        else {
            itemView->hide();
        }
        iter ++;
    }
}

