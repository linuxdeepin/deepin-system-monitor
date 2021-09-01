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
#ifndef NETIF_SUMMARY_VIEW_WIDGET_H
#define NETIF_SUMMARY_VIEW_WIDGET_H

#include <DTableView>
#include "system/netif.h"
using namespace core::system;
DWIDGET_USE_NAMESPACE
/**
 * @brief Network interface summary view widget
 */

class NetInfoModel;
class NetInfoDetailItemDelegate;
struct ShowInfo {
    enum InfoType {
        Normal,     // 正常数据
        IPV4,       // IPV4
        IPV6        // IPV6
    };

    InfoType eType = Normal;
    QString strKey;     // 第一列内容
    QString strValue;   // 第二列值
//    bool isIPV = false;  // 是否是IPV数据
};
class NetifSummaryViewWidget : public DTableView
{
    Q_OBJECT
public:
    explicit NetifSummaryViewWidget(QWidget *parent = nullptr);
    void updateInfo(const QByteArray &strKey);

protected:
    void paintEvent(QPaintEvent *event) override;

signals:

public slots:
    void fontChanged(const QFont &font);
    void onModelUpdate();

    void onNetifItemClicked(const QString &mac);

private:
    QFont m_font;
    NetInfoModel *m_netInfoModel;
    NetInfoDetailItemDelegate *m_netInfoDetailItemDelegate;
    QString m_strCurrentKey;
};

#endif // NETIF_SUMMARY_VIEW_WIDGET_H
