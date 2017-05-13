#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHBoxLayout>
#include "dmainwindow.h"
#include "status_monitor.h"
#include "process_manager.h"

DWIDGET_USE_NAMESPACE

class MainWindow : public DMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(DMainWindow *parent = 0);
    
private:
    ProcessManager *processManager;
    QHBoxLayout *layout;
    QWidget *layoutWidget;
    StatusMonitor *statusMonitor;
};

#endif
