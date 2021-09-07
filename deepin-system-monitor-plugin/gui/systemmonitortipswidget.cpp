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

SystemMonitorTipsWidget::SystemMonitorTipsWidget(QWidget *parent)
    : QFrame(parent)
{

}

void SystemMonitorTipsWidget::setSystemMonitorTipsText(QStringList strList)
{
    m_textList  = strList;
    //  [   CPU: 100.0%    ↓ 1000kb/s  ]

    if (m_textList.size() > 0) {
        setFixedSize(fontMetrics().boundingRect(m_textList.at(0).at(0)).width() * 25, fontMetrics().boundingRect(m_textList.at(0)).height() * 2);
        update();
    }
}


void SystemMonitorTipsWidget::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.setPen(QPen(palette().brightText(), 1));

    QTextOption optionLeft;
    optionLeft.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QTextOption optionRight;
    optionRight.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QTextOption optionMid;
    optionMid.setAlignment(Qt::AlignCenter);

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
        else { }
    }

    if (m_textList.size() == 0) {
        cpu = "0.0";
        mem = "0.0";
        upLoad = "0kb/s";
        downLoad = "0kb/s";
    }

    int specialCharaWidth = fontMetrics().boundingRect(QString("↓")).width();
    painter.drawText(QRectF(0.0, 0.0, rect().width() / 2.0, rect().height() / 2.0), QString("   CPU: %1%").arg(cpu), optionLeft);
    painter.save();
    painter.setPen(QPen(QColor("red"), 1));
    painter.drawText(QRectF(rect().width() / 2.0, 0.0, specialCharaWidth, rect().height() / 2.0), QString("↓"), optionMid);
    painter.restore();
    painter.drawText(QRectF(rect().width() / 2.0 + specialCharaWidth, 0, rect().width() / 2.0 - specialCharaWidth, rect().height() / 2.0), downLoad, optionLeft);

    specialCharaWidth = fontMetrics().boundingRect(QString("↑")).width();
    painter.drawText(QRectF(0, rect().height() / 2.0, rect().width() / 2.0, rect().height() / 2.0), QString("   MEM: %1%").arg(mem), optionLeft);
    painter.save();
    painter.setPen(QPen(QColor("blue"), 1));
    painter.drawText(QRectF(rect().width() /  2.0, rect().height() / 2.0, specialCharaWidth, rect().height() / 2.0), QString("↑"), optionMid);
    painter.restore();
    painter.drawText(QRectF(rect().width()  / 2.0 + specialCharaWidth, rect().height() / 2.0, rect().width() / 2.0 - specialCharaWidth, rect().height() / 2.0), upLoad, optionLeft);
}

bool SystemMonitorTipsWidget::event(QEvent *event)
{
    if (event->type() == QEvent::FontChange) {
        if (m_textList.size() > 0)
            setSystemMonitorTipsText(m_textList);
        else
            setSystemMonitorTipsText(QStringList() << "0.0" << "0.0" << "0kb/s" << "0kb/s");
    }
    return QFrame::event(event);
}
