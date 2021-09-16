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
#include "netif_summary_view_widget.h"
#include "system/device_db.h"
#include "common/common.h"
#include "system/netif.h"
#include "common/thread_manager.h"
#include "system/system_monitor_thread.h"
#include "system/netif_info_db.h"

#include <QHeaderView>
#include <QAbstractTableModel>
#include <QPainter>
#include <DApplicationHelper>
#include <QStyledItemDelegate>
#include <QHeaderView>
#include <DStyle>
#include <DApplication>
#include <QScroller>
#include <QPainterPath>

using namespace core::system;
using namespace common::format;

using namespace common::core;

#define LEFTMARGIN  15  // 左边距
#define TOPMARGIN   10  // 上边距
#define TEXTSPACING 30  // 内容文字间距

Q_DECLARE_METATYPE(ShowInfo)
class NetInfoDetailItemDelegate : public QStyledItemDelegate
{
public:
    explicit NetInfoDetailItemDelegate(QObject *parent = nullptr);
    virtual ~NetInfoDetailItemDelegate();

    void setFont(const QFont &font)
    {
        m_font = font;
    }
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
            QRect textRect = option.rect;
            textRect.setX(textRect.x() + LEFTMARGIN);

            QPen forground;
            forground.setColor(palette.color(DPalette::Active, DPalette::Text));
            painter->setPen(forground);

            ShowInfo stInfo = index.data(Qt::UserRole).value<ShowInfo>();

            if (index.column() == 1 && stInfo.eType != ShowInfo::Normal) {
                // 绘制第2列IPV
                QStringList listKey;
                QStringList listValue = stInfo.strValue.split("/");

                if (stInfo.eType == ShowInfo::IPV4) {
                    listKey << QApplication::translate("NetInfoDetailItemDelegate", "IP address:") << QApplication::translate("NetInfoDetailItemDelegate", "Netmask:") << QApplication::translate("NetInfoDetailItemDelegate", "Broadcast:");
                } else {
                    listKey << QApplication::translate("NetInfoDetailItemDelegate", "IP address:") << QApplication::translate("NetInfoDetailItemDelegate", "Prefixlen:") << QApplication::translate("NetInfoDetailItemDelegate", "Scope:");
                }

                if ((listKey.count() == listValue.count()) && (listValue.count() == 3)) {
                    // 获取key最宽的数值
                    QFontMetrics fm(painter->font());
                    int iMaxW = fm.width(listKey[0]);
                    for (int i = 1; i < listKey.count(); ++i) {
                        if (iMaxW < fm.width(listKey[i]))
                            iMaxW = fm.width(listKey[i]);
                    }

                    // 绘制内容
                    for (int i = 0; i < listKey.count(); ++i) {
                        // 绘制IPV标题
                        QRect titleRect;
                        titleRect.setX(textRect.x());
                        titleRect.setY(textRect.y() + TOPMARGIN + i * fm.height());
                        titleRect.setWidth(fm.width(listKey[i]));
                        titleRect.setHeight(fm.height());
                        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, listKey[i]);

                        // 绘制IP
                        QRect valueRect;
                        valueRect.setX(textRect.x() + iMaxW + TEXTSPACING);
                        valueRect.setY(textRect.y() + TOPMARGIN + i * fm.height());
                        valueRect.setWidth(fm.width(listValue[i]));
                        valueRect.setHeight(fm.height());
                        painter->drawText(valueRect, Qt::AlignLeft | Qt::AlignVCenter, listValue[i]);
                    }
                }

            } else if (index.column() == 0 && stInfo.eType != ShowInfo::Normal) {
                // 绘制第1列IPV
                textRect.setY(textRect.y() + TOPMARGIN);
                painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, stInfo.strKey);
            } else {
                // 其余左对齐、垂直居中
                painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, index.data(Qt::DisplayRole).toString());
            }

        }
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &index) const
    {
        const int item_width = 230;

        if (index.isValid()) {
            ShowInfo stInfo = index.data(Qt::UserRole).value<ShowInfo>();

            if (stInfo.eType != ShowInfo::Normal)
                return QSize(item_width, QFontMetrics(m_font).height() * 3 + 2 * TOPMARGIN);

            return QSize(item_width, 36);
        }

        return QSize(item_width, 36);
    }

private:
    QFont  m_font;
};

NetInfoDetailItemDelegate::NetInfoDetailItemDelegate(QObject *parent): QStyledItemDelegate(parent)
{
}

NetInfoDetailItemDelegate::~NetInfoDetailItemDelegate()
{
}

class NetInfoModel : public QAbstractTableModel
{
public:
    explicit NetInfoModel(QObject *parent = nullptr);
    virtual ~NetInfoModel();

public:
    /**
     * @brief refreshNetifInfo  刷新网络信息
     * @param stNetifInfo       网络信息
     */
    void refreshNetifInfo(const QString &strKey)
    {
        const QMap<QByteArray, NetifInfoPtr> &mapInfo = DeviceDB::instance()->netifInfoDB()->infoDB();

        m_listInfo.clear();

        beginResetModel();
        if (mapInfo.find(strKey.toLocal8Bit()) != mapInfo.end()) {
            NetifInfoPtr stNetifInfo = mapInfo[strKey.toLocal8Bit()];

            ShowInfo stInfo;

            // 处理IPV4
            QList<INet4Addr> listAddr4 = stNetifInfo->addr4InfoList();
            const int ipv4_addr_totle = listAddr4.count();
            for (int i = 0; i < ipv4_addr_totle; ++i) {
                stInfo.eType = ShowInfo::IPV4;
                if (1 == ipv4_addr_totle) {
                    stInfo.strKey = QApplication::translate("NetInfoModel", "IPv4");
                } else {
                    stInfo.strKey = QApplication::translate("NetInfoModel", "IPv4") + " " + QString::number(i + 1);
                }

                stInfo.strValue =  QString("%1/%2/%3").arg(QString(listAddr4[i]->addr)).arg(QString(listAddr4[i]->mask)).arg(QString(listAddr4[i]->bcast));
                m_listInfo << stInfo;
            }

            // 处理IPV6
            QList<INet6Addr> listAddr6 = stNetifInfo->addr6InfoList();
            const int ipv6_addr_totle = listAddr6.count();
            for (int i = 0; i < ipv6_addr_totle; ++i) {
                stInfo.eType = ShowInfo::IPV6;
                if (1 == ipv6_addr_totle) {
                    stInfo.strKey = QApplication::translate("NetInfoModel", "IPv6");
                } else {
                    stInfo.strKey = QApplication::translate("NetInfoModel", "IPv6") + " " + QString::number(i + 1);
                }

                stInfo.strValue = QString("%1/%2/%3").arg(QString(listAddr6[i]->addr)).arg(listAddr6[i]->prefixlen).arg(listAddr6[i]->scope);
                m_listInfo << stInfo;
            }

            // 处理连接类型
            stInfo.eType = ShowInfo::Normal;
            stInfo.strKey = QApplication::translate("NetInfoModel", "Connection type");
            stInfo.strValue = stNetifInfo->connectionType();
            m_listInfo << stInfo;

            // 是否是无线网
            if (stNetifInfo->isWireless()) {
                // 服务器别号
                if (!stNetifInfo->essid().isEmpty()) {
                    stInfo.strKey = QApplication::translate("NetInfoModel", "ESSID");
                    stInfo.strValue = stNetifInfo->essid();
                    m_listInfo << stInfo;
                }

                // 信号质量
                stInfo.strKey = QApplication::translate("NetInfoModel", "Link quality");
                stInfo.strValue = QString("%1").arg(stNetifInfo->linkQuality());
                m_listInfo << stInfo;

                // 信号强度
                stInfo.strKey = QApplication::translate("NetInfoModel", "Signal strength");
                stInfo.strValue = QString("%1 dBm").arg(stNetifInfo->signalLevel());
                m_listInfo << stInfo;

                // 底噪
                stInfo.strKey = QApplication::translate("NetInfoModel", "Noise level");
                stInfo.strValue = QString("%1 dB").arg(stNetifInfo->noiseLevel());
                m_listInfo << stInfo;
            }

            // Mac地址
            stInfo.strKey = QApplication::translate("NetInfoModel", "MAC");
            stInfo.strValue = stNetifInfo->linkAddress();
            m_listInfo << stInfo;

            // 速率
            stInfo.strKey = QApplication::translate("NetInfoModel", "Bandwidth");
            stInfo.strValue = formatUnit(stNetifInfo->speed(), MB, 0, true);
            m_listInfo << stInfo;

            // 接收包数量
            stInfo.strKey = QApplication::translate("NetInfoModel", "RX packets");
            stInfo.strValue = QString("%1").arg(stNetifInfo->rxPackets());
            m_listInfo << stInfo;

            // 总计接收
            stInfo.strKey = QApplication::translate("NetInfoModel", "RX bytes");
            stInfo.strValue = formatUnit(stNetifInfo->rxBytes(), B, 1);
            m_listInfo << stInfo;

            // 接收错误包
            stInfo.strKey = QApplication::translate("NetInfoModel", "RX errors");
            stInfo.strValue = QString("%1").arg(stNetifInfo->rxErrors());
            m_listInfo << stInfo;

            // 接收丢包数
            stInfo.strKey = QApplication::translate("NetInfoModel", "RX dropped");
            stInfo.strValue = QString("%1").arg(stNetifInfo->rxDropped());
            m_listInfo << stInfo;

            // 接收FIFO
            stInfo.strKey = QApplication::translate("NetInfoModel", "RX overruns");
            stInfo.strValue = QString("%1").arg(stNetifInfo->rxFIFO());
            m_listInfo << stInfo;

            // 分组帧错误
            stInfo.strKey = QApplication::translate("NetInfoModel", "RX frame");
            stInfo.strValue = QString("%1").arg(stNetifInfo->rxFrame());
            m_listInfo << stInfo;

            // 发送包数量
            stInfo.strKey = QApplication::translate("NetInfoModel", "TX packets");
            stInfo.strValue = QString("%1").arg(stNetifInfo->txPackets());
            m_listInfo << stInfo;

            // 总计发送
            stInfo.strKey = QApplication::translate("NetInfoModel", "TX bytes");
            stInfo.strValue = formatUnit(stNetifInfo->txBytes(), B, 1);
            m_listInfo << stInfo;

            // 发送错误包
            stInfo.strKey = QApplication::translate("NetInfoModel", "TX errors");
            stInfo.strValue = QString("%1").arg(stNetifInfo->txErrors());
            m_listInfo << stInfo;

            // 发送丢包数
            stInfo.strKey = QApplication::translate("NetInfoModel", "TX dropped");
            stInfo.strValue = QString("%1").arg(stNetifInfo->txDropped());
            m_listInfo << stInfo;

            // 发送FIFO
            stInfo.strKey = QApplication::translate("NetInfoModel", "TX overruns");
            stInfo.strValue = QString("%1").arg(stNetifInfo->txFIFO());
            m_listInfo << stInfo;

            // 载波损耗
            stInfo.strKey = QApplication::translate("NetInfoModel", "TX carrier");
            stInfo.strValue = QString("%1").arg(stNetifInfo->txCarrier());
            m_listInfo << stInfo;
        }
        endResetModel();
    }

private:
    QList<ShowInfo> m_listInfo;

protected:
    int rowCount(const QModelIndex &) const
    {
        return m_listInfo.count();
    }

    int columnCount(const QModelIndex &) const
    {
        return 2;
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        int iRow = index.row();
        int iColumn = index.column();

        if (iRow >= m_listInfo.count() || iRow < 0)
            return QVariant();

        if (role == Qt::UserRole) {
            return QVariant::fromValue(m_listInfo[iRow]);
        } else if (role == Qt::DisplayRole) {
            switch (iColumn) {
            case 0:
                return m_listInfo[iRow].strKey;
            case 1:
                return m_listInfo[iRow].strValue;
            default:
                break;
            }
        }
        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &) const
    {
        return Qt::NoItemFlags;
    }
};

NetInfoModel::NetInfoModel(QObject *parent): QAbstractTableModel(parent)
{
}

NetInfoModel::~NetInfoModel()
{

}

NetifSummaryViewWidget::NetifSummaryViewWidget(QWidget *parent)
    : DTableView(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);

    this->horizontalHeader()->setStretchLastSection(true);

    this->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    this->setGridStyle(Qt::NoPen);
    this->setFrameShape(QFrame::NoFrame);

    m_netInfoModel = new NetInfoModel(this);
    this->setModel(m_netInfoModel);
    m_netInfoDetailItemDelegate = new NetInfoDetailItemDelegate(this);
    this->setItemDelegate(dynamic_cast<QAbstractItemDelegate *>(m_netInfoDetailItemDelegate));

    auto *scroller = QScroller::scroller(viewport());
    auto prop = scroller->scrollerProperties();
    // turn off overshoot to fix performance issue
    prop.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0);
    prop.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 1);
    // lock scroll direction to fix performance issue
    prop.setScrollMetric(QScrollerProperties::AxisLockThreshold, 1);
    scroller->setScrollerProperties(prop);
    // enable touch gesture
    QScroller::grabGesture(viewport(), QScroller::TouchGesture);
}

void NetifSummaryViewWidget::onNetifItemClicked(const QString &mac)
{
    // 按照点击IP刷新数据
    m_strCurrentKey = mac;
    m_netInfoModel->refreshNetifInfo(m_strCurrentKey);
}

void NetifSummaryViewWidget::fontChanged(const QFont &font)
{
    m_font = font;
    this->setFont(m_font);;
    m_netInfoDetailItemDelegate->setFont(m_font);
    setFixedHeight(260);
}

void NetifSummaryViewWidget::onModelUpdate()
{
    m_netInfoModel->refreshNetifInfo(m_strCurrentKey);  // 按时间间隔刷新数据
}

void NetifSummaryViewWidget::paintEvent(QPaintEvent *event)
{
    DTableView::paintEvent(event);

    QPainter painter(this->viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);
    const auto &palette = DApplicationHelper::instance()->applicationPalette();
    QColor frameColor = palette.color(DPalette::TextTips);
    frameColor.setAlphaF(0.3);

    painter.setPen(QPen(frameColor, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(this->horizontalHeader()->sectionSize(0) - 1, 2, this->horizontalHeader()->sectionSize(0) - 1, this->viewport()->height() - 2);
    painter.drawRoundedRect(this->rect().adjusted(1, 1, -1, -1), 6, 6);
}


