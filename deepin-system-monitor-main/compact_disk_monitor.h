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

#ifndef COMPACTDISKMONITOR_H
#define COMPACTDISKMONITOR_H

#include <QWidget>
#include <QPainterPath>

class CompactDiskMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit CompactDiskMonitor(QWidget *parent = nullptr);
    ~CompactDiskMonitor();

public slots:
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event);

private:
    void changeFont(const QFont &font);
    void getPainterPathByData(QList<double> *listData, QPainterPath &path, qreal maxVlaue);

private:
    QList<qreal> *readSpeeds;
    QList<qreal> *writeSpeeds;
    qreal m_readBps {};
    qreal m_writeBps {};

    int m_bulletSize = 6;

    QColor m_diskReadColor {"#8F88FF"};
    QColor m_diskWriteColor {"#6AD787"};

    QPainterPath readPath;
    QPainterPath writePath;

    int renderMaxHeight = 30;

    QFont m_tagFont;
    QFont m_statFont;
};

#endif
