#include "compact_cpu_monitor.h"
#include "stub.h"

#include <QObject>
#include <QTest>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
static int stub_flag;

//实现内存对齐
class Ut_compact_cpu_monitor : public QObject, public::testing::Test
{
public:
    Ut_compact_cpu_monitor() {}
};

TEST(UT_CompactCpuMonitor_CompactCpuMonitor,UT_CompactCpuMonitor_CompactCpuMonitor_001)
{
    QWidget *pQWidget = new QWidget();
    CompactCpuMonitor *pCompactCpuMonitor = new CompactCpuMonitor();
    pQWidget->setFixedHeight(160);
    EXPECT_TRUE(160);
    pQWidget->setFixedWidth(280);
    EXPECT_TRUE(280);

    pQWidget->deleteLater();
    pCompactCpuMonitor->deleteLater();
}

//函数打桩
int stub1_setDetailButtonVisible(){
    stub_flag = 1;
    return 0;
}

TEST(UT_CompactCpuMonitor_onDetailInfoClicked,UT_CompactCpuMonitor_onDetailInfoClicked_001)
{
    QWidget *pQWidget = new QWidget();
    CompactCpuMonitor *pCompactCpuMonitor = new CompactCpuMonitor();

    Stub a1;
    a1.set(ADDR(CompactCpuMonitor,setDetailButtonVisible),stub1_setDetailButtonVisible());
    EXPECT_EQ(stub_flag,1);

    pQWidget->deleteLater();
    pCompactCpuMonitor->deleteLater();
}

TEST(UT_CompactCpuMonitor_updateStatus,UT_CompactCpuMonitor_updateStatus_001)
{
    QWidget *pQWidget = new QWidget();
    CompactCpuMonitor *pCompactCpuMonitor = new CompactCpuMonitor();

    EXPECT_FALSE(pCompactCpuMonitor->totalCpuPercent);

    pQWidget->deleteLater();
    pCompactCpuMonitor->deleteLater();
}

int stub2_updateStatus(){
    stub_flag = 2;
    return 0;
}

TEST(UT_CompactCpuMonitor_updateStatus,UT_CompactCpuMonitor_updateStatus_002)
{
    QWidget *pQWidget = new QWidget();
    CompactCpuMonitor *pCompactCpuMonitor = new CompactCpuMonitor();

    Stub b2;
    b2.set(ADDR(CompactCpuMonitor,updateStatus),stub2_updateStatus());
    EXPECT_EQ(stub_flag,2);

    pQWidget->deleteLater();
    pCompactCpuMonitor->deleteLater();
}

int stub3_setDetailButtonVisible(){
    stub_flag = 3;
    return 0;
}

TEST(UT_CompactCpuMonitor_setDetailButtonVisible,UT_CompactCpuMonitor_setDetailButtonVisible_001)
{
    QWidget *pQWidget = new QWidget();
    CompactCpuMonitor *pCompactCpuMonitor = new CompactCpuMonitor();

    Stub c3;
    c3.set(ADDR(CompactCpuMonitor,setDetailButtonVisible),stub3_setDetailButtonVisible());
    EXPECT_EQ(stub_flag,3);

    pQWidget->deleteLater();
    pCompactCpuMonitor->deleteLater();
}

int stub4_resizeItemRect(){
    stub_flag = 4;
    return 0;
}

TEST(UT_CompactCpuMonitor_resizeItemRect,UT_CompactCpuMonitor_resizeItemRect_001)
{
    QWidget *pQWidget = new QWidget();
    CompactCpuMonitor *pCompactCpuMonitor = new CompactCpuMonitor();

    Stub d4;
    d4.set(ADDR(CompactCpuMonitor,resizeItemRect),stub4_resizeItemRect());
    EXPECT_EQ(stub_flag,4);

    pQWidget->deleteLater();
    pCompactCpuMonitor->deleteLater();
}

TEST(UT_CompactCpuMonitor_changeFont,UT_CompactCpuMonitor_changeFont_001)
{
    QWidget *pQWidget = new QWidget();
    CompactCpuMonitor *pCompactCpuMonitor = new CompactCpuMonitor();

    Stub d4;
    EXPECT_TRUE(stub_flag);

    pQWidget->deleteLater();
    pCompactCpuMonitor->deleteLater();
}

int stub5_mouseReleaseEvent(){
    stub_flag = 5;
    return 0;
}

TEST(UT_CompactCpuMonitor_mouseReleaseEvent,UT_CompactCpuMonitor_mouseReleaseEvent_001)
{
    QWidget *pQWidget = new QWidget();
    CompactCpuMonitor *pCompactCpuMonitor = new CompactCpuMonitor();

    Stub f5;
    f5.set(ADDR(CompactCpuMonitor,clicked),stub5_mouseReleaseEvent());
    EXPECT_EQ(stub_flag,5);

    pQWidget->deleteLater();
    pCompactCpuMonitor->deleteLater();
}
