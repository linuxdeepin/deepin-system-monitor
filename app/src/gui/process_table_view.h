#ifndef PROCESS_TABLE_VIEW_H
#define PROCESS_TABLE_VIEW_H

#include <DLabel>
#include <DTreeView>
#include <QShortcut>

#include "base_table_view.h"
#include "process/system_monitor.h"

DWIDGET_USE_NAMESPACE

class ProcessTableModel;
class ProcessSortFilterProxyModel;
class ProcessItemDelegate;
class ProcessTableHeaderView;

class ProcessTableView : public BaseTableView
{
    Q_OBJECT

public:
    explicit ProcessTableView(DWidget *parent = nullptr);
    ~ProcessTableView() override;

    bool eventFilter(QObject *obj, QEvent *event) override;

public Q_SLOTS:
    void endProcess();
    void pauseProcess();
    void resumeProcess();
    void openExecDirWithFM();
    void showProperties();
    void killProcess();
    void search(const QString &text);
    void switchDisplayMode(SystemMonitor::FilterType type);
    void changeProcessPriority(int priority);

protected:
    bool loadSettings();
    void saveSettings();
    void initUI(bool settingsLoaded);
    void initConnections(bool settingsLoaded);
    void displayProcessTableContextMenu(const QPoint &p);
    void displayProcessTableHeaderContextMenu(const QPoint &p);

    void resizeEvent(QResizeEvent *event) override;

    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected) override;
    int sizeHintForColumn(int column) const override;

private:
    void adjustInfoLabelVisibility();
    void customizeProcessPriority();

private:
    ProcessTableModel *m_model;
    ProcessSortFilterProxyModel *m_proxyModel;

    DMenu *m_contextMenu;
    DMenu *m_headerContextMenu;
    DLabel *m_notFoundLabel;
    ProcessItemDelegate *m_itemDelegate;
    ProcessTableHeaderView *m_headerView;

    QVariant m_selectedPID {};

    QShortcut *m_endProcKP {nullptr};
    QShortcut *m_pauseProcKP {nullptr};
    QShortcut *m_resumeProcKP {nullptr};
    QShortcut *m_viewPropKP {nullptr};
    QShortcut *m_killProcKP {nullptr};
};

#endif  // PROCESS_TABLE_VIEW_H
