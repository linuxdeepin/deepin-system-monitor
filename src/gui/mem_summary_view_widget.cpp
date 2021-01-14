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

#include <QHeaderView>
#include <DApplicationHelper>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <DApplication>
#include <DStyle>

class DetailItemDelegate : public QStyledItemDelegate
{
public:
    explicit DetailItemDelegate(QObject *parent = nullptr): QStyledItemDelegate(parent)
    {

    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const
    {
        QStyledItemDelegate::paint(painter, option, index);

        QBrush background;
        auto palette = option.palette;
        if (!(index.row() & 1)) {
            background = palette.color(DPalette::Active, DPalette::AlternateBase);
        } else {
            background = palette.color(DPalette::Active, DPalette::Base);
        }
        painter->fillRect(option.rect, background);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(std::max(36, size.height()));
        return size;
    }
};

class DeailTableModel : public QAbstractTableModel
{
public:
    explicit DeailTableModel(QObject *parent = nullptr): QAbstractTableModel(parent)
    {

    }

    virtual ~DeailTableModel() {};

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

        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &) const
    {
        return Qt::NoItemFlags;
    }
};

MemSummaryViewWidget::MemSummaryViewWidget(QWidget *parent)
    : DTableView(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(252);

    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);
    this->horizontalHeader()->setSectionResizeMode(DHeaderView::Stretch);
    this->setGridStyle(Qt::NoPen);
    this->setFrameShape(QFrame::NoFrame);

    this->setModel(new DeailTableModel(this));
    this->setItemDelegate(new DetailItemDelegate(this));
}

void MemSummaryViewWidget::fontChanged(const QFont &font)
{

}

void MemSummaryViewWidget::paintEvent(QPaintEvent *event)
{
    DTableView::paintEvent(event);
}

