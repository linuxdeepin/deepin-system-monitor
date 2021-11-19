/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan  <yukuan@uniontech.com>
*
* Maintainer: yukuan  <yukuan@uniontech.com>
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

//Self
#include "ui_common.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>




/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

TEST(UT_UICOMMON, formatColor)
{
    QColor color(10, 10, 10);
    QString colorStr = formatColor(QColor(10, 10, 10), 10);

    EXPECT_EQ(colorStr, QString("A:%1 R:%2 G:%3 B:%4")
              .arg(color.alpha())
              .arg(color.red())
              .arg(color.green())
              .arg(color.blue()));
}

TEST(UT_UICOMMON, iconPathFromQrc)
{
    QString iconPath = iconPathFromQrc(QString("11"));

    EXPECT_EQ(iconPath, QString(":/icons/deepin/builtin/11"));
}
