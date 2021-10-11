/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      wangchao <wangchao@uniontech.com>
* Maintainer:  wangchao <wangchao@uniontech.com>
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

#include "common/error_context.h"
#include "process/priority_controller.h"
#include "application.h"

#include <QProcess>
#include <QFile>
#include <QObject>

#include "ut_priority_controller.h"

Ut_PriorityController::Ut_PriorityController()
{
}

TEST(UT_PriorityController_PriorityController, UT_PriorityController_PriorityController_001)
{
    pid_t testpid = 123;
    int testpriority = 1;

    PriorityController* ctrl = new PriorityController(testpid, testpriority, nullptr);

    EXPECT_NE(ctrl, nullptr);

    ctrl->deleteLater();
}
