#include <DApplication>
#include <DApplicationHelper>
#include <DHeaderView>
#include <DLabel>
#include <DMenu>
#include <DMessageBox>
#include <QDebug>
#include <QFutureWatcher>
#include <QtConcurrent>

#include "common/error_context.h"
#include "main_window.h"
#include "model/system_service_sort_filter_proxy_model.h"
#include "model/system_service_table_model.h"
#include "service/service_manager.h"
#include "service/system_service_entry.h"
#include "service_name_sub_input_dialog.h"
#include "settings.h"
#include "system_service_item_delegate.h"
#include "system_service_table_header_view.h"
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

    m_headerDelegate = new SystemServiceTableHeaderView(Qt::Horizontal, this);
    setHeader(m_headerDelegate);

    // table options
    setSortingEnabled(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setFrameStyle(QFrame::NoFrame);

    // column header options
    DHeaderView *columnHeader = header();
    columnHeader->setSectionsMovable(true);
    columnHeader->setSectionsClickable(true);
    columnHeader->setSectionResizeMode(DHeaderView::Interactive);
    columnHeader->setStretchLastSection(true);
    columnHeader->setSortIndicatorShown(true);
    columnHeader->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    columnHeader->setAlternatingRowColors(true);

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
    QAction *startServiceAction =
        m_contextMenu->addAction(DApplication::translate("Service.Table.Context.Menu", "Start"));
    connect(startServiceAction, &QAction::triggered, this, &SystemServiceTableView::startService);
    m_contextMenu->addSeparator();
    // stop service
    QAction *stopServiceAction =
        m_contextMenu->addAction(DApplication::translate("Service.Table.Context.Menu", "Stop"));
    connect(stopServiceAction, &QAction::triggered, this, &SystemServiceTableView::stopService);
    // restart service
    QAction *restartServiceAction =
        m_contextMenu->addAction(DApplication::translate("Service.Table.Context.Menu", "Restart"));
    connect(restartServiceAction, &QAction::triggered, this,
            &SystemServiceTableView::restartService);
    // refresh context menu item
    QAction *refreshAction =
        m_contextMenu->addAction(DApplication::translate("Service.Table.Context.Menu", "Refresh"));
    connect(refreshAction, &QAction::triggered, this, &SystemServiceTableView::refresh);

    // >>> header context menu
    m_headerContextMenu = new DMenu(this);
    // load state column
    QAction *loadStateHeaderAction = m_headerContextMenu->addAction(
        DApplication::translate("Service.Table.Header", kSystemServiceLoadState));
    loadStateHeaderAction->setCheckable(true);
    connect(loadStateHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(SystemServiceTableModel::kSystemServiceLoadStateColumn, !b);
        saveSettings();
    });
    // active state column
    QAction *activeStateHeaderAction = m_headerContextMenu->addAction(
        DApplication::translate("Service.Table.Header", kSystemServiceActiveState));
    activeStateHeaderAction->setCheckable(true);
    connect(activeStateHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(SystemServiceTableModel::kSystemServiceActiveStateColumn, !b);
        saveSettings();
    });
    // sub state column
    QAction *subStateHeaderAction = m_headerContextMenu->addAction(
        DApplication::translate("Service.Table.Header", kSystemServiceSubState));
    subStateHeaderAction->setCheckable(true);
    connect(subStateHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(SystemServiceTableModel::kSystemServiceSubStateColumn, !b);
        saveSettings();
    });
    // state column
    QAction *stateHeaderAction = m_headerContextMenu->addAction(
        DApplication::translate("Service.Table.Header", kSystemServiceState));
    stateHeaderAction->setCheckable(true);
    connect(stateHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(SystemServiceTableModel::kSystemServiceStateColumn, !b);
        saveSettings();
    });
    // description column
    QAction *descriptionHeaderAction = m_headerContextMenu->addAction(
        DApplication::translate("Service.Table.Header", kSystemServiceDescription));
    descriptionHeaderAction->setCheckable(true);
    connect(descriptionHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(SystemServiceTableModel::kSystemServiceDescriptionColumn, !b);
        saveSettings();
    });
    // pid column
    QAction *pidHeaderAction = m_headerContextMenu->addAction(
        DApplication::translate("Service.Table.Header", kSystemServicePID));
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
    m_noMatchingResultLabel =
        new DLabel(DApplication::translate("Service.Table.Search", "Not Found"), this);
    m_noMatchingResultLabel->setVisible(false);
    QFont font = m_noMatchingResultLabel->font();
    font.setPointSize(16);
    m_noMatchingResultLabel->setFont(font);

    viewport()->setAutoFillBackground(false);
}

SystemServiceTableView::~SystemServiceTableView()
{
    saveSettings();
}

void SystemServiceTableView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setOpacity(1);
    painter.setClipping(true);

    QWidget *wnd = DApplication::activeWindow();
    DPalette::ColorGroup cg;
    if (!wnd) {
        cg = DPalette::Inactive;
    } else {
        cg = DPalette::Active;
    }

    auto style = dynamic_cast<DStyle *>(DApplication::style());
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();

    QBrush bgBrush(palette.color(cg, DPalette::Base));

    QStyleOptionFrame option;
    initStyleOption(&option);
    int radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);

    QRect rect = viewport()->rect();
    QRectF clipRect(rect.x(), rect.y() - rect.height(), rect.width(), rect.height() * 2);
    QRectF subRect(rect.x(), rect.y() - rect.height(), rect.width(), rect.height());
    QPainterPath clipPath, subPath;
    clipPath.addRoundedRect(clipRect, radius, radius);
    subPath.addRect(subRect);
    clipPath = clipPath.subtracted(subPath);

    painter.fillPath(clipPath, bgBrush);

    painter.restore();
    DTreeView::paintEvent(event);
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

    QPoint point = mapToGlobal(p);
    // when popup context menu for items, take table header height into consideration
    point.setY(point.y() +
               model()->headerData(0, Qt::Horizontal, Qt::SizeHintRole).toSize().height());
    // TODO: context menu enable status rule
    m_contextMenu->popup(point);
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

    bool newRow = false;

    // dialog
    if (entry.getId().endsWith('@')) {
        ServiceNameSubInputDialog dialog(this);
        dialog.setTitle(
            DApplication::translate("Service.Instance.Name.Dialog", "Service instance name"));
        dialog.setMessage(entry.getDescription());
        dialog.exec();
        if (dialog.result() == QMessageBox::Ok) {
            QString name = entry.getId().append(dialog.getServiceSubName());

            // already in the list
            QModelIndex idx = getSourceModel()->checkServiceEntryExists(name);
            if (idx.isValid()) {
                QItemSelectionModel *selection = selectionModel();
                selection->select(m_ProxyModel->mapFromSource(idx),
                                  QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows |
                                      QItemSelectionModel::Clear);
                setSelectionModel(selection);
                row = getSelectedServiceEntry(entry);
            } else {  // otherwise, add a new entry
                entry = {};
                entry.setId(name);
                newRow = true;
            }
        } else {  // cancel clicked
            return;
        }
    }

    ServiceManager *mgr = ServiceManager::instance();

    auto result = mgr->startService(entry);
    ec = result.first;
    if (ec) {
        handleTaskError(ec);
    } else {
        if (newRow) {
            QModelIndex idx = getSourceModel()->insertServiceEntry(entry);
            QItemSelectionModel *selection = selectionModel();
            selection->select(m_ProxyModel->mapFromSource(idx), QItemSelectionModel::SelectCurrent |
                                                                    QItemSelectionModel::Rows |
                                                                    QItemSelectionModel::Clear);
            setSelectionModel(selection);
        } else {
            getSourceModel()->updateServiceEntry(row);
        }
    }
}

void SystemServiceTableView::stopService()
{
    ErrorContext ec;
    SystemServiceEntry entry;
    bool deleteItem = false;
    SystemServiceEntry opt {};
    QModelIndex rowIndex;

    int row = getSelectedServiceEntry(entry);
    if (row < 0)
        return;

    // service id syntax: xxx@
    if (entry.getId().endsWith('@')) {
        ServiceNameSubInputDialog dialog(this);
        dialog.setTitle(
            DApplication::translate("Service.Instance.Name.Dialog", "Service instance name"));
        dialog.setMessage(entry.getDescription());
        dialog.exec();
        if (dialog.result() == QMessageBox::Ok) {
            QString name = entry.getId();

            opt.setId(name.append(dialog.getServiceSubName()));

            rowIndex = getSourceModel()->checkServiceEntryExists(name);
            if (rowIndex.isValid()) {
                deleteItem = true;
            }
        } else {
            return;
        }
        // serivce id syntax: xxx@yyy
    } else if (entry.getId().contains(QRegularExpression(
                   "^[^@\\.]+@[^@\\.]+$", QRegularExpression::CaseInsensitiveOption))) {
        deleteItem = true;
        opt.setId(entry.getId());
        rowIndex = getSourceModel()->checkServiceEntryExists(entry.getId());
    } else {
        opt.setId(entry.getId());
    }

    ServiceManager *mgr = ServiceManager::instance();

    auto result = mgr->stopService(opt);
    ec = result.first;
    if (ec) {
        handleTaskError(ec);
    } else {
        if (deleteItem) {
            getSourceModel()->removeServiceEntry(rowIndex);
        } else {
            getSourceModel()->updateServiceEntry(row);
        }
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

    setUpdatesEnabled(false);
    m_noMatchingResultLabel->setVisible(m_ProxyModel->rowCount() == 0);
    if (m_noMatchingResultLabel->isVisible())
        m_noMatchingResultLabel->move(rect().center() - m_noMatchingResultLabel->rect().center());
    setUpdatesEnabled(true);
}

void SystemServiceTableView::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    setUpdatesEnabled(false);
    if (m_noMatchingResultLabel->isVisible())
        m_noMatchingResultLabel->move(rect().center() - m_noMatchingResultLabel->rect().center());
    setUpdatesEnabled(true);

    DTreeView::resizeEvent(event);
}

int SystemServiceTableView::sizeHintForColumn(int column) const
{
    int size = DTreeView::sizeHintForColumn(column);
    // TODO: padding problem
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
