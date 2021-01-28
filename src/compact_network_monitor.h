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

#ifndef COMPACTNETWORKMONITOR_H
#define COMPACTNETWORKMONITOR_H

#include <DApplicationHelper>

#include <QWidget>
#include <QPainterPath>

DWIDGET_USE_NAMESPACE

class CompactNetworkMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit CompactNetworkMonitor(QWidget *parent = nullptr);
    ~CompactNetworkMonitor();

public slots:
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event);

private:
    void changeTheme(DApplicationHelper::ColorType themeType);
    void changeFont(const QFont &font);
    void getPainterPathByData(QList<double> *listData, QPainterPath &path, qreal maxVlaue);

private:
    QList<double> *downloadSpeeds;
    QList<double> *uploadSpeeds;
    QPainterPath downloadPath;
    QPainterPath uploadPath;

    QColor recvColor {"#E14300"};
    QColor sentColor {"#004EEF"};
    QColor m_recvIndicatorColor {"#E14300"};
    QColor m_sentIndicatorColor {"#004EEF"};
    QColor summaryColor;
    QColor textColor;
    QColor m_frameColor;

    QFont m_sectionFont;
    QFont m_contentFont;
    QFont m_subContentFont;

    int renderMaxHeight = 20;

    qreal m_recvBps {};
    qreal m_sentBps {};
    qulonglong m_totalRecvBytes {};
    qulonglong m_totalSentBytes {};
};

#endif
