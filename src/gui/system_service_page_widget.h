#ifndef SYSTEM_SERVICE_PAGE_WIDGET_H
#define SYSTEM_SERVICE_PAGE_WIDGET_H

#include <DFrame>

DWIDGET_USE_NAMESPACE

class MainWindow;
class SystemServiceTableView;

class SystemServicePageWidget : public DFrame
{
public:
    explicit SystemServicePageWidget(DWidget *parent = nullptr);
    ~SystemServicePageWidget();

public Q_SLOTS:

Q_SIGNALS:

private:
    SystemServiceTableView *m_svcTableView;
};

#endif  // SYSTEM_SERVICE_PAGE_WIDGET_H
