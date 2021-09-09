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
#ifndef CPU_DETAIL_WIDGET_H
#define CPU_DETAIL_WIDGET_H

#include <QWidget>
#include <DPushButton>
#include <QScrollArea>

#include "base/base_detail_view_widget.h"

class CPUInfoModel;
class QScrollArea;
class CPUDetailGrapTableItem : public QWidget
{
    Q_OBJECT

public:
    explicit CPUDetailGrapTableItem(CPUInfoModel *model, int index, QWidget *parent = nullptr);

    ~CPUDetailGrapTableItem();

    void setMode(int mode);

    void sethorizontal(bool isHorizontalLast);

    void setVerticalLast(bool isVerticalLast);

    void setColor(QColor color);

public slots:
    void updateStat();

protected:
    void paintEvent(QPaintEvent *event);

    void drawNormalMode(QPainter &painter);

    void drawSimpleMode(QPainter &painter);

    void drawTextMode(QPainter &painter);

    /**
     * @brief drawBackground
     * 绘制边框和表格线
     * @param painter
     * @param graphicRect
     */
    void drawBackground(QPainter &painter, const QRect &graphicRect);

private:
    QList<qreal>  m_cpuPercents;
    CPUInfoModel *m_cpuInfomodel = nullptr;
    QColor m_color;
    int m_mode  = 1;        //1:normal 2:simple 3:text
    int m_index = -1;
    bool m_isHorizontalLast = false;
    bool m_isVerticalLast = false;
};

class CPUDetailGrapTable : public QWidget
{
    Q_OBJECT
public:
    CPUDetailGrapTable(CPUInfoModel *model, QWidget *parent);
};

class CPUDetailSummaryTable;
class CPUDetailWidget : public BaseDetailViewWidget
{
    Q_OBJECT

public:
    explicit CPUDetailWidget(QWidget *parent = nullptr);

private slots:
    void detailFontChanged(const QFont &font);

private:
    CPUDetailGrapTable *m_graphicsTable = nullptr;
    CPUDetailSummaryTable *m_summary = nullptr;
};

#endif // CPU_DETAIL_WIDGET_H
