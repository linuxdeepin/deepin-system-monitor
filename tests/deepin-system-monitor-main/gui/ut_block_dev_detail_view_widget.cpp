#include "ut_block_dev_detail_view_widget.h"
#include "block_dev_detail_view_widget.h"
#include "block_dev_stat_view_widget.h"
#include "block_dev_summary_view_widget.h"
#include "block_dev_item_widget.h"
#include "detail_view_stacked_widget.h"
#include <QSignalSpy>

Ut_BlockDevDetailViewWidget::Ut_BlockDevDetailViewWidget()
{

}

TEST(Ut_BlockDevDetailViewWidget, Ut_BlockDevDetailViewWidget_BlockDevDetailViewWidget_001)
{
    DetailViewStackedWidget *stackViewWidget = new DetailViewStackedWidget(nullptr);
    BlockDevDetailViewWidget *pBlockWidget = new BlockDevDetailViewWidget(stackViewWidget);

    EXPECT_STREQ(pBlockWidget->objectName().toLocal8Bit(), "BlockDevDetailViewWidget");

    EXPECT_STREQ(pBlockWidget->title().toLocal8Bit(), "Disks");

    EXPECT_TRUE(pBlockWidget->m_blockStatWidget);

    EXPECT_TRUE(pBlockWidget->m_blocksummaryWidget);

    pBlockWidget->deleteLater();
}

TEST(Ut_BlockDevDetailViewWidget, Ut_BlockDevDetailViewWidget_detailFontChanged_001)
{
    DetailViewStackedWidget *stackViewWidget = new DetailViewStackedWidget(nullptr);
    BlockDevDetailViewWidget *pBlockWidget = new BlockDevDetailViewWidget(stackViewWidget);
    QFont serifFont("Times", 10, QFont::Bold);
    pBlockWidget->detailFontChanged(serifFont);

    foreach (auto widget, pBlockWidget->m_blockStatWidget->m_listBlockItemWidget) {
        EXPECT_EQ(widget->font().family(), serifFont.family());
        EXPECT_EQ(widget->font().bold(), serifFont.bold());
    }
    EXPECT_EQ(pBlockWidget->m_blocksummaryWidget->font().family(), serifFont.family());
    EXPECT_EQ(pBlockWidget->m_blocksummaryWidget->font().bold(), serifFont.bold());

    pBlockWidget->deleteLater();
}
