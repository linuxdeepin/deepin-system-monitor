#ifndef NETIF_ITEM_VIEW_WIDGET_H
#define NETIF_ITEM_VIEW_WIDGET_H

#include <QWidget>
#include <DCommandLinkButton>
#include "system/netif.h"
DWIDGET_USE_NAMESPACE
using namespace core::system;
class ChartViewWidget;
class NetifInfoModel;
class NetifItemViewWidget : public QWidget
{
    Q_OBJECT
public:
  explicit  NetifItemViewWidget(QWidget *parent = nullptr,QByteArray mac = "");
    QByteArray getMac();
    void setMac(QByteArray mac);
    void updateActiveStatus(bool active);
    void setMode(int mode);
protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);

signals:
    void changeAllActive(const QString& mac);
public slots:
    void fontChanged(const QFont &font);
    void updateData(NetifInfo netifInfo);


private:
    void updateWidgetGeometry();

private:
    QColor m_recvColor {"#E14300"};
    QColor m_sentColor {"#004EEF"};

    QString m_ifname;   // 网卡名称
    QString m_recv_bps; // 接受速率
    QString m_sent_bps; // 发送速率
    ChartViewWidget *m_ChartWidget;
    int m_mode;        // 0 表示单个模式，1,表示多个模式。
    QFont m_font;
    QByteArray m_mac;
    bool m_isActive = false;  // 是否被点击

};

#endif // NETIF_ITEM_VIEW_WIDGET_H
