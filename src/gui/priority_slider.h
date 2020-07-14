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
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PRIORITY_SLIDER_H
#define PRIORITY_SLIDER_H

#include <DSlider>

DWIDGET_USE_NAMESPACE

class PrioritySlider : public DSlider
{
    Q_OBJECT

public:
    PrioritySlider(Qt::Orientation orientation = Qt::Horizontal, QWidget *parent = nullptr);

Q_SIGNALS:
    void sizeChanged(const QSize &size);

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif  // PRIORITY_SLIDER_H
