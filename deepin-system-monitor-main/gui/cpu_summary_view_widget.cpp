// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cpu_summary_view_widget.h"
#include "ddlog.h"

using namespace DDLog;

#define SUMMARY_CHART_LINE_ALPH 0.13

CPUSummaryTableModel::CPUSummaryTableModel(CPUInfoModel *model, QObject *parent): QAbstractTableModel(parent), m_model(model)
{
    qCDebug(app) << "CPUSummaryTableModel constructor";
    connect(m_model, &CPUInfoModel::modelUpdated, this, &CPUSummaryTableModel::onModelUpdated);
}

CPUSummaryTableModel::~CPUSummaryTableModel()
{
    qCDebug(app) << "CPUSummaryTableModel destructor";
}

void CPUSummaryTableModel::onModelUpdated()
{
    qCDebug(app) << "CPUSummaryTableModel::onModelUpdated";
    QAbstractTableModel::dataChanged(index(0, 0), index(9, 1));
}

CPUDetailSummaryTable::CPUDetailSummaryTable(CPUInfoModel *dataModel, QWidget *parent)
    : DTableView(parent)
{
    qCDebug(app) << "CPUDetailSummaryTable constructor";
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
    qCDebug(app) << "CPUDetailSummaryTable::onModelUpdate";
    this->viewport()->update();
}

void CPUDetailSummaryTable::fontChanged(const QFont &font)
{
    // qCDebug(app) << "CPUDetailSummaryTable::fontChanged";
    m_font = font;
    this->setFont(m_font);
    setFixedHeight(260);
}

void CPUDetailSummaryTable::paintEvent(QPaintEvent *event)
{
    // qCDebug(app) << "CPUDetailSummaryTable::paintEvent";
    DTableView::paintEvent(event);

    QPainter painter(this->viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const auto &palette = DApplicationHelper::instance()->applicationPalette();
#else
    const auto &palette = DGuiApplicationHelper::instance()->applicationPalette();
#endif
    QColor frameColor = palette.color(DPalette::FrameBorder);
    frameColor.setAlphaF(SUMMARY_CHART_LINE_ALPH);

    painter.setPen(QPen(frameColor, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(this->horizontalHeader()->sectionSize(0) - 1, 2, this->horizontalHeader()->sectionSize(0) - 1, this->viewport()->height() - 2);
    painter.drawRoundedRect(this->rect().adjusted(1, 1, -1, -1), 6, 6);
}

