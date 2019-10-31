#ifndef PROCESS_PAGE_WIDGET_H
#define PROCESS_PAGE_WIDGET_H

#include <DDialog>
#include <DFrame>
#include <QAction>
#include <QHBoxLayout>
#include <QMenu>

#include "interactive_kill.h"
#include "process_manager.h"
#include "settings.h"
#include "status_monitor.h"
#include "toolbar.h"
#include "ui_common.h"

DWIDGET_USE_NAMESPACE

class MainWindow;

class ProcessPageWidget : public DFrame
{
    Q_OBJECT

public:
    explicit ProcessPageWidget(DWidget *parent = nullptr);
    ~ProcessPageWidget();

    QList<bool> getColumnHideFlags();
    bool eventFilter(QObject *, QEvent *);
    bool getSortingOrder();
    int getSortingIndex();
    void initCompactModeAction();
    void adjustStatusBarWidth();

    void initUI();
    void initConnections();

public Q_SLOTS:
    void createWindowKiller();
    void dialogButtonClicked(int index, QString);
    void popupKillConfirmDialog(int pid);
    void recordSortingStatus(int index, bool sortingOrder);
    void recordVisibleColumn(int index, bool visible, QList<bool> columnVisibles);
    void showWindowKiller();
    void switchTab(int index);
    void switchDisplayMode(DisplayMode mode);

protected:
    void paintEvent(QPaintEvent *);

private:
    Settings *m_settings;

    DDialog *killProcessDialog;
    InteractiveKill *killer;
    ProcessManager *processManager;
    QHBoxLayout *layout;
    // TODO
    QString backgroundColor;
    StatusMonitor *statusMonitor;
    int killPid;
};

#endif  // PROCESS_PAGE_WIDGET_H
