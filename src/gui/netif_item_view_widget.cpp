#include "netif_item_view_widget.h"
#include "chart_view_widget.h"
#include "common/common.h"
#include <QPainter>

#include <DApplication>
#include <DApplicationHelper>

using namespace common::format;
NetifItemViewWidget::NetifItemViewWidget(QWidget *parent) : QWidget(parent)
  ,m_mode(0)
{
   setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   m_ChartWidget = new ChartViewWidget(this);
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

    int spacing = 10;
    int sectionSize = 6;
     if(m_mode == 0){
        QString memory = DApplication::translate("Process.Graph.Title", "上传");
        QRect memtitleRect(sectionSize + spacing, 0, painter.fontMetrics().width(memory), painter.fontMetrics().height());
        painter.drawText(memtitleRect, Qt::AlignLeft | Qt::AlignVCenter, memory);

        QString swap = DApplication::translate("Process.Graph.View", "下载");
        QRect swaptitleRect(sectionSize + memtitleRect.right() + 2 * spacing, memtitleRect.y(), painter.fontMetrics().width(swap), painter.fontMetrics().height());
        painter.drawText(swaptitleRect, Qt::AlignLeft | Qt::AlignVCenter, swap);


        painter.setPen(Qt::NoPen);
        painter.setBrush(upColor);
        painter.drawEllipse(0, memtitleRect.center().y(), sectionSize, sectionSize);

        painter.setBrush(downColor);
        painter.drawEllipse(memtitleRect.right() + spacing, swaptitleRect.center().y(), sectionSize, sectionSize);
    }else{
         QString memory = DApplication::translate("Process.Graph.Title", "上传");
         QRect memtitleRect(sectionSize + spacing, 0, painter.fontMetrics().width(memory), painter.fontMetrics().height());
         painter.drawText(memtitleRect, Qt::AlignLeft | Qt::AlignVCenter, memory);

         QString swap = DApplication::translate("Process.Graph.View", "下载");
         QRect swaptitleRect(sectionSize + memtitleRect.right() + 2 * spacing, memtitleRect.y(), painter.fontMetrics().width(swap), painter.fontMetrics().height());
         painter.drawText(swaptitleRect, Qt::AlignLeft | Qt::AlignVCenter, swap);


         painter.setPen(Qt::NoPen);
         painter.setBrush(upColor);
         painter.drawEllipse(0, memtitleRect.center().y(), sectionSize, sectionSize);

         painter.setBrush(downColor);
         painter.drawEllipse(memtitleRect.right() + spacing, swaptitleRect.center().y(), sectionSize, sectionSize);
     }

}

void NetifItemViewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    int fontHeight = QFontMetrics(m_font).height();
    int avgWidth =  this->width();
    int avgHeight = this->height();
    m_ChartWidget->setGeometry(0,fontHeight / 2,avgWidth,avgHeight - fontHeight / 2);
}

void NetifItemViewWidget::fontChanged(const QFont &font)
{
    QString memoryDetail = "f";
    parent()->setProperty("detail", memoryDetail);
}

void NetifItemViewWidget::onModelUpdate()
{

}

void NetifItemViewWidget::setMode(int mode)
{
        m_mode = mode;
}

void NetifItemViewWidget::updateWidgetGeometry()
{

}
