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
#include "cpu_summary_view_widget.h"


CPUSummaryTableModel::CPUSummaryTableModel(CPUInfoModel *model, QObject *parent): QAbstractTableModel(parent), m_model(model)
{
    connect(m_model, &CPUInfoModel::modelUpdated, this, &CPUSummaryTableModel::onModelUpdated);
}

CPUSummaryTableModel::~CPUSummaryTableModel()
{

}

void CPUSummaryTableModel::onModelUpdated()
{
    QAbstractTableModel::dataChanged(index(0, 0), index(9, 1));
}

CPUDetailSummaryTable::CPUDetailSummaryTable(CPUInfoModel *dataModel, QWidget *parent)
    : DTableView(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);

    this->horizontalHeader()->setStretchLastSection(true);

    this->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->setGridStyle(Qt::NoPen);
    this->setFrameShape(QFrame::NoFrame);

    m_model = new CPUSummaryTableModel(dataModel, this);
    this->setModel(m_model);
    this->setItemDelegate(new BaseDetailItemDelegate(this));

    auto *scroller = QScroller::scroller(viewport());
    auto prop = scroller->scrollerProperties();
    // turn off overshoot to fix performance issue
    prop.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0);
    prop.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 1);
    // lock scroll direction to fix performance issue
    prop.setScrollMetric(QScrollerProperties::AxisLockThreshold, 1);
    scroller->setScrollerProperties(prop);
    // enable touch gesture
    QScroller::grabGesture(viewport(), QScroller::TouchGesture);
}

void CPUDetailSummaryTable::onModelUpdate()
{
    this->viewport()->update();
}

void CPUDetailSummaryTable::fontChanged(const QFont &font)
{
    m_font = font;
    this->setFont(m_font);
    setFixedHeight(260);
}

void CPUDetailSummaryTable::paintEvent(QPaintEvent *event)
{
    DTableView::paintEvent(event);

    QPainter painter(this->viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);
    const auto &palette = DApplicationHelper::instance()->applicationPalette();
    QColor frameColor = palette.color(DPalette::TextTips);
    frameColor.setAlphaF(0.3);

    painter.setPen(QPen(frameColor, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(this->horizontalHeader()->sectionSize(0) - 1, 2, this->horizontalHeader()->sectionSize(0) - 1, this->viewport()->height() - 2);
    painter.drawRoundedRect(this->rect().adjusted(1, 1, -1, -1), 6, 6);
}

