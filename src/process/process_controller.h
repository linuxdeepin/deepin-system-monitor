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

#ifndef PROCESSCONTROLLER_H
#define PROCESSCONTROLLER_H

#include "common/error_context.h"

#include <QThread>

class ProcessController : public QThread
{
    Q_OBJECT

public:
    explicit ProcessController(pid_t pid, int signal, QObject *parent = nullptr);

Q_SIGNALS:
    void resultReady(int code);

protected:
    void run() override;

private:
    pid_t m_pid;
    int m_signal {0};
};

#endif  // PROCESSCONTROLLER_H
