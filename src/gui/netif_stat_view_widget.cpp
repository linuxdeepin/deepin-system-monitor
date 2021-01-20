
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
}

void NetifStatViewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}
void NetifStatViewWidget::resizeEvent(QResizeEvent *event)
{

    QWidget::resizeEvent(event);
    updateWidgetGeometry();
}

void NetifStatViewWidget::fontChanged(const QFont &font)
{
    m_font = font;
    //setFixedHeight(130 + 2 * QFontMetrics(font).height());
}

void NetifStatViewWidget::onModelUpdate()
{
    QString memoryDetail = "f";
    parent()->setProperty("detail", memoryDetail);
}

static int t_i = 100;

void NetifStatViewWidget::updateWidgetGeometry()
{

    int netCount  = t_i%18;//m_info->infoDB().size();
    t_i--;
    qInfo()<<"t_i="<<t_i;
    if(netCount <=0 ){
        return ;
    }
    // 界面和数据相等。
    unsigned long m_list_len = m_lists.size();
    if( m_list_len < netCount)
    {
        for( unsigned long i = 0 ; i < netCount - m_list_len ;i++){
               NetifItemViewWidget *item = new NetifItemViewWidget(m_centralWidget);
                m_lists.push_back(item);
        }
    }

    if(netCount == 1){
        showItem1();
    }else if(netCount == 2){
        showItem2();
    }else if(netCount > 2){
        showItemLg2(netCount);
    }
}

void NetifStatViewWidget::showItem1()
{
    int avgWidth = this->width();
    int avgheight = this->height();
     NetifItemViewWidget *item = m_lists.at(0);
     item->show();
     int fontHeight = QFontMetrics(m_font).height();
     item->setGeometry(0,0,avgWidth,avgheight);
     m_centralWidget->resize(avgWidth, avgheight);

     for(int i = 1 ; i < m_lists.size();i++){
        m_lists.at(i)->hide();
     }
}

void NetifStatViewWidget::showItem2()
{
    int avgWidth = this->width();
    int avgheight = this->height();
     avgWidth = this->width() / 2 - 10;
     NetifItemViewWidget *item1 = m_lists.at(0);
     NetifItemViewWidget *item2 = m_lists.at(1);
     item1->show();
     item2->show();
     unsigned int fontHeight = QFontMetrics(m_font).height();

     item1->setGeometry(0, 0, avgWidth, avgheight-20);
     item2->setGeometry(item1->geometry().right() + 20, 0,avgWidth + 20, avgheight-20);
     m_centralWidget->resize(item2->geometry().right(), avgheight - 20);

     for(int i = 2 ; i < m_lists.size();i++){
        m_lists.at(i)->hide();
     }
}

void NetifStatViewWidget::showItemLg2(const unsigned long count)
{
    int Width = this->width();
    int height = this->height();
    unsigned int fontHeight = QFontMetrics(m_font).height();

    int avgWidth = this->width() / 2 - 10;
    int avgheight = height - fontHeight/2;
    for(int i =0 ;i < count;i++){
        int page = i /4;
        NetifItemViewWidget *item1 = m_lists.at(i);
        item1->show();
        if(i%4 == 0){
             item1->setGeometry(page*Width,0,avgWidth,avgheight/2);
        }else if(i%4 == 1){
              item1->setGeometry(page*Width+avgWidth+10,0,avgWidth,avgheight/2);
        }else if(i%4 == 2){
              item1->setGeometry(page*Width,avgheight/2,avgWidth,avgheight/2);
        }else if(i%4 == 3){
              item1->setGeometry(page*Width+avgWidth+10,avgheight/2,avgWidth,avgheight/2);
        }
    }
    m_centralWidget->resize(Width*(((count-1)/4)+1), height);
    for(unsigned long i = count ; i < m_lists.size();i++){
       m_lists.at(i)->hide();
    }

}

