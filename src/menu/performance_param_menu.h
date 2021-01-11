/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     leiyu <leiyu@uniontech.com>
*
* Maintainer: leiyu <leiyu@uniontech.com>
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
#ifndef PERFORMANCE_PARAM_MENU_H
#define PERFORMANCE_PARAM_MENU_H

#include <DMenu>

DWIDGET_USE_NAMESPACE
class PerformanceParamMenu : public DMenu
{
    Q_OBJECT
public:
    explicit PerformanceParamMenu(DWidget *parent = nullptr);

private:
    void initAction();

public slots:
};

#endif // PERFORMANCE_PARAM_MENU_H
