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
#include "mem_summary_view_widget.h"
#include "system/device_db.h"
#include "system/mem.h"
#include "common/common.h"

#include <QHeaderView>
#include <DApplicationHelper>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <DApplication>
#include <DStyle>

using namespace core::system;
using namespace common::format;
class DetailItemDelegate : public QStyledItemDelegate
{
public:
    explicit DetailItemDelegate(QObject *parent = nullptr);
    virtual ~DetailItemDelegate();

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const
    {
        auto palette = option.palette;
        QBrush background = palette.color(DPalette::Active, DPalette::Base);
        if (!(index.row() & 1)) background = palette.color(DPalette::Active, DPalette::AlternateBase);

        painter->save();
        QPainterPath clipPath;
        clipPath.addRoundedRect(option.widget->rect(), 8, 8);
        painter->setClipPath(clipPath);

        painter->setPen(Qt::NoPen);
        painter->setBrush(background);
        painter->drawRect(option.rect);

        if (index.isValid()) {
            const auto &palette = DApplicationHelper::instance()->applicationPalette();
            painter->setPen(palette.color(DPalette::TextTips));

            const QString &text = index.data(Qt::UserRole).toString();
            painter->drawText(option.rect.adjusted(0, 0, -10, 0), Qt::AlignRight | Qt::AlignVCenter, text);
        }
        painter->restore();

        QStyledItemDelegate::paint(painter, option, index);
    }
};

DetailItemDelegate::DetailItemDelegate(QObject *parent): QStyledItemDelegate(parent)
{
}

DetailItemDelegate::~DetailItemDelegate()
{
}

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
        if (!index.isValid())
            return QVariant();

        int row = index.row();
        int column = index.column();

        if (role == Qt::DisplayRole) {
            switch (row) {
            case 0:
                if (column == 0)
                    return tr("Used");
                else if (column == 1)
                    return tr("Available");
                break;
            case 1:
                if (column == 0)
                    return tr("Shmem");
                else if (column == 1)
                    return tr("Cached");
                break;
            case 2:
                if (column == 0)
                    return tr("Buffers");
                else if (column == 1)
                    return tr("SwapCached");
                break;
            case 3:
                if (column == 0)
                    return tr("Active");
                else if (column == 1)
                    return tr("Inactive");
                break;
            case 4:
                if (column == 0)
                    return tr("Dirty");
                else if (column == 1)
                    return tr("Mapped");
                break;
            case 5:
                if (column == 0)
                    return tr("SwapTotal");
                else if (column == 1)
                    return tr("SwapFree");
                break;
            case 6:
                if (column == 0)
                    return tr("Slab");
                break;
            default:
                break;
            }
        } else if (role == Qt::UserRole) {
            switch (row) {
            case 0:
                if (column == 0)
                    return formatUnit((m_memInfo->memTotal() - m_memInfo->memAvailable())  << 10, B, 1);
                else if (column == 1)
                    return formatUnit(m_memInfo->memAvailable() << 10, B, 1);
                break;
            case 1:
                if (column == 0)
                    return formatUnit(m_memInfo->shmem() << 10, B, 1);
                else if (column == 1)
                    return formatUnit(m_memInfo->cached() << 10, B, 1);
                break;
            case 2:
                if (column == 0)
                    return formatUnit(m_memInfo->buffers() << 10, B, 1);
                else if (column == 1)
                    return formatUnit(m_memInfo->swapCached() << 10, B, 1);
                break;
            case 3:
                if (column == 0)
                    return formatUnit(m_memInfo->active() << 10, B, 1);
                else if (column == 1)
                    return formatUnit(m_memInfo->inactive() << 10, B, 1);
                break;
            case 4:
                if (column == 0)
                    return formatUnit(m_memInfo->dirty() << 10, B, 1);
                else if (column == 1)
                    return formatUnit(m_memInfo->mapped() << 10, B, 1);
                break;
            case 5:
                if (column == 0)
                    return formatUnit(m_memInfo->swapTotal() << 10, B, 1);
                else if (column == 1)
                    return formatUnit(m_memInfo->swapFree() << 10, B, 1);
                break;
            case 6:
                if (column == 0)
                    return formatUnit(m_memInfo->slab() << 10, B, 1);
                break;
            default:
                break;
            }
        } else if (role == Qt::TextColorRole) {
            const auto &palette = DApplicationHelper::instance()->applicationPalette();
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
    m_memInfo = DeviceDB::instance()->memInfo();
}

DeailTableModel::~DeailTableModel()
{

}

MemSummaryViewWidget::MemSummaryViewWidget(QWidget *parent)
    : DTableView(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);
    this->horizontalHeader()->setSectionResizeMode(DHeaderView::Stretch);
    this->verticalHeader()->setSectionResizeMode(DHeaderView::Stretch);
    this->setGridStyle(Qt::NoPen);
    this->setFrameShape(QFrame::NoFrame);

    m_model = new DeailTableModel(this);
    this->setModel(m_model);
    this->setItemDelegate(new DetailItemDelegate(this));
}

void MemSummaryViewWidget::fontChanged(const QFont &font)
{
    m_font = font;
    this->setFont(m_font);
    setFixedHeight(QFontMetrics(font).height() * 11);
}

void MemSummaryViewWidget::paintEvent(QPaintEvent *event)
{
    DTableView::paintEvent(event);

    QPainter painter(this->viewport());
    painter.setRenderHints(QPainter::Antialiasing);
    const auto &palette = DApplicationHelper::instance()->applicationPalette();
    painter.setPen(palette.color(DPalette::TextTips));
    painter.setOpacity(0.2);
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(this->horizontalHeader()->sectionSize(0) - 1, 1, this->horizontalHeader()->sectionSize(0) - 1, this->viewport()->height() - 1);
    painter.drawRoundedRect(this->rect().adjusted(1, 1, -1, -1), 8, 8);
}

