/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan <yukuan@uniontech.com>
*
* Maintainer: yukuan <yukuan@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BASE_COMMANDLINK_BUTTON_H
#define BASE_COMMANDLINK_BUTTON_H

#include <DCommandLinkButton>
/**
 * @brief The BaseCommandLinkButton class The DCommandLinkButton subclass,which will be catched the keyBoard Event.
 */
class BaseCommandLinkButton : public Dtk::Widget::DCommandLinkButton
{
    Q_OBJECT
public:
    explicit BaseCommandLinkButton(const QString name, QWidget *parent);
    virtual ~BaseCommandLinkButton();

protected:
    /**
     * @brief eventFilter Filters events if this object has been installed as an event filter for the watched object.
     * @param obj the obj which will be filter.
     * @param event currentBaseCommandLinkButton Event.
     * @return return the eventFilter function will be complete or uncomplete.
     */
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // BASE_COMMANDLINK_BUTTON_H
