/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "block_dev_detail_view_widget.h"
#include "block_dev_stat_view_widget.h"
#include "block_dev_summary_view_widget.h"

#include <DApplication>

BlockDevDetailViewWidget::BlockDevDetailViewWidget(QWidget *parent)
    : BaseDetailViewWidget(parent)
{
    setTitle(DApplication::translate("Process.Graph.View", "Disk"));
    m_blockStatWidget = new BlockStatViewWidget(this);
    m_blocksummaryWidget = new BlockDevSummaryViewWidget(this);
    m_centralLayout->addWidget(m_blockStatWidget);
    m_centralLayout->addWidget(m_blocksummaryWidget);
   // m_centralLayout->setSpacing(0);
    //m_centralLayout->setMargin(0);
    connect(m_blockStatWidget,&BlockStatViewWidget::changeInfo,m_blocksummaryWidget,&BlockDevSummaryViewWidget::chageSummaryInfo);

    fontChanged(DApplication::font());
}
void BlockDevDetailViewWidget::fontChanged(const QFont &font)
{
    BaseDetailViewWidget::fontChanged(font);
    m_blockStatWidget->fontChanged(font);
    m_blocksummaryWidget->fontChanged(font);
    //Child Todo
}
