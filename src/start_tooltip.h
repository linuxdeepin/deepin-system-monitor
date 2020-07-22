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

#ifndef STARTTOOLTIP_H
#define STARTTOOLTIP_H

#include <dwindowmanager.h>

#include <QWidget>

DWM_USE_NAMESPACE

class StartTooltip : public QWidget
{
    Q_OBJECT

public:
    StartTooltip(QWidget *parent = nullptr);
    ~StartTooltip();

    void setWindowManager(DWindowManager *wm);

protected:
    bool eventFilter(QObject *, QEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    QPixmap iconImg;
    QString text;
    DWindowManager *windowManager;
};

#endif
