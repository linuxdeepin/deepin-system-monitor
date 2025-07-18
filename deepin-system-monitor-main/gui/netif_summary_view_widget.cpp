// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netif_summary_view_widget.h"
#include "ddlog.h"

using namespace DDLog;
using namespace core::system;
using namespace common::format;

using namespace common::core;

#define SUMMARY_CHART_LINE_ALPH 0.13




NetInfoDetailItemDelegate::NetInfoDetailItemDelegate(QObject *parent): QStyledItemDelegate(parent)
{
    qCDebug(app) << "NetInfoDetailItemDelegate constructor";
}

NetInfoDetailItemDelegate::~NetInfoDetailItemDelegate()
{
    qCDebug(app) << "NetInfoDetailItemDelegate destructor";
}



NetInfoModel::NetInfoModel(QObject *parent): QAbstractTableModel(parent)
{
    qCDebug(app) << "NetInfoModel constructor";
}

NetInfoModel::~NetInfoModel()
{
    qCDebug(app) << "NetInfoModel destructor";
}

NetifSummaryViewWidget::NetifSummaryViewWidget(QWidget *parent)
    : DTableView(parent)
{
    qCDebug(app) << "NetifSummaryViewWidget constructor";
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);

    this->horizontalHeader()->setStretchLastSection(true);

    this->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    this->setGridStyle(Qt::NoPen);
    this->setFrameShape(QFrame::NoFrame);

    m_netInfoModel = new NetInfoModel(this);
    this->setModel(m_netInfoModel);
    m_netInfoDetailItemDelegate = new NetInfoDetailItemDelegate(this);
    this->setItemDelegate(dynamic_cast<QAbstractItemDelegate *>(m_netInfoDetailItemDelegate));

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


    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
                this, &NetifSummaryViewWidget::fontChanged);
}

void NetifSummaryViewWidget::onNetifItemClicked(const QString &mac)
{
    qCDebug(app) << "NetifSummaryViewWidget onNetifItemClicked";
    // 按照点击IP刷新数据
    m_strCurrentKey = mac;
    m_netInfoModel->refreshNetifInfo(m_strCurrentKey);
}

void NetifSummaryViewWidget::fontChanged(const QFont &font)
{
    qCDebug(app) << "NetifSummaryViewWidget fontChanged";
    m_font = font;
    this->setFont(m_font);;
    m_netInfoDetailItemDelegate->setFont(m_font);
    setFixedHeight(260);
}

void NetifSummaryViewWidget::onModelUpdate()
{
    qCDebug(app) << "NetifSummaryViewWidget onModelUpdate";
    m_netInfoModel->refreshNetifInfo(m_strCurrentKey);  // 按时间间隔刷新数据
}

void NetifSummaryViewWidget::paintEvent(QPaintEvent *event)
{
    // qCDebug(app) << "NetifSummaryViewWidget paintEvent";
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


