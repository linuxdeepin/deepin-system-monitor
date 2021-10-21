#include "ut_netif_detail_view_widget.h"
#include "netif_detail_view_widget.h"
#include "detail_view_stacked_widget.h"
#include "netif_stat_view_widget.h"

#include <DApplication>

static int stub_flag;
Ut_NetifDetailViewWidget::Ut_NetifDetailViewWidget()
{

}

void stub_updateData()
{
    stub_flag=1;
    return;
}


TEST(Ut_NetifDetailViewWidget_NetifDetailViewWidget,Ut_NetifDetailViewWidget_NetifDetailViewWidget_001)
{
    QWidget *pQWidget = new QWidget;
    DetailViewStackedWidget *p = new DetailViewStackedWidget(pQWidget);

    Stub b;
    b.set(ADDR(NetifDetailViewWidget,updateData),stub_updateData);
    NetifDetailViewWidget *pNetifDetailViewWidget = new NetifDetailViewWidget(p);

    EXPECT_EQ(stub_flag,1);

    pQWidget->deleteLater();
    p->deleteLater();
    pNetifDetailViewWidget->deleteLater();
}

void stub_fontChanged()
{
    stub_flag=2;
    return;
}
TEST(Ut_NetifDetailViewWidget_detailFontChanged,Ut_NetifDetailViewWidget_detailFontChanged_001)
{
    QWidget *pQWidget = new QWidget;
    DetailViewStackedWidget *p = new DetailViewStackedWidget(pQWidget);

    NetifDetailViewWidget *pNetifDetailViewWidget = new NetifDetailViewWidget(p);

    Stub b;
    b.set(ADDR(NetifStatViewWidget,fontChanged),stub_fontChanged);
    QFont font;
    pNetifDetailViewWidget->detailFontChanged(font);

     EXPECT_EQ(stub_flag,2);

    pQWidget->deleteLater();
    p->deleteLater();
    pNetifDetailViewWidget->deleteLater();
}
void stub_StatupdateData()
{
    stub_flag=3;
    return;
}

TEST(Ut_NetifDetailViewWidget_updateData,Ut_NetifDetailViewWidget_updateData_001)
{
    QWidget *pQWidget = new QWidget;
    DetailViewStackedWidget *p = new DetailViewStackedWidget(pQWidget);

    NetifDetailViewWidget *pNetifDetailViewWidget = new NetifDetailViewWidget(p);

    Stub b;
    b.set(ADDR(NetifStatViewWidget,onModelUpdate),stub_StatupdateData);

    pNetifDetailViewWidget->updateData();
    EXPECT_EQ(stub_flag,3);

    pQWidget->deleteLater();
    p->deleteLater();
    pNetifDetailViewWidget->deleteLater();
}
