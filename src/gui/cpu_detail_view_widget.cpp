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
#include "cpu_detail_view_widget.h"

#include <QGridLayout>
#include <QTableWidget>
#include <QPainter>
#include <QVBoxLayout>
#include <QHeaderView>


CPUDetailViewWidget::CPUDetailViewWidget(QWidget *parent)
    : QWidget(parent)
{
    m_graphicsTable = new QWidget(this);
    QGridLayout  *graphicsLayout = new QGridLayout(this);
    for (int i = 0; i < 16; ++i) {
        CPUCoreItem *item = new CPUCoreItem(this);
        item->setFixedSize(82, 50);
        graphicsLayout->addWidget(item, i / 4, i % 4);
    }
    m_graphicsTable->setLayout(graphicsLayout);
    m_textTable = new QTableWidget(this);
    m_textTable->setRowCount(8);
    m_textTable->setColumnCount(2);
    m_textTable->horizontalHeader()->hide();
    m_textTable->verticalHeader()->hide();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_graphicsTable);
    layout->addWidget(m_textTable);
    setLayout(layout);
}

void CPUCoreItem::paintEvent(QPaintEvent *event)
{
    {
        QPainter painter(this);
        painter.setBrush(Qt::black);
        painter.setPen(Qt::NoPen);
        painter.drawRect(this->rect());
    }
}
