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

#ifndef PROCESS_WIDGET_H
#define PROCESS_WIDGET_H

#include "process_table_view.h"

#include <QWidget>
#include <QPainterPath>
#include <QIcon>
#include <DApplicationHelper>

DWIDGET_USE_NAMESPACE

class ProcessWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProcessWidget(QWidget *parent = nullptr);
    ~ProcessWidget();

public slots:
    void updateStatus(qreal totalCpuPercent, const QList<qreal> cPercents);

protected:
    void paintEvent(QPaintEvent *event);
    bool eventFilter(QObject *target, QEvent *event);

private:
    void changeTheme(DApplicationHelper::ColorType themeType);
    void changeFont(const QFont &font);

private:
    QList<QList<qreal>> cpuPercents;
    QList<QPainterPath> cpuPaths;
    QList<QColor> cpuColors;
    int cpuRenderMaxHeight = 80;
    int cpuWaveformsRenderOffsetY = 112;
    int gridSize = 10;
    int pointsNumber = 25;
    int waveformRenderPadding = 20;
    double totalCpuPercent = 0;

    int pointerRadius = 6;

    QColor summaryColor;
    QColor textColor;
    QColor ltextColor;
    QFont m_sectionFont;
    QFont m_contentFont;
    QFont m_subContentFont;

    QIcon m_icon {};

    qreal m_recvBps {};
    qreal m_sentBps {};
    qulonglong m_totalRecvBytes {};
    qulonglong m_totalSentBytes {};

    ProcessTableView *m_processTableView;

    bool m_isHover = false;
    int m_titleTrans = 0;
    int m_contentTrans = 0;
    int m_hoverTrans = 0;
};

#endif
