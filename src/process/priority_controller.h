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

#ifndef PRIORITY_CONTROLLER_H
#define PRIORITY_CONTROLLER_H

#include "common/error_context.h"

#include <QThread>

class PriorityController : public QThread
{
    Q_OBJECT

public:
    explicit PriorityController(pid_t pid, int priority, QObject *parent = nullptr);

Q_SIGNALS:
    void resultReady(int code);

protected:
    void run() override;

private:
    pid_t m_pid;
    int m_priority {0};
};

#endif  // PRIORITY_CONTROLLER_H
