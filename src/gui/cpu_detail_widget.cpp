/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      zhangsong<zhangsong@uniontech.com>
* Maintainer:  zhangsong<zhangsong@uniontech.com>
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
#include "cpu_detail_widget.h"
#include "model/cpu_info_model.h"
#include "smooth_curve_generator.h"
#include "common/common.h"
#include "process/process_db.h"
#include "system/device_db.h"
#include "model/cpu_info_model.h"
#include "model/cpu_list_model.h"
#include "system/cpu_set.h"
#include "cpu_summary_view_widget.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyleHelper>
#include <DLabel>
#include <DFontSizeManager>

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QMouseEvent>
#include <QGridLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QPaintEvent>

DWIDGET_USE_NAMESPACE

using namespace common;

CPUDetailGrapTableItem::CPUDetailGrapTableItem(CPUInfoModel *model, int index, QWidget *parent): QWidget(parent), m_cpuInfomodel(model), m_index(index)
{
    TimePeriod period(TimePeriod::kNoPeriod, {2, 0});
    m_cpuInfomodel = new CPUInfoModel(period, this);
    connect(m_cpuInfomodel, &CPUInfoModel::modelUpdated, this, &CPUDetailGrapTableItem::updateStat);
}

CPUDetailGrapTableItem::~CPUDetailGrapTableItem()
{

}

void CPUDetailGrapTableItem::setMode(int mode)
{
    m_mode = mode;
    setToolTip(3 == m_mode ? ("CPU" + QString::number(m_index)) : "");
    update();
}

void CPUDetailGrapTableItem::sethorizontal(bool isHorizontalLast)
{
    m_isHorizontalLast = isHorizontalLast;
}

void CPUDetailGrapTableItem::setVerticalLast(bool isVerticalLast)
{
    m_isVerticalLast = isVerticalLast;
}

void CPUDetailGrapTableItem::setColor(QColor color)
{
    m_color = color;
    update();
}

void CPUDetailGrapTableItem::updateStat()
{
    m_cpuPercents.insert(0, m_cpuInfomodel->cpuPercentList().value(m_index) / 100.0);

    while (m_cpuPercents.count() > 31)
        m_cpuPercents.pop_back();

    update();
}

void CPUDetailGrapTableItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);

    if (1 == m_mode)
        drawNormalMode(painter);
    else if (2 == m_mode)
        drawSimpleMode(painter);
    else if (3 == m_mode)
        drawTextMode(painter);
}

void CPUDetailGrapTableItem::drawNormalMode(QPainter &painter)
{
    //draw background
    QRect graphicRect = QRect(5, 30, this->width() - 10, this->height() - 60);
    drawBackground(painter, graphicRect);

    //draw text
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    painter.setPen(palette.color(DPalette::TextTips));

    QFont font = DApplication::font();
    font.setPointSize(font.pointSize() - 2);
    painter.setFont(font);

    painter.drawText(QRect(5, 5, this->width() - 10, 25), Qt::AlignLeft | Qt::AlignBottom, "CPU" + QString::number(m_index));
    painter.drawText(QRect(5, 5, this->width() - 10, 25), Qt::AlignRight | Qt::AlignBottom, "100%");
    painter.drawText(QRect(5, this->height() - 30, this->width() - 10, 30), Qt::AlignTop | Qt::AlignLeft, tr("60 seconds"));
    painter.drawText(QRect(5, this->height() - 30, this->width() - 10, 30), Qt::AlignTop | Qt::AlignRight, "0");

    // draw cpu
    if (m_cpuPercents.count() > 0) {
        painter.setPen(QPen(m_color, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        QPainterPath Painterpath;

        QPointF sp = QPointF(graphicRect.width() + graphicRect.x(), (1.0 - m_cpuPercents.value(0)) * graphicRect.height() + graphicRect.y());
        Painterpath.moveTo(sp);

        for (int i = 0; i < 30; ++i) {
            if (m_cpuPercents.count() > i) {
                QPointF ep = QPointF((graphicRect.width() - static_cast<double>(graphicRect.width()) / (30.0 / static_cast<double>(i + 1))) + graphicRect.x(), (1.0 - m_cpuPercents.value(i + 1)) * graphicRect.height() + graphicRect.y());
                QPointF c1 = QPointF((sp.x() + ep.x()) / 2, sp.y());
                QPointF c2 = QPointF((sp.x() + ep.x()) / 2, ep.y());
                Painterpath.cubicTo(c1, c2, ep);
                sp = ep;
            }
        }

        painter.drawPath(Painterpath);
    }
}

void CPUDetailGrapTableItem::drawSimpleMode(QPainter &painter)
{
    //draw background
    QRect graphicRect = QRect(1, 1, this->width() - 2, this->height() - 2);
    drawBackground(painter, graphicRect);

    // draw cpu
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_cpuPercents.count() > 0) {
        painter.setPen(QPen(m_color, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        QPainterPath Painterpath;

        QPointF sp = QPointF(graphicRect.width() + graphicRect.x(), (1.0 - m_cpuPercents.value(0)) * graphicRect.height() + graphicRect.y());
        Painterpath.moveTo(sp);

        for (int i = 0; i < 30; ++i) {
            if (m_cpuPercents.count() > i) {
                QPointF ep = QPointF((graphicRect.width() - static_cast<double>(graphicRect.width()) / (30.0 / static_cast<double>(i + 1))) + graphicRect.x(), (1.0 - m_cpuPercents.value(i + 1)) * graphicRect.height() + graphicRect.y());
                QPointF c1 = QPointF((sp.x() + ep.x()) / 2, sp.y());
                QPointF c2 = QPointF((sp.x() + ep.x()) / 2, ep.y());
                Painterpath.cubicTo(c1, c2, ep);
                sp = ep;
            }
        }

        painter.drawPath(Painterpath);
    }
}

void CPUDetailGrapTableItem::drawTextMode(QPainter &painter)
{
    QRect rect = QRect(1, 1, this->width() - (m_isHorizontalLast ? 2 : 0), this->height() - (m_isVerticalLast ? 2 : 0));

    // draw frame
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    QColor frameColor = palette.color(DPalette::FrameBorder);
    painter.setPen(frameColor);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect);

    painter.setPen(m_color);
    painter.drawText(rect, Qt::AlignCenter, QString::number(m_cpuPercents.value(m_index) * 100, 'f', 1) + "%");
}

void CPUDetailGrapTableItem::drawBackground(QPainter &painter, const QRect &graphicRect)
{
    // draw frame
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
    QColor frameColor = palette.color(DPalette::TextTips);
    frameColor.setAlphaF(0.3);

    painter.setPen(QPen(frameColor, 1));
    painter.setBrush(palette.color(QPalette::Base));
    painter.drawRect(graphicRect);

    // draw grid
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 2;
    dashes << space << space;
    gridPen.setDashPattern(dashes);
    gridPen.setColor(frameColor);
    painter.setPen(gridPen);
    int section = 10;

    int totalHeight = graphicRect.height();
    int currentHeight = graphicRect.y();
    while (currentHeight < totalHeight + graphicRect.y()) {
        painter.drawLine(graphicRect.x(), currentHeight, graphicRect.x() + graphicRect.width(), currentHeight);
        currentHeight += section;
    }

    int totalWidth = graphicRect.width();
    int currentWidth = graphicRect.x();
    while (currentWidth < totalWidth + graphicRect.x()) {
        painter.drawLine(currentWidth, graphicRect.y(), currentWidth, graphicRect.y() + graphicRect.height());
        currentWidth += section;
    }
}

CPUDetailWidget::CPUDetailWidget(QWidget *parent) : BaseDetailViewWidget(parent)
{
    TimePeriod period(TimePeriod::kNoPeriod, {2, 0});

    m_cpuInfomodel = new CPUInfoModel(period, this);

    m_graphicsTable = new CPUDetailGrapTable(m_cpuInfomodel, this);

    m_summary  = new  CPUDetailSummaryTable(m_cpuInfomodel, this);

    setTitle(DApplication::translate("Process.Graph.View", "CPU"));

    setDetail(m_cpuInfomodel->cpuSet()->modelName());
}

void CPUDetailWidget::resizeEvent(QResizeEvent *event)
{
    adjustGeometry(event->size());
    BaseDetailViewWidget::resizeEvent(event);
}

void CPUDetailWidget::adjustGeometry(QSize size)
{
    m_graphicsTable->move(5, titleHeight());
    m_graphicsTable->resize(size.width() - 10, size.height() / 2 - titleHeight());
    m_summary->move(10, size.height() / 2);
    m_summary->resize(size.width() - 20, size.height() / 2);
}

void CPUDetailWidget::detailFontChanged(const QFont &font)
{
    BaseDetailViewWidget::detailFontChanged(font);
    adjustGeometry(this->size());
}

CPUDetailGrapTable::CPUDetailGrapTable(CPUInfoModel *model, QWidget *parent): QWidget(parent)
{
    QGridLayout  *graphicsLayout = new QGridLayout(this);

    int cpuCount = int(sysconf(_SC_NPROCESSORS_ONLN));

    QList<QColor> cpuColors;
    cpuColors << "#1094D8"
              << "#F7B300"
              << "#55D500"
              << "#C362FF"
              << "#FF2997"
              << "#00B4C7"
              << "#F8E71C"
              << "#FB1818"
              << "#8544FF"
              << "#00D7AB"
              << "#00D7AB"
              << "#FF00FF"
              << "#30BF03"
              << "#7E41F1"
              << "#2CA7F8"
              << "#A005CE";

    if (1 == cpuCount) {
        CPUDetailGrapTableItem *item = new CPUDetailGrapTableItem(model, 0, this);
        item->setMode(1);
        item->setColor(cpuColors[0]);
        graphicsLayout->addWidget(item, 0, 0);
    } else if (2 == cpuCount) {
        for (int i = 0; i < cpuCount; ++i) {
            CPUDetailGrapTableItem *item = new CPUDetailGrapTableItem(model, i, this);
            item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            item->setMode(1);
            item->setColor(cpuColors[i % cpuColors.size()]);
            graphicsLayout->addWidget(item, i / 2, i % 2);
        }
    } else if (4 == cpuCount) {
        for (int i = 0; i < cpuCount; ++i) {
            CPUDetailGrapTableItem *item = new CPUDetailGrapTableItem(model, i, this);
            item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            item->setMode(1);
            item->setColor(cpuColors[i % cpuColors.size()]);
            graphicsLayout->addWidget(item, i / 2, i % 2);
        }
    } else if (8 == cpuCount) {
        for (int i = 0; i < cpuCount; ++i) {
            CPUDetailGrapTableItem *item = new CPUDetailGrapTableItem(model, i, this);
            item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            item->setMode(1);
            item->setColor(cpuColors[i % cpuColors.size()]);
            graphicsLayout->addWidget(item, i / 4, i % 4);
        }
    } else if (16 == cpuCount) {
        for (int i = 0; i < cpuCount; ++i) {
            CPUDetailGrapTableItem *item = new CPUDetailGrapTableItem(model, i, this);
            item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            item->setMode(2);
            item->setColor(cpuColors[i % cpuColors.size()]);
            graphicsLayout->addWidget(item, i / 4, i % 4);
        }
        graphicsLayout->setSpacing(10);
    } else if (32 == cpuCount) {//8*4
        for (int i = 0; i < cpuCount; ++i) {
            CPUDetailGrapTableItem *item = new CPUDetailGrapTableItem(model, i, this);
            item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            item->setMode(2);
            item->setColor(cpuColors[i % cpuColors.size()]);
            graphicsLayout->addWidget(item, i / 8, i % 8);
        }
        graphicsLayout->setSpacing(5);
    } else if (32 < cpuCount) {
        for (int i = 0; i < cpuCount; ++i) {
            CPUDetailGrapTableItem *item = new CPUDetailGrapTableItem(model, i, this);
            item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            item->setMode(3);
            item->setColor(cpuColors[i % cpuColors.size()]);
            if ((i % 8) == 7)
                item->sethorizontal(true);
            if ((i / 8) == (cpuCount / 8 - 1))
                item->setVerticalLast(true);
            graphicsLayout->addWidget(item, i / 8, i % 8);
        }
        graphicsLayout->setSpacing(0);
    } else {
        //模式2
        for (int i = 0; i < cpuCount; ++i) {
            CPUDetailGrapTableItem *item = new CPUDetailGrapTableItem(model, i, this);
            item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            item->setMode(2);
            item->setColor(cpuColors[i % cpuColors.size()]);
            graphicsLayout->addWidget(item, i / 16, i % 16);
        }
        graphicsLayout->setSpacing(5);
    }

    setLayout(graphicsLayout);
}
