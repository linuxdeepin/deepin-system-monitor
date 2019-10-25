#ifndef SYSTEM_SERVICE_TABLE_VIEW_H
#define SYSTEM_SERVICE_TABLE_VIEW_H

#include <DTreeView>

DWIDGET_USE_NAMESPACE

class SystemServiceSortFilterProxyModel;
class SystemServiceTableModel;
class SystemServiceEntry;
class MainWindow;
class ServiceManager;
class SystemServiceItemDelegate;
class ErrorContext;

class SystemServiceTableView : public DTreeView
{
    Q_OBJECT

public:
    explicit SystemServiceTableView(DWidget *parent = nullptr);
    ~SystemServiceTableView() override;

    SystemServiceTableModel *getSourceModel() const;

public slots:
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
    void resizeEvent(QResizeEvent *event) override;
    int sizeHintForColumn(int) const override;

Q_SIGNALS:
    void sectionVisibilityChanged(int, bool);

private:
    int getSelectedServiceEntry(SystemServiceEntry &entry) const;
    void handleTaskError(const ErrorContext &ec) const;
    void asyncGetServiceEntryList();
    QPair<ErrorContext, QList<SystemServiceEntry>> processAsyncGetServiceListTask();
    void resetModel(const ErrorContext &ec, const QList<SystemServiceEntry> &);

private:
    SystemServiceTableModel *m_Model;
    SystemServiceSortFilterProxyModel *m_ProxyModel;
    DMenu *m_contextMenu;
    DMenu *m_headerContextMenu;
    DLabel *m_noMatchingResultLabel;
    SystemServiceItemDelegate *m_itemDelegate;
    QTimer *m_timer;
};

#endif  // SYSTEM_SERVICE_TABLE_VIEW_H
