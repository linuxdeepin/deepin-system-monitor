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

#ifndef SYSTEM_SERVICE_PAGE_WIDGET_H
#define SYSTEM_SERVICE_PAGE_WIDGET_H

#include <DFrame>
#include <DWidget>

DWIDGET_USE_NAMESPACE

class MainWindow;
class SystemServiceTableView;

/**
 * @brief Service background frame widget
 */
class SystemServicePageWidget : public DFrame
{
public:
    /**
     * @brief Constrcutor
     * @param parent Parent object
     */
    explicit SystemServicePageWidget(DWidget *parent = nullptr);
    /**
    * @brief Destructor
    */
    ~SystemServicePageWidget();

protected:
    /**
     * @brief paintEvent Paint event handler
     * @param event Paint event
     */
    void paintEvent(QPaintEvent *event);

public Q_SLOTS:

Q_SIGNALS:

private:
    // Service table view instance
    SystemServiceTableView *m_svcTableView {};
};

#endif  // SYSTEM_SERVICE_PAGE_WIDGET_H
