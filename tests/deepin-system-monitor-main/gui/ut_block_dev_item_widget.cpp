//#include "ut_block_dev_item_widget.h"
//#include "block_dev_item_widget.h"
//#include "chart_view_widget.h"
//#include <QSignalSpy>

//Ut_BlockDevItemWidget::Ut_BlockDevItemWidget()
//{

//}

//TEST(Ut_BlockDevItemWidget, Ut_BlockDevItemWidget_BlockDevItemWidget_001)
//{
//    BlockDevItemWidget *itemWidget = new BlockDevItemWidget;

//    QColor readColor {"#8F88FF"};
//    QColor writeColor {"#6AD787"};
//    EXPECT_TRUE(itemWidget);

//    EXPECT_TRUE(itemWidget->m_memChartWidget);
//    EXPECT_TRUE(itemWidget->m_memChartWidget->m_speedAxis);
//    EXPECT_EQ(itemWidget->m_memChartWidget->m_data1Color, readColor);
//    EXPECT_EQ(itemWidget->m_memChartWidget->m_data2Color, writeColor);

//    itemWidget->deleteLater();
//}

//TEST(Ut_BlockDevItemWidget, Ut_BlockDevItemWidget_updateWidgetGeometry_001)
//{
//    BlockDevItemWidget *itemWidget = new BlockDevItemWidget;

//    QFont font("Times");
//    itemWidget->m_font = font;
//    itemWidget->m_mode = BlockDevItemWidget::TITLE_HORIZONTAL;
//    int fontHeight = QFontMetrics(itemWidget->m_font).height();
//    itemWidget->updateWidgetGeometry();

//    EXPECT_EQ(QFontMetrics(itemWidget->m_font).height(), QFontMetrics(font).height());

//    itemWidget->deleteLater();
//}

//TEST(Ut_BlockDevItemWidget, Ut_BlockDevItemWidget_updateWidgetGeometry_002)
//{
//    BlockDevItemWidget *itemWidget = new BlockDevItemWidget;

//    QFont font("Times");
//    itemWidget->m_font = font;
//    itemWidget->m_mode = BlockDevItemWidget::TITLE_VERTICAL;
//    int fontHeight = QFontMetrics(itemWidget->m_font).height();
//    itemWidget->updateWidgetGeometry();

//    EXPECT_EQ(QFontMetrics(itemWidget->m_font).height(), QFontMetrics(font).height());

//    itemWidget->deleteLater();
//}

//TEST(Ut_BlockDevItemWidget, Ut_BlockDevItemWidget_fontChanged_001)
//{
//    BlockDevItemWidget *itemWidget = new BlockDevItemWidget;
//    QFont font("Times");

//    itemWidget->fontChanged(font);

//    EXPECT_EQ(itemWidget->m_font, font);

//    itemWidget->deleteLater();
//}

//void BlockDevItemWidget_reszieEvent(QResizeEvent *event)
//{

//}

//TEST(Ut_BlockDevItemWidget, Ut_BlockDevItemWidget_resizeEvent_001)
//{
//    BlockDevItemWidget *itemWidget = new BlockDevItemWidget;
//    QResizeEvent event(QSize(10, 10), QSize(20, 20));
//    QFont font("Times");
//    itemWidget->m_font = font;
//    itemWidget->m_mode = BlockDevItemWidget::TITLE_VERTICAL;
//    int fontHeight = QFontMetrics(itemWidget->m_font).height();

//    itemWidget->resizeEvent(&event);

//    EXPECT_EQ(QFontMetrics(itemWidget->m_font).height(), QFontMetrics(font).height());

//    itemWidget->deleteLater();
//}

//TEST(Ut_BlockDevItemWidget, Ut_BlockDevItemWidget_updateData_001)
//{
//    BlockDevItemWidget *itemWidget = new BlockDevItemWidget;
//    BlockDevice info;
//    itemWidget->updateData(info);

//    EXPECT_EQ(info.deviceName(), itemWidget->m_blokeDeviceInfo.deviceName());

//    itemWidget->deleteLater();
//}

//TEST(Ut_BlockDevItemWidget, Ut_BlockDevItemWidget_activeItemWidget_001)
//{
//    BlockDevItemWidget *itemWidget = new BlockDevItemWidget;

//    itemWidget->activeItemWidget(true);

//    EXPECT_TRUE(itemWidget->m_isActive);

//    itemWidget->deleteLater();
//}

//TEST(Ut_BlockDevItemWidget, Ut_BlockDevItemWidget_activeItemWidget_002)
//{
//    BlockDevItemWidget *itemWidget = new BlockDevItemWidget;

//    itemWidget->activeItemWidget(false);

//    EXPECT_FALSE(itemWidget->m_isActive);

//    itemWidget->deleteLater();
//}

//TEST(Ut_BlockDevItemWidget, Ut_BlockDevItemWidget_mousePressEvent_001)
//{
//    BlockDevItemWidget *itemWidget = new BlockDevItemWidget;
//    itemWidget->grabMouse();

//    QSignalSpy signalSpy(itemWidget, &BlockDevItemWidget::clicked);
//    EXPECT_TRUE(signalSpy.count() == 0);

//    emit itemWidget->clicked(itemWidget->m_blokeDeviceInfo.deviceName());
//    signalSpy.wait(50);
//    EXPECT_TRUE(signalSpy.count() == 1);

//    itemWidget->deleteLater();
//}

//TEST(Ut_BlockDevItemWidget, Ut_BlockDevItemWidget_paintEvent_001)
//{
//    BlockDevItemWidget *itemWidget = new BlockDevItemWidget;
//    EXPECT_TRUE(!itemWidget->grab().isNull());
//    itemWidget->deleteLater();
//}

//TEST(Ut_BlockDevItemWidget, Ut_BlockDevItemWidget_setMode_001)
//{
//    BlockDevItemWidget *itemWidget = new BlockDevItemWidget;
//    int mode = 1;
//    itemWidget->setMode(mode);
//    EXPECT_EQ(mode, itemWidget->m_mode);
//    itemWidget->deleteLater();
//}
