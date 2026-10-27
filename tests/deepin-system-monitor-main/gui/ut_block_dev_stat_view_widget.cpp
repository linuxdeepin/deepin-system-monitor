// Copyright (C) 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "block_dev_stat_view_widget.h"
#include "system/block_device.h"
#include "system/block_device_info_db.h"
#include "block_dev_item_widget.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QResizeEvent>

/***************************************STUB begin*********************************************/
static void stub_readDeviceInfo_noop()
{
}

static BlockDevice makeBlockDevice(const QByteArray &name)
{
    Stub stub;
    stub.set(ADDR(BlockDevice, readDeviceInfo), stub_readDeviceInfo_noop);
    BlockDevice dev;
    dev.setDeviceName(name);
    return dev;
}

// N 台"虚拟"设备列表（名称合法但不存在，速率保持 0）
static QList<BlockDevice> makeDeviceList(int count)
{
    QList<BlockDevice> listDB;
    for (int i = 0; i < count; ++i) {
        listDB.append(makeBlockDevice(QByteArray("ut-sm-disk-") + QByteArray::number(i)));
    }
    return listDB;
}

QList<BlockDevice> stub_deviceList_0()
{
    return QList<BlockDevice>();
}

QList<BlockDevice> stub_deviceList_1()
{
    return makeDeviceList(1);
}

QList<BlockDevice> stub_deviceList_2()
{
    return makeDeviceList(2);
}

QList<BlockDevice> stub_deviceList_5()
{
    return makeDeviceList(5);
}
/***************************************STUB end**********************************************/

class UT_BlockStatViewWidget : public ::testing::Test
{
public:
    UT_BlockStatViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 构造前对 deviceList 打桩，ctor 的 onUpdateData 即使用桩数据
        m_deviceListStub.set(ADDR(BlockDeviceInfoDB, deviceList), stub_deviceList_2);
        m_tester = new BlockStatViewWidget();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    BlockStatViewWidget *m_tester;
    Stub m_deviceListStub;
};

// 构造：onUpdateData 依据桩数据创建 2 个设备条目并完成布局
TEST_F(UT_BlockStatViewWidget, initTest)
{
    // Act: SetUp 已构造（deviceList 桩返回 2 台设备）
    // Assert
    ASSERT_NE(m_tester, nullptr);
    EXPECT_EQ(m_tester->m_listDevice.size(), 2);
    EXPECT_EQ(m_tester->m_listBlockItemWidget.size(), 2);
    EXPECT_EQ(m_tester->m_mapDeviceItemWidget.size(), 2);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->isActiveItem(), true);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[1]->isActiveItem(), false);
}

// resizeEvent 应触发 updateWidgetGeometry（无设备时安全返回）
TEST_F(UT_BlockStatViewWidget, test_resizeEvent_01)
{
    // Arrange
    m_deviceListStub.set(ADDR(BlockDeviceInfoDB, deviceList), stub_deviceList_0);
    BlockStatViewWidget widget;
    QSignalSpy signalSpy(&widget, &BlockStatViewWidget::changeInfo);
    static QResizeEvent re(QSize(10, 10), QSize(20, 20));

    // Act
    widget.resizeEvent(&re);

    // Assert: 无设备 -> updateWidgetGeometry 早退，无 changeInfo 发射
    EXPECT_EQ(signalSpy.count(), 0);
    EXPECT_TRUE(widget.m_listBlockItemWidget.isEmpty());}

// resizeEvent：有设备时触发布局，首个条目被激活并发射 changeInfo
TEST_F(UT_BlockStatViewWidget, test_resizeEvent_02)
{
    // Arrange
    m_deviceListStub.set(ADDR(BlockDeviceInfoDB, deviceList), stub_deviceList_1);
    BlockStatViewWidget widget;
    QSignalSpy signalSpy(&widget, &BlockStatViewWidget::changeInfo);
    static QResizeEvent re(QSize(300, 200), QSize(100, 100));

    // Act
    widget.resizeEvent(&re);

    // Assert: showItem1 对单设备条目不置激活态，但发射 changeInfo
    EXPECT_EQ(widget.m_listBlockItemWidget.size(), 1);
    EXPECT_EQ(widget.m_listBlockItemWidget[0]->isActiveItem(), false);
    EXPECT_EQ(signalSpy.count(), 1);
}

// updateWidgetGeometry：0 台设备走早退分支
TEST_F(UT_BlockStatViewWidget, test_updateWidgetGeometry_01)
{
    // Arrange
    m_tester->m_listDevice.clear();
    m_tester->m_mapDeviceItemWidget.clear();
    QSignalSpy signalSpy(m_tester, &BlockStatViewWidget::changeInfo);

    // Act
    m_tester->updateWidgetGeometry();

    // Assert: 早退分支不发射信号、不创建条目
    EXPECT_EQ(signalSpy.count(), 0);
    EXPECT_TRUE(m_tester->m_mapDeviceItemWidget.isEmpty());
}

// updateWidgetGeometry：1 台设备走 showItem1 分支（垂直模式 + 全幅几何）
TEST_F(UT_BlockStatViewWidget, test_updateWidgetGeometry_02)
{
    // Arrange
    m_tester->m_listDevice = makeDeviceList(1);
    m_tester->m_listBlockItemWidget.clear();
    m_tester->m_mapDeviceItemWidget.clear();
    m_tester->m_listBlockItemWidget.append(new BlockDevItemWidget(m_tester));
    QSignalSpy signalSpy(m_tester, &BlockStatViewWidget::changeInfo);

    // Act
    m_tester->updateWidgetGeometry();

    // Assert: showItem1 不置激活态，仅插入映射并发射 changeInfo
    EXPECT_EQ(m_tester->m_mapDeviceItemWidget.size(), 1);
    EXPECT_EQ(m_tester->m_mapDeviceItemWidget.contains("ut-sm-disk-0"), true);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->m_mode, BlockDevItemWidget::TitleStyle::TITLE_VERTICAL);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->isActiveItem(), false);
    EXPECT_EQ(signalSpy.count(), 1);
    if (signalSpy.count() == 1) {
        EXPECT_EQ(signalSpy.at(0).at(0).toString(), QString("ut-sm-disk-0"));
    }
}

// updateWidgetGeometry：2 台设备走 showItem2 分支（均垂直模式、无激活时激活第一个）
TEST_F(UT_BlockStatViewWidget, test_updateWidgetGeometry_03)
{
    // Arrange
    m_tester->m_listDevice = makeDeviceList(2);
    m_tester->m_listBlockItemWidget.clear();
    for (int i = 0; i < 2; ++i) {
        m_tester->m_listBlockItemWidget.append(new BlockDevItemWidget(m_tester));
    }
    QSignalSpy signalSpy(m_tester, &BlockStatViewWidget::changeInfo);

    // Act
    m_tester->updateWidgetGeometry();

    // Assert
    EXPECT_EQ(m_tester->m_mapDeviceItemWidget.size(), 2);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->m_mode, BlockDevItemWidget::TitleStyle::TITLE_VERTICAL);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->isActiveItem(), true);
    EXPECT_EQ(signalSpy.count(), 0);
}

// updateWidgetGeometry：>2 台设备走 showItemLg2 分支
TEST_F(UT_BlockStatViewWidget, test_updateWidgetGeometry_04)
{
    // Arrange
    m_tester->m_listDevice = makeDeviceList(3);
    m_tester->m_listBlockItemWidget.clear();
    for (int i = 0; i < 3; ++i) {
        m_tester->m_listBlockItemWidget.append(new BlockDevItemWidget(m_tester));
    }
    QSignalSpy signalSpy(m_tester, &BlockStatViewWidget::changeInfo);

    // Act
    m_tester->updateWidgetGeometry();

    // Assert: 3 台设备全部垂直模式，无激活时激活第一个并发射 changeInfo
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(m_tester->m_listBlockItemWidget[i]->m_mode,
                  BlockDevItemWidget::TitleStyle::TITLE_VERTICAL) << i;
    }
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->isActiveItem(), true);
    EXPECT_EQ(signalSpy.count(), 1);
    if (signalSpy.count() == 1) {
        EXPECT_EQ(signalSpy.at(0).at(0).toString(), QString("ut-sm-disk-0"));
    }
}

// onUpdateData：以桩数据重建设备列表，新增条目并刷新数据
TEST_F(UT_BlockStatViewWidget, test_onUpdateData_01)
{
    // Arrange
    m_deviceListStub.set(ADDR(BlockDeviceInfoDB, deviceList), stub_deviceList_5);

    // Act
    m_tester->onUpdateData();

    // Assert: 2 -> 5 台设备，条目补齐并更新数据
    EXPECT_EQ(m_tester->m_listDevice.size(), 5);
    EXPECT_EQ(m_tester->m_listBlockItemWidget.size(), 5);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->m_blokeDeviceInfo.deviceName(),
              QByteArray("ut-sm-disk-0"));
    EXPECT_EQ(m_tester->m_listBlockItemWidget[4]->m_blokeDeviceInfo.deviceName(),
              QByteArray("ut-sm-disk-4"));
}

// onUpdateData：设备数量减少时旧条目被隐藏但保留（源码仅增不删）
TEST_F(UT_BlockStatViewWidget, test_onUpdateData_02)
{
    // Arrange
    m_deviceListStub.set(ADDR(BlockDeviceInfoDB, deviceList), stub_deviceList_1);

    // Act
    m_tester->onUpdateData();

    // Assert: 设备列表收缩为 1；条目列表只增不删，多余条目转为隐藏
    EXPECT_EQ(m_tester->m_listDevice.size(), 1);
    EXPECT_EQ(m_tester->m_listBlockItemWidget.size(), 2);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->m_mode,
              BlockDevItemWidget::TitleStyle::TITLE_VERTICAL);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[1]->isHidden(), true);
}

// onUpdateData：空设备列表时映射被清空，条目保留（updateWidgetGeometry 早退）
TEST_F(UT_BlockStatViewWidget, test_onUpdateData_03)
{
    // Arrange
    m_deviceListStub.set(ADDR(BlockDeviceInfoDB, deviceList), stub_deviceList_0);

    // Act
    m_tester->onUpdateData();

    // Assert
    EXPECT_EQ(m_tester->m_listDevice.size(), 0);
    EXPECT_TRUE(m_tester->m_mapDeviceItemWidget.isEmpty());
    EXPECT_EQ(m_tester->m_listBlockItemWidget.size(), 2);
}

// fontChanged 应传播到所有条目
TEST_F(UT_BlockStatViewWidget, test_fontChanged_01)
{
    // Arrange
    QFont font;
    font.setBold(true);

    // Act
    m_tester->fontChanged(font);

    // Assert
    ASSERT_EQ(m_tester->m_listBlockItemWidget.size(), 2);
    for (int i = 0; i < m_tester->m_listBlockItemWidget.size(); ++i) {
        EXPECT_EQ(m_tester->m_listBlockItemWidget[i]->m_font, font) << i;
    }
}

// onSetItemStatus：激活匹配条目、取消其它条目，并发射 changeInfo
TEST_F(UT_BlockStatViewWidget, test_onSetItemStatus_01)
{
    // Arrange
    QSignalSpy signalSpy(m_tester, &BlockStatViewWidget::changeInfo);
    m_tester->m_listBlockItemWidget[0]->activeItemWidget(true);

    // Act
    m_tester->onSetItemStatus(QString("ut-sm-disk-1"));

    // Assert
    EXPECT_EQ(m_tester->m_mapDeviceItemWidget["ut-sm-disk-1"]->isActiveItem(), true);
    EXPECT_EQ(m_tester->m_mapDeviceItemWidget["ut-sm-disk-0"]->isActiveItem(), false);
    EXPECT_EQ(signalSpy.count(), 1);
    if (signalSpy.count() == 1) {
        EXPECT_EQ(signalSpy.at(0).at(0).toString(), QString("ut-sm-disk-1"));
    }
}

// 条目点击应联动 onSetItemStatus（clicked -> changeInfo）
TEST_F(UT_BlockStatViewWidget, test_onSetItemStatus_02)
{
    // Arrange
    QSignalSpy signalSpy(m_tester, &BlockStatViewWidget::changeInfo);

    // Act: 模拟条目点击
    emit m_tester->m_listBlockItemWidget[1]->clicked(QString("ut-sm-disk-1"));

    // Assert
    EXPECT_EQ(signalSpy.count(), 1);
    if (signalSpy.count() == 1) {
        EXPECT_EQ(signalSpy.at(0).at(0).toString(), QString("ut-sm-disk-1"));
    }
    EXPECT_EQ(m_tester->m_listBlockItemWidget[1]->isActiveItem(), true);
}

// showItemLg2：恰好 4 台（单页）不隐藏任何条目
TEST_F(UT_BlockStatViewWidget, test_showItemLg2_01)
{
    // Arrange
    m_tester->m_listDevice = makeDeviceList(4);
    m_tester->m_listBlockItemWidget.clear();
    for (int i = 0; i < 4; ++i) {
        m_tester->m_listBlockItemWidget.append(new BlockDevItemWidget(m_tester));
    }
    QSignalSpy signalSpy(m_tester, &BlockStatViewWidget::changeInfo);

    // Act
    m_tester->showItemLg2(4);

    // Assert: 单页，全部条目参与布局；无激活时激活第一个
    EXPECT_EQ(m_tester->m_mapDeviceItemWidget.size(), 4);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->isActiveItem(), true);
    EXPECT_EQ(signalSpy.count(), 1);
}

// showItemLg2：5 台（跨页）时中央部件宽度加倍
TEST_F(UT_BlockStatViewWidget, test_showItemLg2_02)
{
    // Arrange
    m_tester->m_listDevice = makeDeviceList(5);
    m_tester->m_listBlockItemWidget.clear();
    for (int i = 0; i < 5; ++i) {
        m_tester->m_listBlockItemWidget.append(new BlockDevItemWidget(m_tester));
    }
    m_tester->resize(400, 300);

    // Act
    m_tester->showItemLg2(5);

    // Assert: totalPage = (5-1)/4 = 1 -> 宽度 = 2 * 页宽
    EXPECT_EQ(m_tester->m_centralWidget->width(), m_tester->width() * 2);
    EXPECT_EQ(m_tester->m_centralWidget->height(), m_tester->height());
}

// showItemLg2：count 小于条目数时，多余条目隐藏并转水平模式
TEST_F(UT_BlockStatViewWidget, test_showItemLg2_03)
{
    // Arrange
    m_tester->m_listDevice = makeDeviceList(4);
    m_tester->m_listBlockItemWidget.clear();
    for (int i = 0; i < 4; ++i) {
        m_tester->m_listBlockItemWidget.append(new BlockDevItemWidget(m_tester));
    }
    m_tester->m_listBlockItemWidget[3]->activeItemWidget(true);

    // Act
    m_tester->showItemLg2(3);

    // Assert: index3 被隐藏且为水平模式；隐藏的激活条目触发首个条目重新激活
    EXPECT_EQ(m_tester->m_listBlockItemWidget[3]->isHidden(), true);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[3]->m_mode,
              BlockDevItemWidget::TitleStyle::TITLE_HORIZONTAL);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->isActiveItem(), true);
}

// showItem1：单设备布局，其余条目隐藏
TEST_F(UT_BlockStatViewWidget, test_showItem1_01)
{
    // Arrange
    m_tester->m_listDevice = makeDeviceList(1);
    m_tester->m_listBlockItemWidget.clear();
    m_tester->m_mapDeviceItemWidget.clear();
    for (int i = 0; i < 2; ++i) {
        m_tester->m_listBlockItemWidget.append(new BlockDevItemWidget(m_tester));
    }
    QSignalSpy signalSpy(m_tester, &BlockStatViewWidget::changeInfo);

    // Act
    m_tester->showItem1();

    // Assert
    EXPECT_EQ(m_tester->m_mapDeviceItemWidget.size(), 1);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->m_mode,
              BlockDevItemWidget::TitleStyle::TITLE_VERTICAL);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[1]->isHidden(), true);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[1]->m_mode,
              BlockDevItemWidget::TitleStyle::TITLE_HORIZONTAL);
    EXPECT_EQ(signalSpy.count(), 1);
    if (signalSpy.count() == 1) {
        EXPECT_EQ(signalSpy.at(0).at(0).toString(), QString("ut-sm-disk-0"));
    }
}

// showItem2：已有激活条目时不重复激活第一个
TEST_F(UT_BlockStatViewWidget, test_showItem2_01)
{
    // Arrange
    m_tester->m_listDevice = makeDeviceList(2);
    m_tester->m_listBlockItemWidget.clear();
    for (int i = 0; i < 3; ++i) {
        m_tester->m_listBlockItemWidget.append(new BlockDevItemWidget(m_tester));
    }
    m_tester->m_listBlockItemWidget[1]->activeItemWidget(true);
    QSignalSpy signalSpy(m_tester, &BlockStatViewWidget::changeInfo);

    // Act
    m_tester->showItem2();

    // Assert: item1 保持激活，信号不发射（无隐藏激活条目）
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->isActiveItem(), false);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[1]->isActiveItem(), true);
    EXPECT_EQ(signalSpy.count(), 0);
}

// showItem2：隐藏条目中有激活者时，转移激活到第一个并发射 changeInfo
TEST_F(UT_BlockStatViewWidget, test_showItem2_02)
{
    // Arrange
    m_tester->m_listDevice = makeDeviceList(2);
    m_tester->m_listBlockItemWidget.clear();
    for (int i = 0; i < 3; ++i) {
        m_tester->m_listBlockItemWidget.append(new BlockDevItemWidget(m_tester));
    }
    m_tester->m_listBlockItemWidget[2]->activeItemWidget(true);
    QSignalSpy signalSpy(m_tester, &BlockStatViewWidget::changeInfo);

    // Act
    m_tester->showItem2();

    // Assert
    EXPECT_EQ(m_tester->m_listBlockItemWidget[2]->isHidden(), true);
    EXPECT_EQ(m_tester->m_listBlockItemWidget[0]->isActiveItem(), true);
    EXPECT_EQ(signalSpy.count(), 1);
    if (signalSpy.count() == 1) {
        EXPECT_EQ(signalSpy.at(0).at(0).toString(), QString("ut-sm-disk-0"));
    }
}
