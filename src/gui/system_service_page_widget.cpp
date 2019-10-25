#include <QHBoxLayout>

#include "main_window.h"
#include "system_service_page_widget.h"
#include "system_service_table_view.h"

SystemServicePageWidget::SystemServicePageWidget(DWidget *parent)
    : DFrame(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    m_svcTableView = new SystemServiceTableView(this);
    layout->addWidget(m_svcTableView);
    layout->setContentsMargins(10, 10, 10, 10);
    setLayout(layout);
}
SystemServicePageWidget::~SystemServicePageWidget() {}
