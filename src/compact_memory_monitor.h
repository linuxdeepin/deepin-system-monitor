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

#ifndef COMPACTMEMORYMONITOR_H
#define COMPACTMEMORYMONITOR_H

#include <QWidget>

class QPropertyAnimation;
class MemStatModel;
class MemInfoModel;

namespace core {
namespace system {
class MemInfo;
}
}

class CompactMemoryMonitor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    explicit CompactMemoryMonitor(QWidget *parent);
    ~CompactMemoryMonitor();

protected:
    void paintEvent(QPaintEvent *event);

private:
    void changeTheme(int themeType);
    void changeFont(const QFont &font);

    qreal progress() const;
    void setProgress(qreal p);

private slots:
    void onStatInfoUpdated();
    void animationFinshed();
    void onValueChanged();

private:
    QColor memoryBackgroundColor;
    QColor memoryColor {"#00C5C0"};
    QColor memoryForegroundColor {"#00C5C0"};
    QColor numberColor;
    QColor summaryColor;
    QColor swapBackgroundColor;
    QColor swapColor {"#FEDF19"};
    QColor swapForegroundColor {"#FEDF19"};
    QColor textColor;

    double memoryBackgroundOpacity = 0.1;
    double memoryForegroundOpacity = 1.0;
    double swapBackgroundOpacity = 0.1;
    double swapForegroundOpacity = 1.0;
    int insideRingRadius = 41;
    int memoryRenderSize = 9;
    int outsideRingRadius = 48;
    int pointerRadius = 3;
    int ringCenterPointerX;
    int ringCenterPointerY = 45;
    int ringWidth = 6;

    core::system::MemInfo *m_memInfo {};

    qreal m_progress {};
    qreal m_lastMemPercent = 0.;
    qreal m_lastSwapPercent = 0.;
    QPropertyAnimation *m_animation {};

    QFont m_contentFont;
    QFont m_subContentFont;
    QFont m_memPercentFont;
};

#endif
