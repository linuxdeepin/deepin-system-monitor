/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     leiyu <leiyu@uniontech.com>
*
* Maintainer: leiyu <leiyu@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef MEM_STAT_VIEW_WIDGET_H
#define MEM_STAT_VIEW_WIDGET_H

#include <QWidget>
#include <DCommandLinkButton>

namespace core {
namespace system {
class MemInfo;
}
}

DWIDGET_USE_NAMESPACE
class ChartViewWidget;
class MemStatViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MemStatViewWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

public slots:
    void fontChanged(const QFont &font);
    void onModelUpdate();

private:
    void updateWidgetGeometry();

private:
    QColor memoryColor {"#00C5C0"};
    QColor swapColor {"#FEDF19"};

    ChartViewWidget *m_memChartWidget;
    ChartViewWidget *m_swapChartWidget;

    core::system::MemInfo *m_memInfo;

    QFont m_font;
};

#endif // MEM_STAT_VIEW_WIDGET_H
