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
#include <QMap>
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
        clipPath.addRoundedRect(option.widget->rect().adjusted(1, 1, -1, -1), 6, 6);
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

        if (!index.isValid() || currDeciveName.isEmpty() )
            return QVariant();
        int row = index.row();
        int column = index.column();
        if (role == Qt::DisplayRole) {
            switch (row) {
             case 0:
                if (column == 0)
                    return tr("Model");
                else if (column == 1)
                    return DApplication::translate("Process.Graph.View", "Disk read");
                break;
            case 1:
                if (column == 0)
                    return DApplication::translate("Process.Graph.View", "Disk write");
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
                    return m_blockInfo.model();
                else if (column == 1)
                    return formatUnit(m_blockInfo.readSpeed(), B, 1);
                break;
            case 1:
                if (column == 0)
                    return formatUnit(m_blockInfo.writeSpeed(), B, 1);
                else if (column == 1)
                    return formatUnit(m_blockInfo.capacity(), B, 1);
                break;
            case  2:
                if (column == 0)
                    return m_blockInfo.blocksRead();
                else if(column == 1)
                    return m_blockInfo.readRequestIssuedPerSecond();
                break;
            case  3:
                if (column == 0)
                    return m_blockInfo.sectorsReadPerSecond();
                else if(column == 1)
                    return m_blockInfo.readRequestMergedPerSecond();
                break;
            case  4:
                if (column == 0)
                    return m_blockInfo.blocksWritten();
                else if(column == 1)
                    return m_blockInfo.writeMerged();
                break;
            case  5:
                if (column == 0)
                    return m_blockInfo.readMerged();
                else if(column == 1)
                    return m_blockInfo.writeRequestIssuedPerSecond();
                break;
            case  6:
                if (column == 0)
                    return m_blockInfo.sectorsWrittenPerSecond();
                else if(column == 1)
                    return m_blockInfo.writeRequestMergedPerSecond();
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
public:
    void setCurrentName(const QString& str) {
        currDeciveName = str;
        if(!str.isEmpty())
            m_blockInfo = m_mapInfo.find(currDeciveName).value();
        updateModel();
    }
private:
    QString currDeciveName;
    BlockDevice m_blockInfo;
    QMap<QString,BlockDevice> m_mapInfo;
};

DeailTableModelBlock::DeailTableModelBlock(QObject *parent): QAbstractTableModel(parent)
{
    currDeciveName = "";
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    connect(monitor, &SystemMonitor::statInfoUpdated, this, &DeailTableModelBlock::updateModel);
    updateModel();
}

void DeailTableModelBlock::updateModel()
{
    beginResetModel();
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    auto infoDB = monitor->deviceDB()->blockDeviceInfoDB()->deviceList();

    for(int i =0; i < infoDB.size(); ++i) {
        m_mapInfo.insert(infoDB[i].deviceName(),infoDB[i]);
    }
    if(currDeciveName == "" && infoDB.size() >=0) {
        currDeciveName = infoDB[0].deviceName();
        m_blockInfo = infoDB[0];
    } else if(infoDB.size() >=0) {
        m_blockInfo = m_mapInfo.find(currDeciveName).value();
    }
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

void BlockDevSummaryViewWidget::chageSummaryInfo(const QString& deviceName)
{
    m_model->setCurrentName(deviceName);
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
    QColor frameColor = palette.color(DPalette::TextTips);
    frameColor.setAlphaF(0.3);
    painter.setPen(QPen(frameColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(this->horizontalHeader()->sectionSize(0) - 1, 2, this->horizontalHeader()->sectionSize(0) - 1, this->viewport()->height() - 2);
    painter.drawRoundedRect(this->rect().adjusted(1, 1, -1, -1), 6, 6);
}
