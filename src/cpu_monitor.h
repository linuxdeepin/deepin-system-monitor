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

#ifndef CpuMONITOR_H
#define CpuMONITOR_H

#include <DApplicationHelper>

#include <QIcon>
#include <QList>
#include <QWidget>
#include <QPainterPath>
#include <DCommandLinkButton>

DWIDGET_USE_NAMESPACE

class Settings;
class QPropertyAnimation;
class CPUInfoModel;
class BaseCommandLinkButton;

class CpuMonitor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    explicit CpuMonitor(QWidget *parent = nullptr);
    ~CpuMonitor();

public:
    void setDetailButtonVisible(bool visible);

signals:
    void signalDetailInfoClicked();

public slots:
    void changeTheme(DApplicationHelper::ColorType themeType);
    void updateStatus();
    void onDetailInfoClicked();

private:
    void resizeItemWidgetRect();

private:
    void changeFont(const QFont &font);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

    qreal progress() const
    {
        return m_progress;
    }
    void setProgress(qreal p)
    {
        m_progress = p;
    }

private:
    QIcon m_icon;

    QList<double> *cpuPercents;
    QPainterPath cpuPath;
    QColor numberColor;
    QColor ringBackgroundColor;
    QColor ringForegroundColor {"#0081FF"};
    QColor textColor;
    double ringBackgroundOpacity = 0.1;
    double ringForegroundOpacity = 1;
    int animationIndex = 0;
    int cpuRenderMaxHeight = 45;
    int percentRenderOffsetY = 131;
    int pointsNumber = 28;
    int ringRadius = 90;
    int ringRenderOffsetY = 86;
    int ringWidth = 8;
    int titleAreaPaddingX = 5;
    int titleRenderOffsetY = 161;
    int waveformsRenderOffsetX;
    int waveformsRenderOffsetY = 96;

    Settings *m_settings;

    QFont m_cpuUsageFont;
    QFont m_cpuDisplayFont;
    QFont m_detailFont;

    qreal m_progress {};
    QPropertyAnimation *m_animation {};
    CPUInfoModel *m_cpuInfomodel;

    BaseCommandLinkButton *m_detailButton;
};

#endif
