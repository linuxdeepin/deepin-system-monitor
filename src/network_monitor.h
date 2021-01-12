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

#ifndef NETWORKMONITOR_H
#define NETWORKMONITOR_H

#include <DApplicationHelper>

#include <QIcon>
#include <QWidget>
#include <QPainterPath>

DWIDGET_USE_NAMESPACE

class NetworkMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkMonitor(QWidget *parent = nullptr);
    ~NetworkMonitor();

signals:
    void signalArrowClicked(QPoint pos);

public slots:
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    void changeTheme(DApplicationHelper::ColorType themeType);
    void changeFont(const QFont &font);

private:
    QIcon m_icon;

    QList<double> *downloadSpeeds;
    QList<double> *uploadSpeeds;
    QPainterPath downloadPath;
    QPainterPath uploadPath;

    QColor downloadColor = "#55D500";
    QColor recvStartColor {"#FF1212"};
    QColor recvEndColor {"#E0FF00"};
    QColor sentStartColor {"#2357FF"};
    QColor sentEndColor {"#00A6E3"};
    QColor uploadColor = "#C362FF";
    QColor m_recvIndicatorColor {"#E14300"};
    QColor m_sentIndicatorColor {"#004EEF"};
    QColor summaryColor;
    QColor textColor;
    QColor ltextColor;
    QColor m_frameColor;

    QFont m_sectionFont;
    QFont m_contentFont;
    QFont m_subContentFont;

    int downloadRenderMaxHeight = 20;
    int downloadRenderPaddingX = 13;
    int downloadRenderPaddingY = 50;
    int downloadRenderSize = 9;
    int downloadWaveformsRenderOffsetX = 4;
    int downloadWaveformsRenderOffsetY = 120;
    int gridSize = 10;
    int pointsNumber = 60;
    int textPadding = 12;
    int titleRenderOffsetX = 20;
    int titleRenderSize = 20;
    int uploadRenderMaxHeight = 20;
    int uploadRenderPaddingX = 13;
    int uploadRenderPaddingY = 70;
    int uploadRenderSize = 9;
    int uploadWaveformsRenderOffsetY = -5;
    int waveformRenderPadding = 4;

    QRect m_arrowRect;

    qreal m_recvBps {};
    qreal m_sentBps {};
    qulonglong m_totalRecvBytes {};
    qulonglong m_totalSentBytes {};
};

#endif
