#ifndef PROCESS_PAGE_WIDGET_H
#define PROCESS_PAGE_WIDGET_H

#include <DButtonBox>
#include <DDialog>
#include <DFrame>
#include <DStackedWidget>
#include <QAction>
#include <QHBoxLayout>
#include <QMenu>

#include "interactive_kill.h"
#include "monitor_compact_view.h"
#include "monitor_expand_view.h"
#include "process_manager.h"
#include "process_table_view.h"
#include "settings.h"
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

    void adjustStatusBarWidth();

    void initUI();
    void initConnections();

public Q_SLOTS:
    void switchDisplayMode(DisplayMode mode);

protected:
    void paintEvent(QPaintEvent *);

private Q_SLOTS:
    void popupKillConfirmDialog(int pid);
    void showWindowKiller();
    void createWindowKiller();
    void updateProcessSummary(int napps, int nprocs);

private:
    Settings *m_settings;

    DStackedWidget *m_views;
    MonitorExpandView *m_expandView;
    MonitorCompactView *m_compactView;

    DLabel *m_procViewMode;
    DLabel *m_procViewModeSummary;
    DButtonBoxButton *m_appButton;
    DButtonBoxButton *m_myProcButton;
    DButtonBoxButton *m_allProcButton;

    ProcessTableView *m_procTable;
};

#endif  // PROCESS_PAGE_WIDGET_H
