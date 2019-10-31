#ifndef SYSTEM_SERVICE_PAGE_WIDGET_H
#define SYSTEM_SERVICE_PAGE_WIDGET_H

#include <DFrame>
#include <DWidget>

DWIDGET_USE_NAMESPACE

class MainWindow;
class SystemServiceTableView;

class SystemServicePageWidget : public DFrame
{
public:
    SystemServicePageWidget(DWidget *parent = nullptr);
    ~SystemServicePageWidget();

protected:
    void paintEvent(QPaintEvent *);

public Q_SLOTS:

Q_SIGNALS:

private:
    SystemServiceTableView *m_svcTableView;
};

#endif  // SYSTEM_SERVICE_PAGE_WIDGET_H
