#include <DApplication>
#include <DApplicationHelper>
#include <DStyle>
#include <DStyleHelper>
#include <QHBoxLayout>

#include "main_window.h"
#include "system_service_page_widget.h"
#include "system_service_table_view.h"

SystemServicePageWidget::SystemServicePageWidget(DWidget *parent)
    : DFrame(parent)
{
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    QHBoxLayout *layout = new QHBoxLayout(this);
    m_svcTableView = new SystemServiceTableView(this);
    layout->addWidget(m_svcTableView);
    layout->setContentsMargins(margin, margin, margin, margin);
    setLayout(layout);
}
SystemServicePageWidget::~SystemServicePageWidget() {}

void SystemServicePageWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QPainterPath path;
    path.addRect(QRectF(rect()));
    painter.setOpacity(1);

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QColor bgColor = palette.color(DPalette::Background);

    painter.fillPath(path, bgColor);
}
