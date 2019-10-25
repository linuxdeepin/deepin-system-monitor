#include <QDebug>
#include <QFutureWatcher>
#include <QtConcurrent>

#include <DHeaderView>
#include <DLabel>
#include <DMenu>
#include <DMessageBox>

#include "common/error_context.h"
#include "main_window.h"
#include "model/system_service_sort_filter_proxy_model.h"
#include "model/system_service_table_model.h"
#include "service/service_manager.h"
#include "service/system_service_entry.h"
#include "service_name_sub_input_dialog.h"
#include "settings.h"
#include "system_service_item_delegate.h"
#include "system_service_table_view.h"
#include "toolbar.h"

DWIDGET_USE_NAMESPACE

static const char *kSettingsOption_ServiceTableHeaderState = "service_table_hearder_state";

SystemServiceTableView::SystemServiceTableView(DWidget *parent)
    : DTreeView(parent)
{
    bool settingsLoaded = loadSettings();

    m_itemDelegate = new SystemServiceItemDelegate(this);
    setItemDelegate(m_itemDelegate);

    // table options
    setSortingEnabled(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setAlternatingRowColors(true);
    setAllColumnsShowFocus(true);
    setRootIsDecorated(false);
    setItemsExpandable(false);

    // column header options
    DHeaderView *columnHeader = header();
    columnHeader->setSectionsMovable(true);
    columnHeader->setSectionsClickable(true);
    columnHeader->setSectionResizeMode(DHeaderView::Interactive);
    columnHeader->setStretchLastSection(true);
    columnHeader->setSortIndicatorShown(true);
    columnHeader->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    columnHeader->setDefaultSectionSize(36);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, [=]() { saveSettings(); });

    // table events
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &DWidget::customContextMenuRequested, this,
            &SystemServiceTableView::displayTableContextMenu);
    // table header events
    columnHeader->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(columnHeader, &QHeaderView::sectionResized, this, [=]() { m_timer->start(2000); });
    connect(columnHeader, &QHeaderView::sectionMoved, this, [=]() { saveSettings(); });
    connect(columnHeader, &QHeaderView::sortIndicatorChanged, this, [=]() { saveSettings(); });
    connect(columnHeader, &QWidget::customContextMenuRequested, this,
            &SystemServiceTableView::displayHeaderContextMenu);

    MainWindow *mainWindow = MainWindow::instance();
    connect(mainWindow->toolbar(), &Toolbar::search, this, &SystemServiceTableView::search);

    // >>> table model
    m_ProxyModel = new SystemServiceSortFilterProxyModel(this);
    m_Model = new SystemServiceTableModel(this);
    QList<SystemServiceEntry> empty;
    m_Model->setServiceEntryList(empty);
    m_ProxyModel->setSourceModel(m_Model);
    setModel(m_ProxyModel);
    asyncGetServiceEntryList();

    // >>> table default style
    if (!settingsLoaded) {
        setColumnWidth(SystemServiceTableModel::kSystemServiceNameColumn, 200);
        setColumnHidden(SystemServiceTableModel::kSystemServiceNameColumn, false);
        setColumnWidth(SystemServiceTableModel::kSystemServiceLoadStateColumn, 100);
        setColumnHidden(SystemServiceTableModel::kSystemServiceLoadStateColumn, true);
        setColumnWidth(SystemServiceTableModel::kSystemServiceActiveStateColumn, 100);
        setColumnHidden(SystemServiceTableModel::kSystemServiceActiveStateColumn, false);
        setColumnWidth(SystemServiceTableModel::kSystemServiceSubStateColumn, 100);
        setColumnHidden(SystemServiceTableModel::kSystemServiceSubStateColumn, false);
        setColumnWidth(SystemServiceTableModel::kSystemServiceStateColumn, 100);
        setColumnHidden(SystemServiceTableModel::kSystemServiceStateColumn, false);
        setColumnWidth(SystemServiceTableModel::kSystemServiceDescriptionColumn, 320);
        setColumnHidden(SystemServiceTableModel::kSystemServiceDescriptionColumn, false);
        setColumnWidth(SystemServiceTableModel::kSystemServicePIDColumn, 100);
        setColumnHidden(SystemServiceTableModel::kSystemServicePIDColumn, true);
        sortByColumn(SystemServiceTableModel::kSystemServiceNameColumn, Qt::AscendingOrder);
    }

    // >>> service tableview context menu
    m_contextMenu = new DMenu(this);
    // start service
    QAction *startServiceAction = m_contextMenu->addAction(tr("Start"));
    connect(startServiceAction, &QAction::triggered, this, &SystemServiceTableView::startService);
    // stop service
    QAction *stopServiceAction = m_contextMenu->addAction(tr("Stop"));
    connect(stopServiceAction, &QAction::triggered, this, &SystemServiceTableView::stopService);
    // restart service
    QAction *restartServiceAction = m_contextMenu->addAction(tr("Restart"));
    connect(restartServiceAction, &QAction::triggered, this,
            &SystemServiceTableView::restartService);
    // refresh context menu item
    QAction *refreshAction = m_contextMenu->addAction(tr("Refresh"));
    connect(refreshAction, &QAction::triggered, this, &SystemServiceTableView::refresh);

    // >>> header context menu
    m_headerContextMenu = new DMenu(this);
    // load state column
    QAction *loadStateHeaderAction = m_headerContextMenu->addAction(tr(kSystemServiceLoadState));
    loadStateHeaderAction->setCheckable(true);
    connect(loadStateHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(SystemServiceTableModel::kSystemServiceLoadStateColumn, !b);
        saveSettings();
    });
    // active state column
    QAction *activeStateHeaderAction =
        m_headerContextMenu->addAction(tr(kSystemServiceActiveState));
    activeStateHeaderAction->setCheckable(true);
    connect(activeStateHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(SystemServiceTableModel::kSystemServiceActiveStateColumn, !b);
        saveSettings();
    });
    // sub state column
    QAction *subStateHeaderAction = m_headerContextMenu->addAction(tr(kSystemServiceSubState));
    subStateHeaderAction->setCheckable(true);
    connect(subStateHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(SystemServiceTableModel::kSystemServiceSubStateColumn, !b);
        saveSettings();
    });
    // state column
    QAction *stateHeaderAction = m_headerContextMenu->addAction(tr(kSystemServiceState));
    stateHeaderAction->setCheckable(true);
    connect(stateHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(SystemServiceTableModel::kSystemServiceStateColumn, !b);
        saveSettings();
    });
    // description column
    QAction *descriptionHeaderAction =
        m_headerContextMenu->addAction(tr(kSystemServiceDescription));
    descriptionHeaderAction->setCheckable(true);
    connect(descriptionHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(SystemServiceTableModel::kSystemServiceDescriptionColumn, !b);
        saveSettings();
    });
    // pid column
    QAction *pidHeaderAction = m_headerContextMenu->addAction(tr(kSystemServicePID));
    pidHeaderAction->setCheckable(true);
    connect(pidHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(SystemServiceTableModel::kSystemServicePIDColumn, !b);
        saveSettings();
    });

    if (!settingsLoaded) {
        loadStateHeaderAction->setChecked(false);
        activeStateHeaderAction->setChecked(true);
        subStateHeaderAction->setChecked(true);
        stateHeaderAction->setChecked(true);
        descriptionHeaderAction->setChecked(true);
        pidHeaderAction->setChecked(false);
    }
    connect(m_headerContextMenu, &QMenu::aboutToShow, this, [=]() {
        bool b;
        b = header()->isSectionHidden(SystemServiceTableModel::kSystemServiceLoadStateColumn);
        loadStateHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(SystemServiceTableModel::kSystemServiceActiveStateColumn);
        activeStateHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(SystemServiceTableModel::kSystemServiceSubStateColumn);
        subStateHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(SystemServiceTableModel::kSystemServiceStateColumn);
        stateHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(SystemServiceTableModel::kSystemServiceDescriptionColumn);
        descriptionHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(SystemServiceTableModel::kSystemServicePIDColumn);
        pidHeaderAction->setChecked(!b);
    });

    // >>> "no matching result" display label
    m_noMatchingResultLabel = new DLabel(tr("No matching result"), this);
    m_noMatchingResultLabel->setVisible(false);
    QFont font = m_noMatchingResultLabel->font();
    font.setPointSize(16);
    m_noMatchingResultLabel->setFont(font);
}

SystemServiceTableView::~SystemServiceTableView()
{
    saveSettings();
}

void SystemServiceTableView::saveSettings()
{
    Settings *s = Settings::instance();
    if (s) {
        QByteArray buf = header()->saveState();
        s->setOption(kSettingsOption_ServiceTableHeaderState, buf.toBase64());
        s->flush();
    }
}

bool SystemServiceTableView::loadSettings()
{
    Settings *s = Settings::instance();
    if (s) {
        QVariant opt = s->getOption(kSettingsOption_ServiceTableHeaderState);
        if (opt.isValid()) {
            QByteArray buf = QByteArray::fromBase64(opt.toByteArray());
            header()->restoreState(buf);
            return true;
        }
    }
    return false;
}

void SystemServiceTableView::displayHeaderContextMenu(const QPoint &p)
{
    m_headerContextMenu->popup(mapToGlobal(p));
}

void SystemServiceTableView::displayTableContextMenu(const QPoint &p)
{
    if (selectedIndexes().size() == 0)
        return;
    // TODO: context menu enable status rule
    m_contextMenu->popup(mapToGlobal(p));
}

int SystemServiceTableView::getSelectedServiceEntry(SystemServiceEntry &entry) const
{
    QModelIndexList list = selectedIndexes();
    if (list.size() > 0) {
        QModelIndex index = m_ProxyModel->mapToSource(list.at(0));
        SystemServiceEntry e = getSourceModel()->getSystemServiceEntry(index);
        if (!e.getId().isEmpty()) {
            entry = e;
            return list.at(0).row();
        }
    }

    return -1;
}

void SystemServiceTableView::startService()
{
    ErrorContext ec;
    SystemServiceEntry entry;
    int row = getSelectedServiceEntry(entry);
    if (row < 0)
        return;

    // dialog
    if (entry.getId().endsWith('@')) {
        ServiceNameSubInputDialog dialog(this);
        dialog.setTitle(tr("Service Name"));
        dialog.setMessage(entry.getDescription());
        dialog.exec();
        if (dialog.result() == QMessageBox::Ok)
            entry.setId(entry.getId().append(dialog.getServiceSubName()));
        else {
            return;
        }
    }

    ServiceManager *mgr = ServiceManager::instance();

    auto result = mgr->startService(entry);
    ec = result.first;
    if (ec) {
        handleTaskError(ec);
    } else {
        getSourceModel()->updateServiceEntry(row);
    }
}

void SystemServiceTableView::stopService()
{
    ErrorContext ec;
    SystemServiceEntry entry;
    int row = getSelectedServiceEntry(entry);
    if (row < 0)
        return;

    ServiceManager *mgr = ServiceManager::instance();

    auto result = mgr->stopService(entry);
    ec = result.first;
    if (ec) {
        handleTaskError(ec);
    } else {
        getSourceModel()->updateServiceEntry(row);
    }
}

void SystemServiceTableView::restartService()
{
    ErrorContext ec;
    SystemServiceEntry entry;
    int row = getSelectedServiceEntry(entry);
    if (row < 0)
        return;

    ServiceManager *mgr = ServiceManager::instance();

    auto result = mgr->restartService(entry);
    ec = result.first;
    if (ec) {
        handleTaskError(ec);
    } else {
        getSourceModel()->updateServiceEntry(row);
    }
}

void SystemServiceTableView::handleTaskError(const ErrorContext &ec) const
{
    MainWindow *mainWindow = MainWindow::instance();
    DMessageBox::critical(mainWindow, ec.getErrorName(), ec.getErrorMessage());
}

void SystemServiceTableView::search(const QString &pattern)
{
    m_ProxyModel->setFilterRegExp(QRegExp(pattern, Qt::CaseInsensitive));

    m_noMatchingResultLabel->setVisible(m_ProxyModel->rowCount() == 0);
    if (m_noMatchingResultLabel->isVisible())
        m_noMatchingResultLabel->move(rect().center() - m_noMatchingResultLabel->rect().center());
}

void SystemServiceTableView::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    if (m_noMatchingResultLabel->isVisible())
        m_noMatchingResultLabel->move(rect().center() - m_noMatchingResultLabel->rect().center());

    DTreeView::resizeEvent(event);
}

int SystemServiceTableView::sizeHintForColumn(int column) const
{
    int size = DTreeView::sizeHintForColumn(column);
    return size + 32;
}

void SystemServiceTableView::refresh()
{
    asyncGetServiceEntryList();
}

SystemServiceTableModel *SystemServiceTableView::getSourceModel() const
{
    return m_Model;
}

void SystemServiceTableView::asyncGetServiceEntryList()
{
    auto *watcher = new QFutureWatcher<QPair<ErrorContext, QList<SystemServiceEntry>>>;
    QFuture<QPair<ErrorContext, QList<SystemServiceEntry>>> future;
    QObject::connect(watcher, &QFutureWatcher<void>::finished, [=]() {
        auto fe = watcher->future();
        resetModel(fe.result().first, fe.result().second);
        watcher->deleteLater();
    });
    future = QtConcurrent::run(this, &SystemServiceTableView::processAsyncGetServiceListTask);
    watcher->setFuture(future);
}

QPair<ErrorContext, QList<SystemServiceEntry>>
SystemServiceTableView::processAsyncGetServiceListTask()
{
    ServiceManager *mgr = ServiceManager::instance();
    Q_ASSERT(mgr != nullptr);
    return mgr->getServiceEntryList();
}

void SystemServiceTableView::resetModel(const ErrorContext &ec,
                                        const QList<SystemServiceEntry> &list)
{
    if (ec) {
        handleTaskError(ec);
        return;
    }
    m_Model->setServiceEntryList(list);
    m_ProxyModel->setSourceModel(m_Model);
    setModel(m_ProxyModel);
    loadSettings();
}
