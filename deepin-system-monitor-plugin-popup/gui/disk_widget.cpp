/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      Wang Yong <wangyong@deepin.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "disk_widget.h"
#include "../common/utils.h"
#include "datadealsingleton.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyleHelper>

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QBrush>
#include <QPaintEvent>
#include <QFontMetrics>

DWIDGET_USE_NAMESPACE

using namespace Utils;
const int pointsNumber = 30;

DiskWidget::DiskWidget(QWidget *parent)
    : QWidget(parent)
{
    readSpeeds = new QList<qreal>();
    for (int i = 0; i <= pointsNumber; i++) {
        readSpeeds->append(0);
    }

    writeSpeeds = new QList<qreal>();
    for (int i = 0; i <= pointsNumber; i++) {
        writeSpeeds->append(0);
    }


     auto *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &DiskWidget::changeTheme);
    changeTheme(dAppHelper->themeType());

    setFixedSize(280, 204);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &DiskWidget::changeFont);

    connect(&DataDealSingleton::getInstance(), &DataDealSingleton::sigDataUpdate, this, &DiskWidget::updateStatus);
    this->installEventFilter(this);
}

DiskWidget::~DiskWidget() {
    delete readSpeeds;
    delete writeSpeeds;
}

void DiskWidget::updateStatus()
{
    QString diskRead, diskTotalSize, diskWrite, diskAvailable;
    if (!DataDealSingleton::getInstance().readDiskInfo(diskRead, diskTotalSize, diskWrite, diskAvailable))
        return;

    QStringList strs = diskRead.split(" ");
    if (strs.size() == 2)
    {
        m_diskRead = strs.at(0);
        m_diskReadUnit = strs.at(1);
    }
    else
        return;

    // Init download path.
    readSpeeds->append(m_diskRead.toDouble());

    if (readSpeeds->size() > pointsNumber + 1) {
        readSpeeds->pop_front();
    }
    double downloadMaxHeight = *std::max_element(readSpeeds->begin(), readSpeeds->end()) * 1.1;

    strs = diskWrite.split(" ");
    if (strs.size() == 2)
    {
        m_diskWrite = strs.at(0);
        m_diskWriteUnit = strs.at(1);
    }
    else
        return;

    // Init upload path.
    writeSpeeds->append(m_diskWrite.toDouble());

    if (writeSpeeds->size() > pointsNumber + 1) {
        writeSpeeds->pop_front();
    }
    double uploadMaxHeight = *std::max_element(writeSpeeds->begin(), writeSpeeds->end()) * 1.1;

    double maxHeight = qMax(downloadMaxHeight, uploadMaxHeight);

    QPainterPath tmpDownloadpath;
    getPainterPathByData(readSpeeds, tmpDownloadpath, maxHeight);
    downloadPath = tmpDownloadpath;

    QPainterPath tmpUploadpath;
    getPainterPathByData(writeSpeeds, tmpUploadpath, maxHeight);
    uploadPath = tmpUploadpath;


    //zong
    strs = diskTotalSize.split(" ");
    if (strs.size() == 2)
    {
        m_diskTotal = strs.at(0);
        m_diskTotalUnit = strs.at(1);
    }
    else
        return;

    strs = diskAvailable.split(" ");
    if (strs.size() == 2)
    {
        m_diskAvailable = strs.at(0);
        m_diskAvailableUnit = strs.at(1);
    }
//    else
//        return;

    update();
}

void DiskWidget::changeTheme(DApplicationHelper::ColorType themeType)
{
    switch (themeType) {
    case DApplicationHelper::LightType:
        m_icon = QIcon(QString(":/icons/hard-disk_light.png"));
        break;
    case DApplicationHelper::DarkType:
//        m_icon = QIcon(iconPathFromQrc("dark/icon_network_light.svg"));
        break;
    default:
        break;
    }

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
#ifndef THEME_FALLBACK_COLOR
    ltextColor = palette.color(DPalette::TextTitle);
#else
    ltextColor = palette.color(DPalette::Text);
#endif

    textColor = palette.color(DPalette::Text);

    textColor = palette.color(DPalette::Text);
    summaryColor = palette.color(DPalette::TextTips);
}

void DiskWidget::getPainterPathByData(QList<double> *listData, QPainterPath &path, qreal maxVlaue)
{
    qreal offsetX = 0;
    qreal distance = (260) * 1.0 / pointsNumber;
    int dataCount = listData->size();

    for (int i = 0;  i < dataCount - 1; i++) {
        QPointF sp = QPointF(offsetX, renderMaxHeight * listData->at(i) / maxVlaue);;
        QPointF ep = QPointF(offsetX + distance, renderMaxHeight * listData->at(i + 1) / maxVlaue);;

        offsetX += distance;

        QPointF c1 = QPointF((sp.x() + ep.x()) / 2.0, sp.y());
        QPointF c2 = QPointF((sp.x() + ep.x()) / 2.0, ep.y());
        path.cubicTo(c1, c2, ep);
    }
}

void DiskWidget::paintEvent(QPaintEvent *e)
{
    setFixedWidth(280);
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //裁剪绘制区域
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    painter.setClipPath(path);
    if (m_isHover) {
        painter.fillRect(rect(), QBrush(QColor(255, 255, 255, 50)));
    } else {
        painter.fillRect(rect(), QBrush(QColor(255, 255, 255, 0)));
    }

    //背景
    QRect titleRect(rect().x(), rect().y(), 280, 36);
    painter.fillRect(titleRect, QBrush(QColor(255, 255, 255,25)));
    QRect contentRect(rect().x(), rect().y()+36, 280, 167);
    if (m_isHover) {
        painter.fillRect(contentRect, QBrush(QColor(255, 255, 255,100)));
    } else {
        painter.fillRect(contentRect, QBrush(QColor(255, 255, 255,50)));
    }

    //标题
    painter.setFont(m_sectionFont);
    QFontMetrics fmTitle = painter.fontMetrics();
    int widthTitleTxt = fmTitle.width("磁盘");
    int heightTitleTxt = fmTitle.descent()+fmTitle.ascent();
    QRect netTitleRect(titleRect.x(), titleRect.y(), widthTitleTxt, heightTitleTxt);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter,"磁盘");

    //图标
    int iconSize = 20;
    QRect iconRect(titleRect.x()+titleRect.width()/2-netTitleRect.width() -5, titleRect.y() + qCeil((titleRect.height() - iconSize) / 2.) + 2,iconSize, iconSize);
    m_icon.paint(&painter, iconRect);


    //分隔符
    int sepHeight = 38;
    painter.setFont(m_contentFont);
    QRect separatorRect1(contentRect.x()+10, contentRect.y()+10, 1, sepHeight);
    painter.fillRect(separatorRect1, QBrush(QColor(0, 0, 0,20)));
    QRect separatorRect2(contentRect.x()+10+87*1, contentRect.y()+10, 1, sepHeight);
    painter.fillRect(separatorRect2, QBrush(QColor(0, 0, 0,20)));
    QRect separatorRect3(contentRect.x()+10+87*2, contentRect.y()+10, 1, sepHeight);
    painter.fillRect(separatorRect3, QBrush(QColor(0, 0, 0,20)));
    QRect separatorRect4(contentRect.x()+10+87*3, contentRect.y()+10, 1, sepHeight);
    painter.fillRect(separatorRect4, QBrush(QColor(0, 0, 0,20)));


    int bulletSize = 6;
    int padleft = 36;
    auto spacing = 20;
    auto margin = 10;

    // Draw network summary.
    QString recvTitle = DApplication::translate("Process.Graph.View", "磁盘读取");//Download
    QString recvTotalTitle = DApplication::translate("Process.Graph.View", "总容量");//Total Received"
    QString sentTitle = DApplication::translate("Process.Graph.View", "磁盘写入");//Upload
    QString sentTotalTitle = DApplication::translate("Process.Graph.View", "可用空间");//otal Sent

    QFontMetrics fmContent(m_contentFont);
    QFontMetrics fmContentUnit(m_contentUnitFont);
    QFontMetrics fmSubContent(m_subContentFont);
    QRect contentNetRect(padleft, separatorRect1.y() + separatorRect1.height(),
                      rect().x() + rect().width() - padleft, 1);

    //正在接受
    QRect recv1Rect(contentNetRect.x(), contentNetRect.y(),
                          fmContent.size(Qt::TextSingleLine, m_diskRead).width(), fmContent.height());
    //正在接受单位
    QRect recv2UnitRect(recv1Rect.x()+recv1Rect.width(), contentNetRect.y()+8,
                              fmContentUnit.size(Qt::TextSingleLine, m_diskReadUnit).width(), fmContent.height());
    //"总计接受"的值
    QRect totalReceiveRect(qCeil((contentNetRect.width() + spacing) / 2) + spacing, recv1Rect.y(),
                  contentNetRect.width() - recv1Rect.width() - spacing, recv1Rect.height());
    QRect Receive1Rect(totalReceiveRect.x(), totalReceiveRect.y(),
                          fmContent.size(Qt::TextSingleLine, m_diskTotal).width(), fmContent.height());
    QRect Receive2Rect(Receive1Rect.x()+Receive1Rect.width(), Receive1Rect.y()+8,
                              fmContentUnit.size(Qt::TextSingleLine, m_diskTotalUnit).width(), fmContent.height());

    //正在接受标题
    QRect recvTitleRect(contentNetRect.x(), recv1Rect.y() + recv1Rect.height(),
                        fmContent.size(Qt::TextSingleLine, recvTitle).width(), fmSubContent.height());
    //"总计接受"标题
    QRect totalReceiveTitleRect(totalReceiveRect.x(), recvTitleRect.y(), totalReceiveRect.width(), recvTitleRect.height());


    //"正在发送"的值
    QRect Sent1Rect(recvTitleRect.x(), recvTitleRect.y() + recvTitleRect.height()+10,
                          fmContent.size(Qt::TextSingleLine, m_diskWrite).width(), fmContent.height());
    QRect Sent2Rect(Sent1Rect.x()+Sent1Rect.width(), Sent1Rect.y() + 8,
                              fmContentUnit.size(Qt::TextSingleLine, m_diskWriteUnit).width(), fmContent.height());
    //"总计发送"的值
    QRect crect32(totalReceiveRect.x(), Sent1Rect.y(), totalReceiveRect.width(), recv1Rect.height());
    QRect SentTotal1Rect(crect32.x(), crect32.y(),
                          fmContent.size(Qt::TextSingleLine, m_diskAvailable).width(), fmContent.height());
    QRect SentTotal2Rect(SentTotal1Rect.x()+SentTotal1Rect.width(), SentTotal1Rect.y()+8,
                              fmContentUnit.size(Qt::TextSingleLine, m_diskAvailableUnit).width(), fmContent.height());


    //正在发送标题
    QRect sentTitleRect(contentNetRect.x(), Sent1Rect.y() + Sent1Rect.height(),
                  fmContent.size(Qt::TextSingleLine, recvTitle).width(), fmSubContent.height());
    QRect sentTotalTitleRect(totalReceiveRect.x(), sentTitleRect.y(), totalReceiveRect.width(), recvTitleRect.height());
    QRectF r1Ind(contentNetRect.x() - margin, recvTitleRect.y() + qCeil((recvTitleRect.height() - bulletSize) / 2.), bulletSize,
                 bulletSize);
    QRectF r2Ind(contentNetRect.x() - margin, sentTitleRect.y() + qCeil((sentTitleRect.height() - bulletSize) / 2.), bulletSize,
                 bulletSize);

    //正在接受
    painter.setPen(ltextColor);
    painter.setFont(m_contentFont);
    painter.drawText(recv1Rect, Qt::AlignLeft | Qt::AlignHCenter,m_diskRead);
    //正在接受的单位
    painter.setPen(ltextColor);
    painter.setFont(m_contentUnitFont);
    painter.drawText(recv2UnitRect, Qt::AlignLeft | Qt::AlignHCenter,m_diskReadUnit);

    //总计接受
    painter.setPen(ltextColor);
    painter.setFont(m_contentFont);
    painter.drawText(Receive1Rect, Qt::AlignLeft | Qt::AlignVCenter,m_diskTotal);
    //总计接受的单位
    painter.setPen(ltextColor);
    painter.setFont(m_contentUnitFont);
    painter.drawText(Receive2Rect, Qt::AlignLeft | Qt::AlignHCenter, m_diskTotalUnit);

    //正在发送
    painter.setPen(ltextColor);
    painter.setFont(m_contentFont);
    painter.drawText(Sent1Rect, Qt::AlignLeft | Qt::AlignVCenter,m_diskWrite);
    //正在发送的单位
    painter.setPen(ltextColor);
    painter.setFont(m_contentUnitFont);
    painter.drawText(Sent2Rect, Qt::AlignLeft | Qt::AlignHCenter, m_diskWriteUnit);

    //总发送
    painter.setPen(ltextColor);
    painter.setFont(m_contentFont);
    painter.drawText(SentTotal1Rect, Qt::AlignLeft | Qt::AlignVCenter,m_diskAvailable);
    //总的单位
    painter.setPen(ltextColor);
    painter.setFont(m_contentUnitFont);
    painter.drawText(SentTotal2Rect, Qt::AlignLeft | Qt::AlignHCenter, m_diskAvailableUnit);


    //标题
    painter.setPen(summaryColor);
    painter.setFont(m_subContentFont);
    painter.drawText(recvTitleRect, Qt::AlignLeft | Qt::AlignVCenter,recvTitle);
    painter.drawText(sentTitleRect, Qt::AlignLeft | Qt::AlignVCenter, sentTitle);
    painter.drawText(totalReceiveTitleRect, Qt::AlignLeft | Qt::AlignVCenter, recvTotalTitle);
    painter.drawText(sentTotalTitleRect, Qt::AlignLeft | Qt::AlignVCenter,sentTotalTitle);

    QPainterPath path1, path2;
    path1.addEllipse(r1Ind);
    path2.addEllipse(r2Ind);

    painter.fillPath(path1, m_diskReadColor);
    painter.fillPath(path2, m_diskWriteColor);

    //quxian
    QPainterPath framePath;
    QRect chartRect(separatorRect1.x(), separatorRect1.y(), contentRect.width()-20, 38);
//    framePath.addRect(chartRect);


    QBrush recvBrush(m_diskReadColor);
    QBrush sentBrush(m_diskWriteColor);
    qreal networkCurveWidth = 1.2;

    painter.setRenderHint(QPainter::Antialiasing, true);//反锯齿
    QPainterPath clip;
    clip.addRect(chartRect);
    painter.setClipPath(clip);
    painter.translate(chartRect.x() + 2, chartRect.y() + chartRect.height() / 2 - 2);
    painter.scale(1, -1);
    painter.setPen(QPen(recvBrush, networkCurveWidth));
    painter.drawPath(downloadPath);

    painter.translate(0, -5);
    painter.scale(1, -1);
    painter.setPen(QPen(sentBrush, networkCurveWidth));
    painter.drawPath(uploadPath);
}

bool DiskWidget::eventFilter(QObject *target, QEvent *event)
{
    if (target == this) {
        if (event->type() == QEvent::Enter) {
            m_isHover = true;
            update();
            return true;
        }
        if (event->type() == QEvent::Leave) {
            m_isHover = false;
            update();
            return true;
        }
    }
    return QWidget::eventFilter(target, event);
}

void DiskWidget::changeFont(const QFont &font)
{
    m_sectionFont = font;
    m_sectionFont.setFamily("SourceHanSansSC");
    m_sectionFont.setWeight(QFont::DemiBold);
    m_sectionFont.setPointSizeF(m_sectionFont.pointSizeF());

    m_contentFont = font;
    m_contentFont.setFamily("HelveticaNeueLT");
    m_contentFont.setWeight(QFont::Normal);
    m_contentFont.setPointSizeF(m_contentFont.pointSizeF()+5);

    m_contentUnitFont = font;
    m_contentUnitFont.setFamily("HelveticaNeueLT");
    m_contentUnitFont.setWeight(QFont::Normal);
    m_contentUnitFont.setPointSizeF(m_contentUnitFont.pointSizeF());

    m_subContentFont = font;
    m_subContentFont.setFamily("SourceHanSansSC");
    m_subContentFont.setWeight(QFont::ExtraLight);
    m_subContentFont.setPointSizeF(m_subContentFont.pointSizeF()-2 );

}
