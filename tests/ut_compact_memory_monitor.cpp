#include "compact_memory_monitor.h"
#include "stub.h"

#include <QObject>
#include <QTest>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
static int stub_flag;

//实现内存对齐
class Ut_compact_memory_monitor : public QObject, public::testing::Test
{
public:
    Ut_compact_memory_monitor() {}
};

//函数打桩
int stub1_CpactMemoryMonitor(){
    stub_flag = 1;
    return 0;
}

TEST(UT_CompactMemoryMonitor_CompactMemoryMonitor,UT_CompactMemoryMonitor_CompactMemoryMonitor_001)
{
    QWidget *pQWidget = new QWidget();

    Stub a1;
    a1.set(ADDR(CompactMemoryMonitor,animationFinshed),stub1_CpactMemoryMonitor());
    EXPECT_EQ(stub_flag,1);

    pQWidget->deleteLater();
}

TEST(UT_CompactMemoryMonitor_progress,UT_CompactMemoryMonitor_progress_001)
{
    QWidget *pQWidget = new QWidget();

    pQWidget->deleteLater();
}

int stub2_SetProgress(){
    stub_flag = 2;
    return 0;
}

TEST(UT_CompactMemoryMonitor_SetProgress,UT_CompactMemoryMonitor_SetProgress_001)
{
    QWidget *pQWidget = new QWidget();

    pQWidget->deleteLater();
}
