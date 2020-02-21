#ifndef SYSTEM_SERVICE_TABLE_VIEW_H
#define SYSTEM_SERVICE_TABLE_VIEW_H

#include <DLabel>
#include <DSpinner>
#include <QShortcut>

#include "base_table_view.h"

DWIDGET_USE_NAMESPACE

class SystemServiceSortFilterProxyModel;
class SystemServiceTableModel;
class SystemServiceEntry;
class MainWindow;
class ServiceManager;
class ErrorContext;

class SystemServiceTableView : public BaseTableView
{
    Q_OBJECT

public:
    SystemServiceTableView(DWidget *parent = nullptr);
    ~SystemServiceTableView() override;

    SystemServiceTableModel *getSourceModel() const;

    bool eventFilter(QObject *obj, QEvent *event) override;

public Q_SLOTS:
    void startService();
    void stopService();
    void restartService();
    void refresh();

protected Q_SLOTS:
    virtual void search(const QString &pattern);
    void displayHeaderContextMenu(const QPoint &);
    void displayTableContextMenu(const QPoint &);
    virtual bool loadSettings();
    virtual void saveSettings();

protected:
    int sizeHintForColumn(int) const override;
    void resizeEvent(QResizeEvent *event) override;

Q_SIGNALS:
    void sectionVisibilityChanged(int, bool);

private:
    int getSelectedServiceEntry(SystemServiceEntry &entry) const;
    void handleTaskError(const ErrorContext &ec) const;
    void asyncGetServiceEntryList();
    QPair<ErrorContext, QList<SystemServiceEntry>> processAsyncGetServiceListTask();
    void resetModel(const ErrorContext &ec, const QList<SystemServiceEntry> &);
    void adjustInfoLabelVisibility();

private:
    SystemServiceTableModel *m_Model;
    SystemServiceSortFilterProxyModel *m_ProxyModel;
    DMenu *m_contextMenu;
    DMenu *m_headerContextMenu;
    DLabel *m_noMatchingResultLabel;

    QShortcut *m_refreshKP;
    QShortcut *m_startKP;
    QShortcut *m_stopKP;
    QShortcut *m_restartKP;

    DSpinner *m_spinner {nullptr};

    bool m_loading {false};
};

#endif  // SYSTEM_SERVICE_TABLE_VIEW_H
