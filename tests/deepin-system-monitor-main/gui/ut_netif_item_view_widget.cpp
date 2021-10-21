#include "ut_netif_item_view_widget.h"
#include "netif_item_view_widget.h"
#include "chart_view_widget.h"
#include <QSignalSpy>

Ut_NetifItemViewWidget::Ut_NetifItemViewWidget()
{

}

TEST(Ut_NetifItemViewWidget_NetifItemViewWidget,Ut_NetifItemViewWidget_NetifItemViewWidget_001)
{
    QWidget *pQWidget = new QWidget;

    NetifItemViewWidget *pNetifItemViewWidget = new NetifItemViewWidget(pQWidget);

    EXPECT_EQ(pNetifItemViewWidget->m_ifname,"Ethernet");
    ASSERT_NE(pNetifItemViewWidget->m_ChartWidget,nullptr);



    pQWidget->deleteLater();
    pNetifItemViewWidget->deleteLater();
}

TEST(Ut_NetifItemViewWidget_mousePressEvent,Ut_NetifItemViewWidget_mousePressEvent_001)
{
    QWidget *pQWidget = new QWidget;

    NetifItemViewWidget *pNetifItemViewWidget = new NetifItemViewWidget(pQWidget);

    QSignalSpy signalSpy(pNetifItemViewWidget, &NetifItemViewWidget::clicked);
    EXPECT_TRUE(signalSpy.count() == 0);

    emit pNetifItemViewWidget->clicked(pNetifItemViewWidget->m_mac);
    signalSpy.wait(1000);
    EXPECT_TRUE(signalSpy.count() == 1);

    pQWidget->deleteLater();
    pNetifItemViewWidget->deleteLater();
}


TEST(Ut_NetifItemViewWidget_paintEvent,Ut_NetifItemViewWidget_paintEvent_001)
{
    QWidget *pQWidget = new QWidget;

    NetifItemViewWidget *pNetifItemViewWidget = new NetifItemViewWidget(pQWidget);

    EXPECT_TRUE(!pNetifItemViewWidget->grab().isNull());

    pQWidget->deleteLater();
    pNetifItemViewWidget->deleteLater();
}



TEST(Ut_NetifItemViewWidget_fontChanged,Ut_NetifItemViewWidget_fontChanged_001)
{
    QWidget *pQWidget = new QWidget;
    NetifItemViewWidget *pNetifItemViewWidget = new NetifItemViewWidget(pQWidget);

    const QFont *pfont = new QFont;
    pNetifItemViewWidget->fontChanged(*pfont);

    EXPECT_EQ(pNetifItemViewWidget->m_font.key(),pfont->key());

    pQWidget->deleteLater();
    pNetifItemViewWidget->deleteLater();
}

TEST(Ut_NetifItemViewWidget_setMode,Ut_NetifItemViewWidget_setMode_001)
{
    QWidget *pQWidget = new QWidget;
    NetifItemViewWidget *pNetifItemViewWidget = new NetifItemViewWidget(pQWidget);

    int mode = 0;
    pNetifItemViewWidget->setMode(mode);

    EXPECT_EQ(pNetifItemViewWidget->m_mode,mode);

    pQWidget->deleteLater();
    pNetifItemViewWidget->deleteLater();
}


TEST(Ut_NetifItemViewWidget_updateActiveStatus,Ut_NetifItemViewWidget_updateActiveStatus_001)
{
    QWidget *pQWidget = new QWidget;
    NetifItemViewWidget *pNetifItemViewWidget = new NetifItemViewWidget(pQWidget);

    bool active = true;
    pNetifItemViewWidget->updateActiveStatus(active);

    EXPECT_EQ(pNetifItemViewWidget->m_isActive,active);

    pQWidget->deleteLater();
    pNetifItemViewWidget->deleteLater();
}

TEST(Ut_NetifItemViewWidget_updateActiveStatus,Ut_NetifItemViewWidget_updateActiveStatus_002)
{
    QWidget *pQWidget = new QWidget;
    NetifItemViewWidget *pNetifItemViewWidget = new NetifItemViewWidget(pQWidget);

    bool active = false;
    pNetifItemViewWidget->updateActiveStatus(active);

    EXPECT_EQ(pNetifItemViewWidget->m_isActive,active);

    pQWidget->deleteLater();
    pNetifItemViewWidget->deleteLater();
}

TEST(Ut_NetifItemViewWidget_updateWidgetGeometry,Ut_NetifItemViewWidget_updateWidgetGeometry_001)
{
    QWidget *pQWidget = new QWidget;
    NetifItemViewWidget *pNetifItemViewWidget = new NetifItemViewWidget(pQWidget);

    pNetifItemViewWidget->m_mode = 0;
    int pfontHeight = QFontMetrics(pNetifItemViewWidget->m_font).height();
    pNetifItemViewWidget->updateWidgetGeometry();
    EXPECT_EQ(pNetifItemViewWidget->m_ChartWidget->geometry().x(),6);
    EXPECT_EQ(pNetifItemViewWidget->m_ChartWidget->geometry().y(),pfontHeight / 2);
    EXPECT_EQ(pNetifItemViewWidget->m_ChartWidget->geometry().width(), pNetifItemViewWidget->width() - 2 * 6);
    EXPECT_EQ(pNetifItemViewWidget->m_ChartWidget->geometry().height(), pNetifItemViewWidget->height() - pfontHeight / 2 - 6);

    pQWidget->deleteLater();
    pNetifItemViewWidget->deleteLater();
}

//TEST(Ut_NetifItemViewWidget_updateWidgetGeometry,Ut_NetifItemViewWidget_updateWidgetGeometry_002)
//{
//    QWidget *pQWidget = new QWidget;
//    NetifItemViewWidget *pNetifItemViewWidget = new NetifItemViewWidget(pQWidget);

//    pNetifItemViewWidget->m_mode = 1;
//    int pfontHeight = QFontMetrics(pNetifItemViewWidget->m_font).height();
//    pNetifItemViewWidget->updateWidgetGeometry();
//    EXPECT_EQ(pNetifItemViewWidget->m_ChartWidget->geometry().x(),6);
//    EXPECT_EQ(pNetifItemViewWidget->m_ChartWidget->geometry().y(),pfontHeight * 4);
//    EXPECT_EQ(pNetifItemViewWidget->m_ChartWidget->geometry().width(), pNetifItemViewWidget->width() - 2 * 6);
//    EXPECT_EQ(pNetifItemViewWidget->m_ChartWidget->geometry().height(), pNetifItemViewWidget->height() - pfontHeight * 4 - 6);

//    pQWidget->deleteLater();
//    pNetifItemViewWidget->deleteLater();
//}








