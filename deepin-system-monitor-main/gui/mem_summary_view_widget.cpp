// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mem_summary_view_widget.h"
#include "system/device_db.h"
#include "system/mem.h"
#include "common/common.h"
#include "base/base_detail_item_delegate.h"
#include "ddlog.h"

#include <QHeaderView>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <DApplication>
#include <DStyle>
#include <QScroller>

using namespace DDLog;
using namespace core::system;
using namespace common::format;

#define SUMMARY_CHART_LINE_ALPH 0.13

class DeailTableModel : public QAbstractTableModel
{
public:
    explicit DeailTableModel(QObject *parent = nullptr);
    virtual ~DeailTableModel();

private:
    MemInfo *m_memInfo;

protected:
    int rowCount(const QModelIndex &) const
    {
        return 7;
    }

    int columnCount(const QModelIndex &) const
    {
        return 2;
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if (!index.isValid()) {
            qCDebug(app) << "DeailTableModel::data invalid index";
            return QVariant();
        }

        int row = index.row();
        int column = index.column();

        if (role == Qt::DisplayRole) {
            qCDebug(app) << "DeailTableModel::data role is Qt::DisplayRole, row:" << row << "column:" << column;
            switch (row) {
            case 0:
                if (column == 0)
                    return QApplication::translate("DeailTableModel", "Used");
                else if (column == 1)
                    return QApplication::translate("DeailTableModel", "Available");
                break;
            case 1:
                if (column == 0)
                    return QApplication::translate("DeailTableModel", "Shared");
                else if (column == 1)
                    return QApplication::translate("DeailTableModel", "Cached");
                break;
            case 2:
                if (column == 0)
                    return QApplication::translate("DeailTableModel", "Buffers");
                else if (column == 1)
                    return QApplication::translate("DeailTableModel", "Cached swap");
                break;
            case 3:
                if (column == 0)
                    return QApplication::translate("DeailTableModel", "Active");
                else if (column == 1)
                    return QApplication::translate("DeailTableModel", "Inactive");
                break;
            case 4:
                if (column == 0)
                    return QApplication::translate("DeailTableModel", "Dirty");
                else if (column == 1)
                    return QApplication::translate("DeailTableModel", "Mapped");
                break;
            case 5:
                if (column == 0)
                    return QApplication::translate("DeailTableModel", "Total swap");
                else if (column == 1)
                    return QApplication::translate("DeailTableModel", "Free swap");
                break;
            case 6:
                if (column == 0)
                    return QApplication::translate("DeailTableModel", "Slab");
                break;
            default:
                break;
            }
        } else if (role == Qt::UserRole) {
            qCDebug(app) << "DeailTableModel::data role is Qt::UserRole, row:" << row << "column:" << column;
            switch (row) {
            case 0:
                if (column == 0)
                    return formatUnit_memory_disk((m_memInfo->memTotal() - m_memInfo->memAvailable())  << 10, B, 1);
                else if (column == 1)
                    return formatUnit_memory_disk(m_memInfo->memAvailable() << 10, B, 1);
                break;
            case 1:
                if (column == 0)
                    return formatUnit_memory_disk(m_memInfo->shmem() << 10, B, 1);
                else if (column == 1)
                    return formatUnit_memory_disk(m_memInfo->cached() << 10, B, 1);
                break;
            case 2:
                if (column == 0)
                    return formatUnit_memory_disk(m_memInfo->buffers() << 10, B, 1);
                else if (column == 1)
                    return formatUnit_memory_disk(m_memInfo->swapCached() << 10, B, 1);
                break;
            case 3:
                if (column == 0)
                    return formatUnit_memory_disk(m_memInfo->active() << 10, B, 1);
                else if (column == 1)
                    return formatUnit_memory_disk(m_memInfo->inactive() << 10, B, 1);
                break;
            case 4:
                if (column == 0)
                    return formatUnit_memory_disk(m_memInfo->dirty() << 10, B, 1);
                else if (column == 1)
                    return formatUnit_memory_disk(m_memInfo->mapped() << 10, B, 1);
                break;
            case 5:
                if (column == 0)
                    return formatUnit_memory_disk(m_memInfo->swapTotal() << 10, B, 1);
                else if (column == 1)
                    return formatUnit_memory_disk(m_memInfo->swapFree() << 10, B, 1);
                break;
            case 6:
                if (column == 0)
                    return formatUnit_memory_disk(m_memInfo->slab() << 10, B, 1);
                break;
            default:
                break;
            }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        } else if (role == Qt::TextColorRole) {
            qCDebug(app) << "DeailTableModel::data role is Qt::TextColorRole, row:" << row << "column:" << column;
            const auto &palette = DApplicationHelper::instance()->applicationPalette();
#else
        } else if (role == Qt::ForegroundRole) {
            qCDebug(app) << "DeailTableModel::data role is Qt::ForegroundRole, row:" << row << "column:" << column;
            const auto &palette = DGuiApplicationHelper::instance()->applicationPalette();
#endif
            return palette.color(DPalette::Text);
        }

        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &) const
    {
        return Qt::NoItemFlags;
    }
};

DeailTableModel::DeailTableModel(QObject *parent): QAbstractTableModel(parent)
{
    qCDebug(app) << "DeailTableModel constructor";
    m_memInfo = DeviceDB::instance()->memInfo();
}

DeailTableModel::~DeailTableModel()
{
    // qCDebug(app) << "DeailTableModel destructor";
}

MemSummaryViewWidget::MemSummaryViewWidget(QWidget *parent)
    : DTableView(parent)
{
    qCDebug(app) << "MemSummaryViewWidget constructor";
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);

    this->horizontalHeader()->setStretchLastSection(true);

    this->horizontalHeader()->setSectionResizeMode(DHeaderView::Stretch);
    this->verticalHeader()->setSectionResizeMode(DHeaderView::ResizeToContents);
    this->setGridStyle(Qt::NoPen);
    this->setFrameShape(QFrame::NoFrame);

    m_model = new DeailTableModel(this);
    this->setModel(m_model);
    this->setItemDelegate(new BaseDetailItemDelegate(this));

    setFixedHeight(260);

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

void MemSummaryViewWidget::onModelUpdate()
{
    // qCDebug(app) << "MemSummaryViewWidget onModelUpdate";
    this->viewport()->update();
}

void MemSummaryViewWidget::fontChanged(const QFont &font)
{
    // qCDebug(app) << "MemSummaryViewWidget fontChanged";
    m_font = font;
    this->setFont(m_font);
}

void MemSummaryViewWidget::paintEvent(QPaintEvent *event)
{
    // qCDebug(app) << "MemSummaryViewWidget paintEvent";
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

