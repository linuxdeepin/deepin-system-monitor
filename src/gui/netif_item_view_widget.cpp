#include "netif_item_view_widget.h"
#include "chart_view_widget.h"
#include "common/common.h"
#include <QPainter>

#include <DApplication>
#include <DApplicationHelper>

using namespace common::format;
NetifItemViewWidget::NetifItemViewWidget(QWidget *parent,QByteArray mac) : QWidget(parent)
  ,m_mode(0)
  ,m_mac(mac)
  ,m_ifname("以太网")
{
   setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   m_ChartWidget = new ChartViewWidget(this);
   m_ChartWidget->setData1Color(m_recvColor);
   m_ChartWidget->setData2Color(m_sentColor);
   m_ChartWidget->setSpeedAxis(true);
   updateWidgetGeometry();

}

void NetifItemViewWidget::paintEvent(QPaintEvent *event)
{

    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setFont(m_font);

    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    painter.setPen(palette.color(DPalette::TextTips));
    QRect ifnameRect(0,0,painter.fontMetrics().width(m_ifname), painter.fontMetrics().height());
    painter.drawText(ifnameRect, Qt::AlignLeft | Qt::AlignVCenter, m_ifname);

   int spacing = 10;
   int sectionSize = 6;

     if(m_mode == 0){
        QString strLeftBrackets = "(";
         QRect leftBracketsRect(ifnameRect.right() + spacing, 0, painter.fontMetrics().width(strLeftBrackets), painter.fontMetrics().height());
          painter.drawText(leftBracketsRect, Qt::AlignLeft | Qt::AlignVCenter, strLeftBrackets);

        QString strRecvData = DApplication::translate("Process.Graph.Title", "接收");
        strRecvData = strRecvData +" "+ m_recv_bps;
        QRect recvRect(leftBracketsRect.right() + spacing*2 + sectionSize , 0, painter.fontMetrics().width(strRecvData), painter.fontMetrics().height());
        painter.drawText(recvRect, Qt::AlignLeft | Qt::AlignVCenter, strRecvData);

        QString strSentData = DApplication::translate("Process.Graph.View", "发送");
        strSentData = strSentData + " " + m_sent_bps + "  )";
        QRect sentRect(sectionSize + recvRect.right() + 2 * spacing, recvRect.y(), painter.fontMetrics().width(strSentData), painter.fontMetrics().height());
        painter.drawText(sentRect, Qt::AlignLeft | Qt::AlignVCenter, strSentData);


        painter.setPen(Qt::NoPen);
        painter.setBrush(m_recvColor);
        painter.drawEllipse(leftBracketsRect.right()+spacing, leftBracketsRect.center().y(), sectionSize, sectionSize);

        painter.setBrush(m_sentColor);
        painter.drawEllipse(recvRect.right() + spacing, recvRect.center().y(), sectionSize, sectionSize);
    }else{

        QString strRecvData = DApplication::translate("Process.Graph.Title", "接收");
        strRecvData = strRecvData + " " + m_recv_bps;
        QRect recvRect(spacing + sectionSize, ifnameRect.height(), painter.fontMetrics().width(strRecvData), painter.fontMetrics().height());
        painter.drawText(recvRect, Qt::AlignLeft | Qt::AlignVCenter, strRecvData);

        QString strSentData = DApplication::translate("Process.Graph.View", "发送");
        strSentData = strSentData  + " " + m_sent_bps;
        QRect sentRect(spacing + sectionSize, recvRect.y() + recvRect.height(), painter.fontMetrics().width(strSentData), painter.fontMetrics().height());
        painter.drawText(sentRect, Qt::AlignLeft | Qt::AlignVCenter, strSentData);


        painter.setPen(Qt::NoPen);
        painter.setBrush(m_recvColor);
        painter.drawEllipse(0, recvRect.center().y(), sectionSize, sectionSize);

        painter.setBrush(m_sentColor);
        painter.drawEllipse(0 , sentRect.center().y(), sectionSize, sectionSize);
     }

     if(m_isActive){
         painter.save();
         painter.setPen(Qt::NoPen);
         painter.setBrush(QColor::fromRgb(0,129,255,26));
         painter.drawRect(rect());
         painter.restore();
     }else{
         painter.save();
         painter.setPen(Qt::NoPen);
         painter.setBrush(Qt::transparent);
         painter.drawRect(rect());
         painter.restore();
     }
}

void NetifItemViewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateWidgetGeometry();
}

void NetifItemViewWidget::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    m_isActive = true;
    update();
    qInfo()<<"m_mac"<<m_mac;
    emit changeAllActive(m_mac);
}

void NetifItemViewWidget::fontChanged(const QFont &font)
{
    m_font = font;
    this->setFont(m_font);
    setFixedHeight(QFontMetrics(font).height() * 11);
}

void NetifItemViewWidget::setMode(int mode)
{
    m_mode = mode;
}

void NetifItemViewWidget::updateData(NetifInfo netifInfo)
{
    m_ChartWidget->addData1(netifInfo.recv_bps());
    m_ChartWidget->addData2(netifInfo.sent_bps());

    if(!netifInfo.ifname().isNull()){m_ifname = netifInfo.ifname();}

    m_recv_bps = formatUnit(netifInfo.recv_bps(), B, 1, true);
    m_sent_bps = formatUnit(netifInfo.sent_bps(), B, 1, true);

//    qInfo()<<"m_recv_bps:"<<m_recv_bps;
//    qInfo()<<"m_sent_bps:"<<m_sent_bps;
}

QByteArray NetifItemViewWidget::getMac()
{
    return m_mac;
}

void NetifItemViewWidget::setMac(QByteArray mac)
{
    m_mac = mac;
}

void NetifItemViewWidget::updateActiveStatus(bool active)
{
    m_isActive = active;
    updateWidgetGeometry();
}

void NetifItemViewWidget::updateWidgetGeometry()
{

    int fontHeight = QFontMetrics(m_font).height();
    int avgWidth =  this->width();
    int avgHeight = this->height();
    if(m_mode == 0){
        m_ChartWidget->setGeometry(0,fontHeight / 2,avgWidth,avgHeight - fontHeight / 2);
    }
    else{
        m_ChartWidget->setGeometry(0,int(fontHeight * 2.5) ,avgWidth,avgHeight - int(fontHeight* 2.5));
    }



}
