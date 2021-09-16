/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     tenghuan <tenghuan@uniontech.com>
*
* Maintainer: tenghuan <tenghuan@uniontech.com>
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
#ifndef BLOCK_DEV_ITEM_WIDGET_H
#define BLOCK_DEV_ITEM_WIDGET_H

#include <QObject>
#include <QWidget>
#include "system/block_device.h"

using namespace core::system;
class ChartViewWidget;
class BlockDevInfoModel;
class QGridLayout;
class BlockDevItemWidget : public QWidget
{
    Q_OBJECT
public:
    enum TitleStyle {
        TITLE_HORIZONTAL = 0,// 单个模式
        TITLE_VERTICAL = 1   // 多个模式
    };

    explicit BlockDevItemWidget(QWidget *parent = nullptr);

signals:
    void clicked(const QString &deviceName);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

public slots:
    void fontChanged(const QFont &font);
    void activeItemWidget(bool isShow);

public:
    void updateData(const BlockDevice &info);
    void setMode(int mode);
    bool isActiveItem() { return  m_isActive;}

private:
    void updateWidgetGeometry();

private:
    int m_mode = TITLE_HORIZONTAL;
    QColor readColor {"#8F88FF"};
    QColor writeColor {"#6AD787"};
    ChartViewWidget *m_memChartWidget;

    QFont m_font;
    BlockDevice  m_blokeDeviceInfo;
    QList<qreal> m_listWriteSpeed;
    QList<qreal> m_listReadSpeed;
    bool m_isActive = false;
};

#endif // BLOCK_DEV_ITEM_WIDGET_H
