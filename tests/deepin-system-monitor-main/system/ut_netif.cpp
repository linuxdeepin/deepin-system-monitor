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
#include "system/netif.h"
#include "system/wireless.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include <QDebug>
#include <QByteArray>
#include <QSharedDataPointer>

using namespace core::system;

/***************************************STUB begin*********************************************/

bool is_wireless_true()
{
    return true;
}

bool is_wireless_false()
{
    return false;
}

/***************************************STUB end**********************************************/

class UT_NetifInfo: public ::testing::Test
{
public:
    UT_NetifInfo() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new NetifInfo();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetifInfo *m_tester;
};

TEST_F(UT_NetifInfo, initTest)
{
}

TEST_F(UT_NetifInfo, test_copy_1)
{
    NetifInfo copy(*m_tester);
}

TEST_F(UT_NetifInfo, test_copy_02)
{
    NetifInfo *copy = m_tester;
}

TEST_F(UT_NetifInfo, test_set_recv_bps)
{
    m_tester->set_recv_bps(10);
    qInfo()<<"m_tester->recv_bps():"<<m_tester->recv_bps();
    EXPECT_EQ(m_tester->recv_bps(), 10);
}

TEST_F(UT_NetifInfo, test_set_sent_bps)
{
    m_tester->set_sent_bps(10);
    EXPECT_EQ(m_tester->sent_bps(), 10);
}

TEST_F(UT_NetifInfo, test_updateWirelessInfo_01)
{
    Stub stub;
    stub.set(ADDR(wireless, is_wireless), is_wireless_true);
    m_tester->updateWirelessInfo();
    EXPECT_TRUE(m_tester->isWireless());
}
TEST_F(UT_NetifInfo, test_updateWirelessInfo_02)
{
    Stub stub;
    stub.set(ADDR(wireless, is_wireless), is_wireless_false);
    m_tester->updateWirelessInfo();
    EXPECT_FALSE(m_tester->isWireless());
}


TEST_F(UT_NetifInfo, test_updateBrandInfo)
{
    m_tester->updateBrandInfo();
}

TEST_F(UT_NetifInfo, test_index)
{
    EXPECT_NE(m_tester->index(), -1);
}

TEST_F(UT_NetifInfo, test_ifname)
{
    m_tester->ifname();
}

TEST_F(UT_NetifInfo, test_essid_01)
{
    wireless wireless1(m_tester->d->ifname);
    m_tester->d->isWireless = true;
    m_tester->d->iw_info->essid = wireless1.essid();
    m_tester->d->iw_info->qual.qual = wireless1.link_quality();
    m_tester->d->iw_info->qual.level = wireless1.signal_levle();
    m_tester->d->iw_info->qual.noise = wireless1.noise_level();
    m_tester->d->iw_info;
    m_tester->essid();
}

TEST_F(UT_NetifInfo, test_essid_02)
{
    m_tester->d->iw_info = nullptr;
    m_tester->essid();
}

TEST_F(UT_NetifInfo, test_linkQuality_01)
{
    wireless wireless1(m_tester->d->ifname);
    m_tester->d->isWireless = true;
    m_tester->d->iw_info->essid = wireless1.essid();
    m_tester->d->iw_info->qual.qual = wireless1.link_quality();
    m_tester->d->iw_info->qual.level = wireless1.signal_levle();
    m_tester->d->iw_info->qual.noise = wireless1.noise_level();
    m_tester->d->iw_info;
    m_tester->linkQuality();
}

TEST_F(UT_NetifInfo, test_linkQuality_02)
{
    m_tester->d->iw_info = nullptr;
    m_tester->linkQuality();
}

TEST_F(UT_NetifInfo, test_signalLevel_01)
{
    wireless wireless1(m_tester->d->ifname);
    m_tester->d->isWireless = true;
    m_tester->d->iw_info->essid = wireless1.essid();
    m_tester->d->iw_info->qual.qual = wireless1.link_quality();
    m_tester->d->iw_info->qual.level = wireless1.signal_levle();
    m_tester->d->iw_info->qual.noise = wireless1.noise_level();
    m_tester->d->iw_info;
    m_tester->signalLevel();
}

TEST_F(UT_NetifInfo, test_signalLevel_02)
{
    m_tester->d->iw_info = nullptr;
    m_tester->signalLevel();
}

TEST_F(UT_NetifInfo, test_noiseLevel_01)
{
    wireless wireless1(m_tester->d->ifname);
    m_tester->d->isWireless = true;
    m_tester->d->iw_info->essid = wireless1.essid();
    m_tester->d->iw_info->qual.qual = wireless1.link_quality();
    m_tester->d->iw_info->qual.level = wireless1.signal_levle();
    m_tester->d->iw_info->qual.noise = wireless1.noise_level();
    m_tester->d->iw_info;
    m_tester->noiseLevel();
}

TEST_F(UT_NetifInfo, test_noiseLevel_02)
{
    m_tester->d->iw_info = nullptr;
    m_tester->noiseLevel();
}
