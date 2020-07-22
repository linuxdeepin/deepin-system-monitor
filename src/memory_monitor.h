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

#ifndef MemoryMONITOR_H
#define MemoryMONITOR_H

#include <DApplicationHelper>

#include <QIcon>
#include <QWidget>

DWIDGET_USE_NAMESPACE

class Settings;
class QPropertyAnimation;

class MemoryMonitor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    MemoryMonitor(QWidget *parent = nullptr);
    ~MemoryMonitor();

public slots:
    void updateStatus(qulonglong uMemory, qulonglong tMemory,
                      qulonglong uSwap, qulonglong tSwap);

private:
    void changeTheme(DApplicationHelper::ColorType themeType);
    void changeFont(const QFont &font);

protected:
    void paintEvent(QPaintEvent *event);

    qreal progress() const
    {
        return m_progress;
    }
    void setProgress(qreal p)
    {
        m_progress = p;
    }

private:
    QIcon m_icon {};

    QColor memoryBackgroundColor;
    QColor memoryColor {"#00C5C0"};
    QColor memoryForegroundColor {"#00C5C0"};
    QColor numberColor;
    QColor summaryColor;
    QColor swapBackgroundColor;
    QColor swapColor {"#FEDF19"};
    QColor swapForegroundColor {"#FEDF19"};
    QColor textColor;
    QColor ltextColor;

    double memoryBackgroundOpacity = 0.1;
    double memoryForegroundOpacity = 1.0;
    double swapBackgroundOpacity = 0.1;
    double swapForegroundOpacity = 1.0;
    int animationIndex = 0;
    int insideRingRadius = 41;
    int outsideRingRadius = 48;
    int pointerRadius = 3;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 9;
    int ringCenterPointerX;
    int ringCenterPointerY = 70;
    int ringWidth = 6;

    qulonglong m_prevUsedMemory = 0;
    qulonglong m_prevUsedSwap = 0;
    qulonglong m_totalMemory = 0;
    qulonglong m_totalSwap = 0;
    qulonglong m_usedMemory = 0;
    qulonglong m_usedSwap = 0;

    QFont m_titleFont;
    QFont m_contentFont;
    QFont m_subContentFont;
    QFont m_memPercentFont;

    qreal m_progress {};
    QPropertyAnimation *m_animation {};

    Settings *m_settings;
    DApplicationHelper::ColorType m_themeType;
};

#endif
