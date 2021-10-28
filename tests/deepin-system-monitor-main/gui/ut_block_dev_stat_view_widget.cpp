#include "ut_block_dev_stat_view_widget.h"
#include "block_dev_stat_view_widget.h"
#include "block_dev_item_widget.h"

#include <QSignalSpy>

Ut_BlockStatViewWidget::Ut_BlockStatViewWidget()
{

}

void BlockStatViewWidget_showItem()
{
    BlockStatViewWidget *pBlockStatViewWidget = new BlockStatViewWidget;
//    pBlockStatViewWidget->m_listBlockItemWidget.append(new BlockDevItemWidget);

    pBlockStatViewWidget->deleteLater();
}

void BlockStatViewWidget_showItemLg2()
{
    BlockStatViewWidget *pBlockStatViewWidget = new BlockStatViewWidget;
//    pBlockStatViewWidget->m_listBlockItemWidget.append(new BlockDevItemWidget);

    pBlockStatViewWidget->deleteLater();
}

TEST(Ut_BlockStatViewWidget, Ut_BlockStatViewWidget_BlockStatViewWidget_001)
{
    BlockStatViewWidget *pBlockStatViewWidget = new BlockStatViewWidget;

    EXPECT_TRUE(pBlockStatViewWidget->m_centralWidget);

    EXPECT_EQ(pBlockStatViewWidget->widget(), pBlockStatViewWidget->m_centralWidget);

    EXPECT_EQ(pBlockStatViewWidget->frameShape(), QFrame::NoFrame);

    pBlockStatViewWidget->deleteLater();
}


TEST(Ut_BlockStatViewWidget, Ut_BlockStatViewWidget_resizeEvent_001)
{
    BlockStatViewWidget *pBlockStatViewWidget = new BlockStatViewWidget;

    pBlockStatViewWidget->resize(20, 20);

    EXPECT_EQ(pBlockStatViewWidget->width(), 20);
    EXPECT_EQ(pBlockStatViewWidget->height(), 20);

    pBlockStatViewWidget->deleteLater();
}

TEST(Ut_BlockStatViewWidget, Ut_BlockStatViewWidget_fontChanged_001)
{
    BlockStatViewWidget *pBlockStatViewWidget = new BlockStatViewWidget;

    QFont font("Times", 20, 10, false);
    pBlockStatViewWidget->fontChanged(font);
    for (int i = 0; i < pBlockStatViewWidget->m_listBlockItemWidget.size(); ++i) {
        EXPECT_EQ(pBlockStatViewWidget->m_listBlockItemWidget[i]->m_font.family(), font.family());
        EXPECT_EQ(pBlockStatViewWidget->m_listBlockItemWidget[i]->m_font.pointSize(), font.pointSize());
        EXPECT_EQ(pBlockStatViewWidget->m_listBlockItemWidget[i]->m_font.italic(), font.italic());
        EXPECT_EQ(pBlockStatViewWidget->m_listBlockItemWidget[i]->m_font.bold(), font.bold());
    }

    pBlockStatViewWidget->deleteLater();
}

TEST(Ut_BlockStatViewWidget, Ut_BlockStatViewWidget_updateWidgetGeometry_001)
{
    BlockStatViewWidget *pBlockStatViewWidget = new BlockStatViewWidget;

    pBlockStatViewWidget->updateWidgetGeometry();

    EXPECT_LE(pBlockStatViewWidget->m_listDevice.size(), 0);

    pBlockStatViewWidget->deleteLater();
}

TEST(Ut_BlockStatViewWidget, Ut_BlockStatViewWidget_onSetItemStatus_001)
{
    BlockStatViewWidget *pBlockStatViewWidget = new BlockStatViewWidget;

    QString devicename = "dev1";
    pBlockStatViewWidget->onSetItemStatus(devicename);

    QSignalSpy signalSpy(pBlockStatViewWidget, &BlockStatViewWidget::changeInfo);
    EXPECT_TRUE(signalSpy.count() == 0);

    emit pBlockStatViewWidget->changeInfo(devicename);
    signalSpy.wait(50);
    EXPECT_TRUE(signalSpy.count() == 1);

    pBlockStatViewWidget->deleteLater();
}

TEST(Ut_BlockStatViewWidget, Ut_BlockStatViewWidget_showItem1_001)
{
    Stub stub;
    BlockStatViewWidget *pBlockStatViewWidget = new BlockStatViewWidget;
    stub.set(ADDR(BlockStatViewWidget, showItem1), BlockStatViewWidget_showItem);

//    BlockDevItemWidget *item2 = new BlockDevItemWidget(pBlockStatViewWidget->m_centralWidget);
//    pBlockStatViewWidget->m_listBlockItemWidget.append(item2);
//    BlockDevItemWidget *item3 = new BlockDevItemWidget(pBlockStatViewWidget->m_centralWidget);
//    pBlockStatViewWidget->m_listBlockItemWidget.append(item3);
    pBlockStatViewWidget->showItem1();
//    EXPECT_TRUE(pBlockStatViewWidget->m_listBlockItemWidget.at(0));
//    EXPECT_TRUE(pBlockStatViewWidget->m_listBlockItemWidget.at(1));

    QString devicename = "dev1";
    QSignalSpy signalSpy(pBlockStatViewWidget, &BlockStatViewWidget::changeInfo);
    EXPECT_TRUE(signalSpy.count() == 0);

    emit pBlockStatViewWidget->changeInfo(devicename);
    signalSpy.wait(50);
    EXPECT_TRUE(signalSpy.count() == 1);

    pBlockStatViewWidget->deleteLater();
}

TEST(Ut_BlockStatViewWidget, Ut_BlockStatViewWidget_showItem2_001)
{
    Stub stub;
    BlockStatViewWidget *pBlockStatViewWidget = new BlockStatViewWidget;
    stub.set(ADDR(BlockStatViewWidget, showItem2), BlockStatViewWidget_showItem);
    pBlockStatViewWidget->showItem2();

    QString devicename = "dev1";
    QSignalSpy signalSpy(pBlockStatViewWidget, &BlockStatViewWidget::changeInfo);
    EXPECT_TRUE(signalSpy.count() == 0);

    emit pBlockStatViewWidget->changeInfo(devicename);
    signalSpy.wait(50);
    EXPECT_TRUE(signalSpy.count() == 1);

    pBlockStatViewWidget->deleteLater();
}

TEST(Ut_BlockStatViewWidget, Ut_BlockStatViewWidget_showItemLg2_001)
{
    Stub stub;
    BlockStatViewWidget *pBlockStatViewWidget = new BlockStatViewWidget;
    stub.set(ADDR(BlockStatViewWidget, showItemLg2), BlockStatViewWidget_showItemLg2);


    pBlockStatViewWidget->showItemLg2(10);

    QString devicename = "dev1";
    QSignalSpy signalSpy(pBlockStatViewWidget, &BlockStatViewWidget::changeInfo);
    EXPECT_TRUE(signalSpy.count() == 0);

    emit pBlockStatViewWidget->changeInfo(devicename);
    signalSpy.wait(50);
    EXPECT_TRUE(signalSpy.count() == 1);

    pBlockStatViewWidget->deleteLater();
}

TEST(Ut_BlockStatViewWidget, Ut_BlockStatViewWidget_onUpdateData_001)
{
    BlockStatViewWidget *pBlockStatViewWidget = new BlockStatViewWidget;

    pBlockStatViewWidget->onUpdateData();

    EXPECT_EQ(pBlockStatViewWidget->m_mapDeviceItemWidget.size(), 0);

    pBlockStatViewWidget->deleteLater();
}













