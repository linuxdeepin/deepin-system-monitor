/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      yk <yukuan@uniontech.com>
* Maintainer:  yk <yukuan@uniontech.com>
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
#ifndef CUSTOMBUTTONBOX_H
#define CUSTOMBUTTONBOX_H

#include <DButtonBox>

DWIDGET_USE_NAMESPACE

class CustomButtonBox : public DButtonBox
{
    Q_OBJECT
public:
    explicit CustomButtonBox(QWidget *parent = nullptr);

protected:
    //!
    //! \brief focusInEvent 焦点切换事件
    //! \param event
    //!
    void focusInEvent(QFocusEvent *event);
signals:

public slots:
};

#endif // CUSTOMBUTTONBOX_H
