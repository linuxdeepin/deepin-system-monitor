/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "priority_slider.h"

#include <QResizeEvent>

// constructor
PrioritySlider::PrioritySlider(Qt::Orientation orientation, QWidget *parent)
    : DSlider(orientation, parent)
{
}

// resize event handler
void PrioritySlider::resizeEvent(QResizeEvent *event)
{
    DSlider::resizeEvent(event);
    // emit size changed signal
    Q_EMIT sizeChanged(event->size());
}
