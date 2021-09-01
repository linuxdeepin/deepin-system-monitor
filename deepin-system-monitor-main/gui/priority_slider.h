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

#ifndef PRIORITY_SLIDER_H
#define PRIORITY_SLIDER_H

#include <DSlider>

DWIDGET_USE_NAMESPACE

/**
 * @brief Priority slider sub class
 */
class PrioritySlider : public DSlider
{
    Q_OBJECT

public:
    /**
     * @brief PrioritySlider Constructor
     * @param orientation Slider orientation
     * @param parent Parent object
     */
    PrioritySlider(Qt::Orientation orientation = Qt::Horizontal, QWidget *parent = nullptr);

Q_SIGNALS:
    /**
     * @brief sizeChanged Size changed signal
     * @param size New size
     */
    void sizeChanged(const QSize &size);

protected:
    /**
     * @brief resizeEvent Resize event handler
     * @param event Resize event
     */
    void resizeEvent(QResizeEvent *event) override;
};

#endif  // PRIORITY_SLIDER_H
