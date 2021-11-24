/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      baohaifeng <baohaifeng@uniontech.com>
* Maintainer:  baohaifeng <baohaifeng@uniontech.com>
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

//self
#include "compact_network_monitor.h"
#include "model/cpu_info_model.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <DApplication>
#include <QPainterPath>
#include <QMouseEvent>
#include <DApplication>
#include <DApplicationHelper>


/***************************************STUB begin*********************************************/
/***************************************STUB end**********************************************/


class UT_CompactNetworkMonitor: public ::testing::Test
{
public:
    UT_CompactNetworkMonitor() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new CompactNetworkMonitor();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CompactNetworkMonitor *m_tester;
};

TEST_F(UT_CompactNetworkMonitor, initTest)
{
}

TEST_F(UT_CompactNetworkMonitor, test_getPainterPathByData)
{
    m_tester->uploadSpeeds->append(0.1);
    m_tester->uploadSpeeds->append(0.2);
    m_tester->uploadSpeeds->append(0.3);
    m_tester->uploadSpeeds->append(0.4);
    m_tester->uploadSpeeds->append(0.5);
    double maxHeight =20;
    QPainterPath tmpUploadpath;
    m_tester->getPainterPathByData(m_tester->uploadSpeeds, tmpUploadpath, maxHeight);
}
TEST_F(UT_CompactNetworkMonitor, test_updateStatus_01)
{
    m_tester->downloadSpeeds->append(0.1);
    m_tester->downloadSpeeds->append(0.2);
    m_tester->downloadSpeeds->append(0.3);
    m_tester->downloadSpeeds->append(0.4);
    m_tester->downloadSpeeds->append(0.5);
    m_tester->updateStatus();
}

TEST_F(UT_CompactNetworkMonitor, test_paintEvent)
{
    EXPECT_FALSE(m_tester->grab().isNull());
}

TEST_F(UT_CompactNetworkMonitor, test_changeTheme)
{
    m_tester->changeTheme(DGuiApplicationHelper::ColorType::LightType);
}

TEST_F(UT_CompactNetworkMonitor, test_changeFont)
{
    QFont font;
    font.setWeight(QFont::Medium);
    m_tester->changeFont(font);
}

TEST_F(UT_CompactNetworkMonitor, test_mouseReleaseEvent)
{
    QMouseEvent *event = new QMouseEvent(QEvent::Type::Move, QPointF(0, 0), Qt::MouseButton::LeftButton, Qt::MouseButton::LeftButton, Qt::KeyboardModifier::ShiftModifier);
    m_tester->mouseReleaseEvent(event);
    delete event;
}

TEST_F(UT_CompactNetworkMonitor, test_mouseMoveEvent)
{
    QMouseEvent *event = new QMouseEvent(QEvent::Type::Move, QPointF(0, 0), Qt::MouseButton::LeftButton, Qt::MouseButton::LeftButton, Qt::KeyboardModifier::ShiftModifier);
    m_tester->mouseMoveEvent(event);
    delete event;
}

