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

#ifndef CPU_WIDGET_H
#define CPU_WIDGET_H

#include <QWidget>
#include <QPainterPath>
#include <QIcon>
#include <DApplicationHelper>

DWIDGET_USE_NAMESPACE

class CpuWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CpuWidget(QWidget *parent = nullptr);
    ~CpuWidget();

public slots:
//    void updateStatus(qreal totalCpuPercent, const QList<qreal> cPercents);
//    void slot_onCpuPer(QString cpuPer);
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event) override;

    bool eventFilter(QObject *target, QEvent *event) override;

    //!
    //! \brief mousePressEvent 鼠标压下事件，唤醒系统监视器主进程，并跳转到CPU详情界面
    //! \param event
    //!
    void mousePressEvent(QMouseEvent *event) override;

private:
    void initConnection();
    void changeTheme(DApplicationHelper::ColorType themeType);
    void changeFont(const QFont &font);
    void getPainterPathByData(QList<double> *listData, QPainterPath &path, qreal maxVlaue);

private:
    QList<QList<qreal>> cpuPercents;
    QList<QPainterPath> cpuPaths;
    QList<double> *downloadSpeeds;
    QPainterPath downloadPath;

//    ReadCpuThread *m_readCpuThread = nullptr;

    int cpuRenderMaxHeight = 80;
    int cpuWaveformsRenderOffsetY = 112;
    int gridSize = 10;
    int pointsNumber = 25;
    int waveformRenderPadding = 20;
    int numCPU;
    double totalCpuPercent = 0;
    qreal m_cpuPer = 0;
    int m_width = 0;

    int pointerRadius = 6;

    QColor summaryColor;
    QColor textColor;
    QColor ltextColor;

    QFont m_TitleFont;
    QFont m_contentFont;
    QFont m_contentUnitFont;
    QFont m_subContentFont;
    QIcon m_icon {};

    bool m_isHover = false;

    int m_titleTrans = 0;
    int m_contentTrans = 0;
    int m_hoverTrans = 0;
};

#endif
