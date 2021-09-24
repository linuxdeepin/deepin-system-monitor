/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan  <yukuan@uniontech.com>
*
* Maintainer: yukuan  <yukuan@uniontech.com>
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
#include "systemmonitortipswidget.h"

#include <QAccessible>
#include <QPainter>
#include <DApplication>

DWIDGET_USE_NAMESPACE

SystemMonitorTipsWidget::SystemMonitorTipsWidget(QWidget *parent)
    : QFrame(parent)
{
}

void SystemMonitorTipsWidget::setSystemMonitorTipsText(QStringList strList)
{
    m_textList  = strList;

    //  [   CPU: 100.0%    ↓ 1000kb/s  ]
    // 获取strList字符串，并存入cpu,mem,upLoad,download中
    QString cpu;
    QString mem;
    QString upLoad;
    QString downLoad;
    for (int i = 0; i < m_textList.size(); ++i) {
        if (i == 0)
            cpu = m_textList.at(i);
        else if (i == 1)
            mem = m_textList.at(i);
        else if (i == 2)
            downLoad = m_textList.at(i);
        else if (i == 3)
            upLoad = m_textList.at(i);
        else
            qDebug() << "do not set the text";
    }

    // 设置左侧字符串宽度
    if (cpu.length() == 3) {
        m_leftWidth = fontMetrics().width(QString(" ") + DApplication::translate("Plugin.cpu", "CPU") + QString(": 0") + cpu + QString("%") + QString(" "));
    } else {
        m_leftWidth = fontMetrics().width(QString(" ") + DApplication::translate("Plugin.cpu", "CPU") + QString(": ") + cpu + QString("%") + QString(" "));
    }
    // 左侧宽度预留10个像素
    m_leftWidth += 10;

    // 设置右侧字符串宽度
    if (downLoad.length() == 3) {
        m_rightWidth = fontMetrics().width(QString("↓000") + downLoad + QString(" "));
    } else if (downLoad.length() == 4) {
        m_rightWidth = fontMetrics().width(QString("↓00") + downLoad + QString(" "));
    } else if (downLoad.length() == 5) {
        m_rightWidth = fontMetrics().width(QString("↓0") + downLoad + QString(" "));
    } else {
        m_rightWidth = fontMetrics().width(QString("↓") + downLoad + QString(" "));
    }
    // 设置右侧字符串宽度预留15个像素
    m_rightWidth += 15;


    // 设置当前窗体固定大小
    setFixedSize(m_leftWidth + m_rightWidth, fontMetrics().boundingRect(m_textList.at(0)).height() * 2);
    update();
}


void SystemMonitorTipsWidget::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    int rectHeight = rect().height();
    QPainter painter(this);
    painter.setPen(QPen(palette().brightText(), 1));

    QTextOption optionLeft;
    optionLeft.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QTextOption optionRight;
    optionRight.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QTextOption optionMid;
    optionMid.setAlignment(Qt::AlignCenter);

    // 获取strList字符串，并存入cpu,mem,upLoad,download中
    QString cpu;
    QString mem;
    QString upLoad;
    QString downLoad;
    for (int i = 0; i < m_textList.size(); ++i) {
        if (i == 0)
            cpu = m_textList.at(i);
        else if (i == 1)
            mem = m_textList.at(i);
        else if (i == 2)
            downLoad = m_textList.at(i);
        else if (i == 3)
            upLoad = m_textList.at(i);
        else
            qDebug() << "do not set the text";
    }

    // 当没有数据更新时，设置默认的悬浮框内容
    if (m_textList.size() == 0) {
        cpu = "0.0";
        mem = "0.0";
        upLoad = "0KB/s";
        downLoad = "0KB/s";
    }

    int specialCharaWidth = fontMetrics().width(QString("↓"));
    painter.drawText(QRectF(0.0, 0.0, m_leftWidth, rectHeight / 2.0), QString(" ") + DApplication::translate("Plugin.cpu", "CPU") + QString(": ") + cpu + QString("%"), optionLeft);
    painter.setPen(QPen(QColor("red"), 1));
    painter.drawText(QRectF(m_leftWidth, 0.0, specialCharaWidth, rectHeight / 2.0), QString("↓"), optionMid);
    painter.setPen(QPen(palette().brightText(), 1));
    painter.drawText(QRectF(m_leftWidth + specialCharaWidth, 0, m_rightWidth, rectHeight / 2.0), downLoad, optionLeft);

    specialCharaWidth = fontMetrics().width(QString("↑"));
    painter.drawText(QRectF(0, rectHeight / 2.0, m_leftWidth, rectHeight / 2.0), QString(" ") + DApplication::translate("Plugin.mem", "MEM") + QString(": ") + mem + QString("%"), optionLeft);
    painter.setPen(QPen(QColor("blue"), 1));
    painter.drawText(QRectF(m_leftWidth, rectHeight / 2.0, specialCharaWidth, rectHeight / 2.0), QString("↑"), optionMid);
    painter.setPen(QPen(palette().brightText(), 1));
    painter.drawText(QRectF(m_leftWidth + specialCharaWidth, rectHeight / 2.0, m_rightWidth, rectHeight / 2.0), upLoad, optionLeft);
}

bool SystemMonitorTipsWidget::event(QEvent *event)
{
    // 字体切换事件捕捉，当前获取到字体切换信号时，更新当前的悬浮框界面
    if (event->type() == QEvent::FontChange) {
        if (m_textList.size() > 0)
            setSystemMonitorTipsText(m_textList);
        else
            setSystemMonitorTipsText(QStringList() << "0.0" << "0.0" << "0KB/s" << "0KB/s");
    }
    return QFrame::event(event);
}
