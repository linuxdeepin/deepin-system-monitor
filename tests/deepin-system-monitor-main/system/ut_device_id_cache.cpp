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
#include "system/device_id_cache.h"
#include "system/udev.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::system;

class UT_DeviceIDCache: public ::testing::Test
{
public:
    UT_DeviceIDCache() : m_tester(nullptr),m_udev(nullptr) {}

public:
    virtual void SetUp()
    {
        m_udev = new UDev;
        m_tester = new DeviceIDCache(m_udev);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        if (m_udev) {
            delete m_udev;
            m_udev = nullptr;
        }
    }

protected:
    DeviceIDCache *m_tester;
    UDev *m_udev;
};

TEST_F(UT_DeviceIDCache, initTest)
{
}
