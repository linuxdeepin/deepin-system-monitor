#include "ut_netif_stat_view_widget.h"
#include "netif_stat_view_widget.h"

static int stub_flag;
Ut_NetifStatViewWidget::Ut_NetifStatViewWidget()
{

}
TEST(Ut_Ut_NetifStatViewWidget_NetifStatViewWidget,Ut_NetifStatViewWidget_NetifStatViewWidget_001)
{
    QWidget *pQWidget = new QWidget;
    NetifStatViewWidget *pNetifStatViewWidget = new NetifStatViewWidget(pQWidget);

    ASSERT_NE(pNetifStatViewWidget->m_centralWidget,nullptr);
    EXPECT_EQ(pNetifStatViewWidget->frameShape(),QFrame::NoFrame);

    pQWidget->deleteLater();
    pNetifStatViewWidget->deleteLater();
}

int stub1_updateWidgetGeometry(){
    stub_flag = 1;
    return 0;
}

TEST(Ut_Ut_NetifStatViewWidget_resizeEvent,Ut_NetifStatViewWidget_resizeEvent_001)
{
    QWidget *pQWidget = new QWidget;
    NetifStatViewWidget *pNetifStatViewWidget = new NetifStatViewWidget(pQWidget);

    QResizeEvent *pevent;

    Stub b1;
    b1.set(ADDR(NetifStatViewWidget,updateWidgetGeometry),stub1_updateWidgetGeometry);
    pNetifStatViewWidget->resizeEvent(pevent);

    EXPECT_EQ(stub_flag,1);

    pQWidget->deleteLater();
    pNetifStatViewWidget->deleteLater();
}

int stub2_updateWidgetGeometry(){
    stub_flag = 2;
    return 0;
}

TEST(Ut_Ut_NetifStatViewWidget_onModelUpdate,Ut_NetifStatViewWidget_onModelUpdate_001)
{
    QWidget *pQWidget = new QWidget;
    NetifStatViewWidget *pNetifStatViewWidget = new NetifStatViewWidget(pQWidget);

    Stub b2;
    b2.set(ADDR(NetifStatViewWidget,updateWidgetGeometry),stub2_updateWidgetGeometry);

    pNetifStatViewWidget->onModelUpdate();
    EXPECT_EQ(stub_flag,2);

    pQWidget->deleteLater();
    pNetifStatViewWidget->deleteLater();
}

TEST(Ut_Ut_NetifStatViewWidget_onSetItemActiveStatus,Ut_NetifStatViewWidget_onSetItemActiveStatus_001)
{
    QWidget *pQWidget = new QWidget;
    NetifStatViewWidget *pNetifStatViewWidget = new NetifStatViewWidget(pQWidget);

    QString sMac("a8:a1:59:6d:da:b1");

    pNetifStatViewWidget->onSetItemActiveStatus(sMac);

    EXPECT_STREQ(pNetifStatViewWidget->m_currentMac,sMac.toUtf8());


    pQWidget->deleteLater();
    pNetifStatViewWidget->deleteLater();
}


TEST(Ut_Ut_NetifStatViewWidget_showItemOnlyeOne,Ut_NetifStatViewWidget_showItemOnlyeOne_001)
{
    QWidget *pQWidget = new QWidget;
    NetifStatViewWidget *pNetifStatViewWidget = new NetifStatViewWidget(pQWidget);

    pNetifStatViewWidget->showItemOnlyeOne();

    EXPECT_EQ(pNetifStatViewWidget->height(),pNetifStatViewWidget->m_centralWidget->height());
    EXPECT_EQ(pNetifStatViewWidget->width(),pNetifStatViewWidget->m_centralWidget->width());

    pQWidget->deleteLater();
    pNetifStatViewWidget->deleteLater();
}


TEST(Ut_Ut_NetifStatViewWidget_showItemDouble,Ut_NetifStatViewWidget_showItemDouble_001)
{
    QWidget *pQWidget = new QWidget;
    NetifStatViewWidget *pNetifStatViewWidget = new NetifStatViewWidget(pQWidget);

    pNetifStatViewWidget->showItemDouble();
    EXPECT_EQ(pNetifStatViewWidget->height(),pNetifStatViewWidget->m_centralWidget->height());
    EXPECT_EQ(pNetifStatViewWidget->width(),pNetifStatViewWidget->m_centralWidget->width());

    pQWidget->deleteLater();
    pNetifStatViewWidget->deleteLater();
}



