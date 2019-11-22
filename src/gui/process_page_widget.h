#ifndef PROCESS_PAGE_WIDGET_H
#define PROCESS_PAGE_WIDGET_H

#include <DApplicationHelper>
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
    void changeIconTheme(DApplicationHelper::ColorType themeType);

private:
    Settings *m_settings;

    DStackedWidget *m_views {nullptr};
    MonitorExpandView *m_expandView {nullptr};
    MonitorCompactView *m_compactView {nullptr};

    DLabel *m_procViewMode {nullptr};
    DLabel *m_procViewModeSummary {nullptr};
    DButtonBoxButton *m_appButton {nullptr};
    DButtonBoxButton *m_myProcButton {nullptr};
    DButtonBoxButton *m_allProcButton {nullptr};

    ProcessTableView *m_procTable {nullptr};

    InteractiveKill *m_wndKiller {nullptr};
};

#endif  // PROCESS_PAGE_WIDGET_H
