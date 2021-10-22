#include "stub.h"

#include <QObject>
#include <QTest>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "compact_disk_monitor.h"
static int stub_flag;

TEST(UT_CompactDiskMonitor_CompactDiskMonitor,UT_CompactDiskMonitor_CompactDiskMonitor_001)
{
    QWidget *pQWidget = new QWidget();
    CompactDiskMonitor *pCompactDiskMonitor = new CompactDiskMonitor();

    pQWidget->deleteLater();
    pCompactDiskMonitor->deleteLater();
}

int stub1_updateStatus(){
    stub_flag = 1;
    return 0;
}

TEST(UT_CompactDiskMonitor_updateStatus,UT_CompactDiskMonitor_updateStatus_001)
{
    QWidget *pQWidget = new QWidget();
    CompactDiskMonitor *pCompactDiskMonitor = new CompactDiskMonitor();

    Stub a1;
    a1.set(ADDR(CompactDiskMonitor,updateStatus),stub1_updateStatus());
    EXPECT_EQ(stub_flag,1);

    pQWidget->deleteLater();
    pCompactDiskMonitor->deleteLater();
}

int stub2_getPainterPathByData(){
    stub_flag = 2;
    return 0;
}

TEST(UT_CompactDiskMonitor_getPainterPathByData,UT_CompactDiskMonitor_getPainterPathByData_001)
{
    QWidget *pQWidget = new QWidget();
    CompactDiskMonitor *pCompactDiskMonitor = new CompactDiskMonitor();

    Stub b2;
    b2.set(ADDR(CompactDiskMonitor,getPainterPathByData),stub2_getPainterPathByData());
    EXPECT_EQ(stub_flag,2);

    pQWidget->deleteLater();
    pCompactDiskMonitor->deleteLater();
}

int stub3_paintEvent(){
    stub_flag = 3;
    return 0;
}

TEST(UT_CompactDiskMonitor_paintEvent,UT_CompactDiskMonitor_paintEvent_001)
{
    QWidget *pQWidget = new QWidget();
    CompactDiskMonitor *pCompactDiskMonitor = new CompactDiskMonitor();

    Stub c3;
    c3.set(ADDR(CompactDiskMonitor,setFixedHeight),stub3_paintEvent());
    EXPECT_EQ(stub_flag,3);

    pQWidget->deleteLater();
    pCompactDiskMonitor->deleteLater();
}

int stub4_changeFont(){
    stub_flag = 4;
    return 0;
}

TEST(UT_CompactDiskMonitor_changeFont,UT_CompactDiskMonitor_changeFont_001)
{
    QWidget *pQWidget = new QWidget();
    CompactDiskMonitor *pCompactDiskMonitor = new CompactDiskMonitor();

    Stub d4;
    d4.set(ADDR(QWidget,font),stub4_changeFont());
    EXPECT_EQ(stub_flag,4);

    pQWidget->deleteLater();
    pCompactDiskMonitor->deleteLater();
}

int stub6_mouseReleaseEvent(){
    stub_flag = 5;
    return 0;
}

TEST(UT_CompactDiskMonitor_mouseReleaseEvent,UT_CompactDiskMonitor_mouseReleaseEvent_001)
{
    QWidget *pQWidget = new QWidget();
    CompactDiskMonitor *pCompactDiskMonitor = new CompactDiskMonitor();

    Stub f5;
    f5.set(ADDR(CompactDiskMonitor,clicked),stub6_mouseReleaseEvent());
    EXPECT_EQ(stub_flag,5);

    pQWidget->deleteLater();
    pCompactDiskMonitor->deleteLater();
}
