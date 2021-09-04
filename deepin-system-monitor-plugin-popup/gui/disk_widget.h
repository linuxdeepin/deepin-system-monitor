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

#ifndef DISK_WIDGET_H
#define DISK_WIDGET_H

#include <QWidget>
#include <QPainterPath>
#include <QIcon>
#include <DApplicationHelper>


DWIDGET_USE_NAMESPACE

class DiskWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DiskWidget(QWidget *parent = nullptr);
    ~DiskWidget();

public slots:
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *target, QEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;



private:
    void changeTheme(DApplicationHelper::ColorType themeType);
    void changeFont(const QFont &font);

    void getPainterPathByData(QList<double> *listData, QPainterPath &path, qreal maxVlaue);

private:
    QColor m_diskReadColor {"#8F88FF"};
    QColor m_diskWriteColor {"#6AD787"};

    QList<double> *readSpeeds;
    QList<double> *writeSpeeds;
    QPainterPath readDiskPath;
    QPainterPath writeDiskPath;

    int pointsNumber = 25;
    int pointerRadius = 6;
    int renderMaxHeight = 18;
    QColor summaryColor;
    QColor textColor;
    QColor ltextColor;
    QFont m_sectionFont;
    QFont m_contentFont;
    QFont m_contentUnitFont;
    QFont m_subContentFont;

    QIcon m_icon {};

//    qreal m_recvBps {};
//    qreal m_sentBps {};
    QString m_diskRead;
    QString m_diskTotal;
    QString m_diskWrite;
    QString m_diskAvailable;

    QString m_diskReadUnit;
    QString m_diskTotalUnit;
    QString m_diskWriteUnit;
    QString m_diskAvailableUnit;

    bool m_isHover = false;
    int m_titleTrans = 0;
    int m_contentTrans = 0;
    int m_hoverTrans = 0;
};

#endif
