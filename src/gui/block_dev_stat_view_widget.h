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
#ifndef BLOCK_DEV_STAT_VIEW_WIDGET_H
#define BLOCK_DEV_STAT_VIEW_WIDGET_H

#include <QMap>
#include <QScrollArea>
#include "system/block_device.h"

using namespace core::system;

class BlockDevItemWidget;
class BlockStatViewWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit BlockStatViewWidget(QWidget *parent = nullptr);

signals:
    void changeInfo(const QString &deviceName);

protected:
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void updateWidgetGeometry();
    void onUpdateData();
    void fontChanged(const QFont &font);
    void onSetItemStatus(const QString &deviceName);

private:
    void showItem1();
    void showItem2();
    void showItemLg2(int count);
    void resetMapInfo();

private:
    QList<BlockDevice> m_listDevice;
    QList<BlockDevItemWidget *> m_listBlockItemWidget;

    QMap<QString, BlockDevItemWidget *> m_mapDeviceItemWidget;

    QWidget *m_centralWidget;
    QFont m_font;
};

#endif // BLOCK_DEV_STAT_VIEW_WIDGET_H
