#ifndef NETIF_ITEM_VIEW_WIDGET_H
#define NETIF_ITEM_VIEW_WIDGET_H

#include <QWidget>
#include <DCommandLinkButton>
DWIDGET_USE_NAMESPACE
class ChartViewWidget;
class NetifInfoModel;
class NetifItemViewWidget : public QWidget
{
    Q_OBJECT
public:
  explicit  NetifItemViewWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
signals:

public slots:
    void fontChanged(const QFont &font);
    void onModelUpdate();
    void setMode(int mode);
private:
    void updateWidgetGeometry();

private:
    QColor upColor {"#FF0000"};
    QColor downColor {"#FEDF19"};

    ChartViewWidget *m_ChartWidget;
     int m_mode;        // 0 表示单个模式，1,表示多个模式。
     NetifInfoModel *m_model;
     QFont m_font;
     int m_index;
};

#endif // NETIF_ITEM_VIEW_WIDGET_H
