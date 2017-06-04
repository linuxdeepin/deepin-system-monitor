#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ddialog.h"
#include "dmainwindow.h"
#include "interactive_kill.h"
#include "process_manager.h"
#include "status_monitor.h"
#include "toolbar.h"
#include <QAction>
#include <QHBoxLayout>
#include <QMenu>

DWIDGET_USE_NAMESPACE

class MainWindow : public DMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(DMainWindow *parent = 0);
                                       
    bool eventFilter(QObject *, QEvent *);
    void paintEvent(QPaintEvent *);
    
public slots:
    void createWindowKiller();
    void dialogButtonClicked(int index, QString);
    void popupKillConfirmDialog(int pid);
    void showWindowKiller();
    void switchTab(int index);
    
private:
    DDialog *killProcessDialog;
    InteractiveKill *killer;
    ProcessManager *processManager;
    QAction *killAction;
    QHBoxLayout *layout;
    QMenu *menu;
    QWidget *layoutWidget;
    StatusMonitor *statusMonitor;
    Toolbar *toolbar;
    int killPid;
};

#endif
