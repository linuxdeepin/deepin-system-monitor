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
#include "block_dev_summary_view_widget.h"
#include "system/device_db.h"
#include "common/common.h"
#include "system/block_device_info_db.h"
#include "common/thread_manager.h"
#include "system/block_device.h"
#include "common/thread_manager.h"
#include "system/system_monitor_thread.h"
#include "system/block_device_info_db.h"
#include "system/device_db.h"
#include <QHeaderView>
#include <DApplicationHelper>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <DApplication>
#include <DStyle>
using namespace core::system;
using namespace common::format;


class DetailItemDelegateBlock : public QStyledItemDelegate
{
public:
    explicit DetailItemDelegateBlock(QObject *parent = nullptr);
    virtual ~DetailItemDelegateBlock();

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
DetailItemDelegateBlock::DetailItemDelegateBlock(QObject *parent): QStyledItemDelegate(parent)
{

}

DetailItemDelegateBlock::~DetailItemDelegateBlock()
{

}

class DeailTableModelBlock : public QAbstractTableModel {
public:
    explicit DeailTableModelBlock(QObject *parent = nullptr);
    virtual ~DeailTableModelBlock();
protected:
    int rowCount(const QModelIndex &) const
    {
        return 7;
    }

    int columnCount(const QModelIndex &) const
    {
        return 2;
    }

    QVariant data(const QModelIndex &index, int role) const {

        if (!index.isValid() || m_infoDB.size()<=0 )
            return QVariant();
        int row = index.row();
        int column = index.column();
        if (role == Qt::DisplayRole) {
            switch (row) {
             case 0:
                if (column == 0)
                    return tr("Model");
                else if (column == 1)
                    return tr("Reading speed");
                break;
            case 1:
                if (column == 0)
                    return tr("Write speed");
                else if (column == 1)
                    return tr("Capacity");
                break;
            case 2:
                if (column == 0)
                    return tr("Num of reads sectors");
                else if (column == 1)
                    return tr("r/s");
                break;
            case 3:
                if (column == 0)
                    return tr("rsec/s");
                else if (column == 1)
                    return tr("rrqm/s");
                break;
            case 4:
                if (column == 0)
                    return tr("Num of write sectors");
                else if (column == 1)
                    return tr("Num of write completions");
                break;
            case 5:
                if (column == 0)
                    return tr("Num of merge write completions");
                else if (column == 1)
                    return tr("w/s");
                break;
            case 6:
                if (column == 0)
                    return tr("wsec/s");
                else if (column == 1)
                    return tr("wrqm/s");
                break;

            }
        }else if (role == Qt::UserRole) {
            switch (row) {
            case 0:
                if (column == 0)
                    return m_infoDB[m_index].model();
                else if (column == 1)
                    return formatUnit(m_infoDB[m_index].readSpeed(), B, 1);
                break;
            case 1:
                if (column == 0)
                    return formatUnit(m_infoDB[m_index].writeSpeed(), B, 1);
                else if (column == 1)
                    return formatUnit(m_infoDB[m_index].capacity(), B, 1);
                break;
            case  2:
                if (column == 0)
                    return m_infoDB[m_index].blocksRead();
                else if(column == 1)
                    return m_infoDB[m_index].readRequestIssuedPerSecond();
                break;
            case  3:
                if (column == 0)
                    return m_infoDB[m_index].sectorsReadPerSecond();
                else if(column == 1)
                    return m_infoDB[m_index].readRequestMergedPerSecond();
                break;
            case  4:
                if (column == 0)
                    return m_infoDB[m_index].blocksWritten();
                else if(column == 1)
                    return m_infoDB[m_index].writeMerged();
                break;
            case  5:
                if (column == 0)
                    return m_infoDB[m_index].readMerged();
                else if(column == 1)
                    return m_infoDB[m_index].writeRequestIssuedPerSecond();
                break;
            case  6:
                if (column == 0)
                    return m_infoDB[m_index].sectorsWrittenPerSecond();
                else if(column == 1)
                    return m_infoDB[m_index].writeRequestMergedPerSecond();
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
public slots:
    void updateModel();
private:
    int m_index {0};
    QList<BlockDevice> m_infoDB;
};

DeailTableModelBlock::DeailTableModelBlock(QObject *parent): QAbstractTableModel(parent)
{
    m_index = 0;
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    connect(monitor, &SystemMonitor::statInfoUpdated, this, &DeailTableModelBlock::updateModel);

   // m_infoDB ={};
  //  m_memInfo = DeviceDB::instance()->memInfo();
}

void DeailTableModelBlock::updateModel()
{
    beginResetModel();
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    m_infoDB = monitor->deviceDB()->blockDeviceInfoDB()->deviceList();
    endResetModel();
}

DeailTableModelBlock::~DeailTableModelBlock()
{

}

BlockDevSummaryViewWidget::BlockDevSummaryViewWidget(QWidget *parent)
    : DTableView(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);

    this->horizontalHeader()->setSectionResizeMode(DHeaderView::Stretch);
    this->verticalHeader()->setSectionResizeMode(DHeaderView::Stretch);
    this->setGridStyle(Qt::NoPen);
    this->setFrameShape(QFrame::NoFrame);
    m_model = new DeailTableModelBlock(this);
    this->setModel(m_model);
    this->setItemDelegate(new DetailItemDelegateBlock(this));



}
void BlockDevSummaryViewWidget::fontChanged(const QFont &font)
{
    m_font = font;
    this->setFont(m_font);
    setFixedHeight(QFontMetrics(font).height() * 11);
}
void BlockDevSummaryViewWidget::paintEvent(QPaintEvent *event)
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
