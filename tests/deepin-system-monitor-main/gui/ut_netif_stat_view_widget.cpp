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
#include "netif_stat_view_widget.h"
#include "system/device_db.h"
#include "system/netif.h"
#include "system/netif_info_db.h"
#include "netif_item_view_widget.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>

/***************************************STUB begin*********************************************/

const QMap<QByteArray, NetifInfoPtr> stub_updateWidgetGeometry_infoDB_1()
{
    QMap<QByteArray, NetifInfoPtr> map;
    NetifInfoPtr ptr;
    map.insert("1", ptr);
    return map;
}


const QMap<QByteArray, NetifInfoPtr> stub_updateWidgetGeometry_infoDB_2()
{
    QMap<QByteArray, NetifInfoPtr> map;
    NetifInfoPtr ptr;
    map.insert("1", ptr);
    map.insert("2", ptr);
    return map;
}

const QMap<QByteArray, NetifInfoPtr> stub_updateWidgetGeometry_infoDB_3()
{
    QMap<QByteArray, NetifInfoPtr> map;
    NetifInfoPtr ptr;
    map.insert("1", ptr);
    map.insert("2", ptr);
    map.insert("3", ptr);
    return map;
}

const QMap<QByteArray, NetifInfoPtr> stub_updateWidgetGeometry_infoDB_4()
{
    QMap<QByteArray, NetifInfoPtr> map;
    NetifInfoPtr ptr;
    map.insert("1", ptr);
    map.insert("2", ptr);
    map.insert("3", ptr);
    map.insert("4", ptr);
    return map;
}

/***************************************STUB end**********************************************/

class UT_NetifStatViewWidget : public ::testing::Test
{
public:
    UT_NetifStatViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget parent;
        m_tester = new NetifStatViewWidget(&parent);
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    NetifStatViewWidget *m_tester;
};

TEST_F(UT_NetifStatViewWidget, initTest)
{

}

TEST_F(UT_NetifStatViewWidget, test_updateWidgetGeometry_01)
{
    Stub stub;
    stub.set(ADDR(core::system::NetifInfoDB, infoDB), stub_updateWidgetGeometry_infoDB_1);
    m_tester->updateWidgetGeometry();
}

TEST_F(UT_NetifStatViewWidget, test_updateWidgetGeometry_02)
{
    Stub stub;
    stub.set(ADDR(core::system::NetifInfoDB, infoDB), stub_updateWidgetGeometry_infoDB_2);
    m_tester->updateWidgetGeometry();
}

TEST_F(UT_NetifStatViewWidget, test_updateWidgetGeometry_03)
{
    Stub stub;
    stub.set(ADDR(core::system::NetifInfoDB, infoDB), stub_updateWidgetGeometry_infoDB_3);
    m_tester->updateWidgetGeometry();
}

TEST_F(UT_NetifStatViewWidget, test_showItemOnlyeOne_01)
{
    Stub stub;
    stub.set(ADDR(core::system::NetifInfoDB, infoDB), stub_updateWidgetGeometry_infoDB_3);

    NetifItemViewWidget wid;
    m_tester->m_mapItemView.insert("1", &wid);
    m_tester->showItemOnlyeOne();
}


TEST_F(UT_NetifStatViewWidget, test_showItemOnlyeOne_02)
{
    Stub stub;
    stub.set(ADDR(core::system::NetifInfoDB, infoDB), stub_updateWidgetGeometry_infoDB_3);

    NetifItemViewWidget wid;
    m_tester->m_mapItemView.insert("4", &wid);
    m_tester->showItemOnlyeOne();
}


TEST_F(UT_NetifStatViewWidget, test_showItemDouble_01)
{
    Stub stub;
    stub.set(ADDR(core::system::NetifInfoDB, infoDB), stub_updateWidgetGeometry_infoDB_3);

    static QWidget tmpWidget;
    NetifItemViewWidget wid(&tmpWidget);
    m_tester->m_mapItemView.insert("1", &wid);
    m_tester->showItemDouble();
}

TEST_F(UT_NetifStatViewWidget, test_showItemDouble_02)
{
    m_tester->m_currentMac = "1";
    Stub stub;
    stub.set(ADDR(core::system::NetifInfoDB, infoDB), stub_updateWidgetGeometry_infoDB_3);

    static QWidget tmpWidget;
    NetifItemViewWidget wid(&tmpWidget);
    m_tester->m_mapItemView.insert("1", &wid);
    m_tester->showItemDouble();
}


TEST_F(UT_NetifStatViewWidget, test_showItemDouble_03)
{
    Stub stub;
    stub.set(ADDR(core::system::NetifInfoDB, infoDB), stub_updateWidgetGeometry_infoDB_3);

    static QWidget tmpWidget;
    NetifItemViewWidget wid(&tmpWidget);
    m_tester->m_mapItemView.insert("4", &wid);
    m_tester->showItemDouble();
}

TEST_F(UT_NetifStatViewWidget, test_showItemLgDouble_01)
{
    Stub stub;
    stub.set(ADDR(core::system::NetifInfoDB, infoDB), stub_updateWidgetGeometry_infoDB_4);

    static QWidget tmpWidget;
    NetifItemViewWidget wid(&tmpWidget);
    m_tester->m_mapItemView.insert("1", &wid);
    m_tester->m_mapItemView.insert("2", &wid);
    m_tester->m_mapItemView.insert("3", &wid);
    m_tester->m_mapItemView.insert("4", &wid);
    m_tester->showItemLgDouble();
}

TEST_F(UT_NetifStatViewWidget, test_showItemLgDouble_02)
{
    Stub stub;
    stub.set(ADDR(core::system::NetifInfoDB, infoDB), stub_updateWidgetGeometry_infoDB_4);

    static QWidget tmpWidget;
    NetifItemViewWidget wid(&tmpWidget);
    m_tester->m_mapItemView.insert("5", &wid);
    m_tester->showItemLgDouble();
}

TEST_F(UT_NetifStatViewWidget, test_showItemLgDouble_03)
{
    m_tester->m_currentMac = "1";
    Stub stub;
    stub.set(ADDR(core::system::NetifInfoDB, infoDB), stub_updateWidgetGeometry_infoDB_4);

    static QWidget tmpWidget;
    NetifItemViewWidget wid(&tmpWidget);
    m_tester->m_mapItemView.insert("1", &wid);
    m_tester->m_mapItemView.insert("2", &wid);
    m_tester->m_mapItemView.insert("3", &wid);
    m_tester->m_mapItemView.insert("4", &wid);
    m_tester->showItemLgDouble();
}


















