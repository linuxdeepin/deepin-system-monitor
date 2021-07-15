#include <QScrollBar>
#include <QtConcurrent>
#include <QKeyEvent>
#include <QDebug>
#include <QFuture>
#include <QtConcurrent>
#include <QFutureWatcher>

#include <DApplication>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DHeaderView>
#include <DLabel>
#include <DMenu>
#include <DMessageBox>
#include <DSpinner>

#include "common/error_context.h"
#include "main_window.h"
#include "model/system_service_sort_filter_proxy_model.h"
#include "model/system_service_table_model.h"
#include "service/service_manager.h"
#include "service/system_service_entry.h"
#include "service_name_sub_input_dialog.h"
#include "settings.h"
#include "system_service_table_view.h"
#include "toolbar.h"
#include "dialog/error_dialog.h"
#include "dbus/systemd1_unit_interface.h"

DWIDGET_USE_NAMESPACE

static const char *kSettingsOption_ServiceTableHeaderState = "service_table_header_state";

// not thread-safe
static bool defer_initialized {false};

SystemServiceTableView::SystemServiceTableView(DWidget *parent)
    : BaseTableView(parent)
{
    installEventFilter(this);

    // >>> table model
    m_model = new SystemServiceTableModel(this);
    m_proxyModel = new SystemServiceSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    setModel(m_proxyModel);

    bool settingsLoaded = loadSettings();

    initUI(settingsLoaded);
    initConnections();
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

    QPoint point = mapToGlobal(p);
    // when popup context menu for items, take table header height into consideration
    point.setY(point.y() + header()->sizeHint().height());
    m_contextMenu->popup(point);
}

void SystemServiceTableView::startService()
{
    if (!m_selectedSName.isValid())
        return;

    auto sname = m_selectedSName.toString();

    // dialog
    if (sname.endsWith('@')) {
        ServiceNameSubInputDialog dialog(this);
        dialog.setTitle(
            DApplication::translate("Service.Instance.Name.Dialog", "Service instance name"));
        auto desc = selectedIndexes()
                    .value(SystemServiceTableModel::kSystemServiceDescriptionColumn)
                    .data()
                    .toString();
        dialog.setMessage(desc);
        dialog.exec();
        if (dialog.result() == QMessageBox::Ok) {
            sname = sname.append(dialog.getServiceSubName());
        } else {  // cancel clicked
            return;
        }
    }

    auto *mwnd = MainWindow::instance();
    Q_ASSERT(mwnd != nullptr);
    auto *mgr = ServiceManager::instance();
    Q_ASSERT(mgr != nullptr);

    Q_EMIT mwnd->authProgressStarted();
    auto *watcher  = new QFutureWatcher<ErrorContext>();
    connect(watcher, &QFutureWatcher<ErrorContext>::finished, this, [this, mgr, mwnd, watcher, sname]() {
        Q_EMIT mwnd->authProgressEnded();
        refreshServiceStatus(sname);
        if (watcher->result()) {
            Q_EMIT mgr->errorOccurred(watcher->result());
        }
        watcher->deleteLater();
    });
    QFuture<ErrorContext> fu = QtConcurrent::run([mgr, sname]() {
        auto ec = mgr->startService(sname);
        return ec;
    });
    watcher->setFuture(fu);
}

void SystemServiceTableView::stopService()
{
    if (!m_selectedSName.isValid()) {
        return;
    }

    auto sname = m_selectedSName.toString();

    // service id syntax: xxx@
    if (sname.endsWith('@')) {
        ServiceNameSubInputDialog dialog(this);
        dialog.setTitle(
            DApplication::translate("Service.Instance.Name.Dialog", "Service instance name"));
        auto desc = selectedIndexes()
                    .value(SystemServiceTableModel::kSystemServiceDescriptionColumn)
                    .data()
                    .toString();
        dialog.setMessage(desc);
        dialog.exec();
        if (dialog.result() == QMessageBox::Ok) {
            sname = sname.append(dialog.getServiceSubName());
        } else {
            return;
        }
    }

    auto *mwnd = MainWindow::instance();
    Q_ASSERT(mwnd != nullptr);
    auto *mgr = ServiceManager::instance();
    Q_ASSERT(mgr != nullptr);

    Q_EMIT mwnd->authProgressStarted();
    auto *watcher  = new QFutureWatcher<ErrorContext>();
    connect(watcher, &QFutureWatcher<ErrorContext>::finished, this, [this, mgr, mwnd, watcher, sname]() {
        Q_EMIT mwnd->authProgressEnded();
        refreshServiceStatus(sname);
        if (watcher->result()) {
            Q_EMIT mgr->errorOccurred(watcher->result());
        }
        watcher->deleteLater();
    });
    QFuture<ErrorContext> fu = QtConcurrent::run([mgr, sname]() {
        auto ec = mgr->stopService(sname);
        return ec;
    });
    watcher->setFuture(fu);
}

void SystemServiceTableView::restartService()
{
    if (!m_selectedSName.isValid()) {
        return;
    }

    auto sname = m_selectedSName.toString();

    // service id syntax: xxx@
    if (sname.endsWith('@')) {
        ServiceNameSubInputDialog dialog(this);
        dialog.setTitle(
            DApplication::translate("Service.Instance.Name.Dialog", "Service instance name"));
        auto desc = selectedIndexes()
                    .value(SystemServiceTableModel::kSystemServiceDescriptionColumn)
                    .data()
                    .toString();
        dialog.setMessage(desc);
        dialog.exec();
        if (dialog.result() == QMessageBox::Ok) {
            sname = sname.append(dialog.getServiceSubName());
        } else {
            return;
        }
    }

    auto *mwnd = MainWindow::instance();
    Q_ASSERT(mwnd != nullptr);
    auto *mgr = ServiceManager::instance();
    Q_ASSERT(mgr != nullptr);

    Q_EMIT mwnd->authProgressStarted();
    auto *watcher  = new QFutureWatcher<ErrorContext>();
    connect(watcher, &QFutureWatcher<ErrorContext>::finished, this, [this, mgr, mwnd, watcher, sname]() {
        Q_EMIT mwnd->authProgressEnded();
        refreshServiceStatus(sname);
        if (watcher->result()) {
            Q_EMIT mgr->errorOccurred(watcher->result());
        }
        watcher->deleteLater();
    });
    QFuture<ErrorContext> fu = QtConcurrent::run([mgr, sname]() {
        auto ec = mgr->restartService(sname);
        return ec;
    });
    watcher->setFuture(fu);
}

void SystemServiceTableView::setServiceStartupMode(bool autoStart)
{
    if (!m_selectedSName.isValid()) {
        return;
    }

    auto sname = m_selectedSName.toString();

    auto *mwnd = MainWindow::instance();
    Q_ASSERT(mwnd != nullptr);
    auto *mgr = ServiceManager::instance();
    Q_ASSERT(mgr != nullptr);

    Q_EMIT mwnd->authProgressStarted();
    auto *watcher  = new QFutureWatcher<ErrorContext>();
    connect(watcher, &QFutureWatcher<ErrorContext>::finished, this, [this, mgr, mwnd, watcher, sname]() {
        Q_EMIT mwnd->authProgressEnded();
        refreshServiceStatus(sname);
        if (watcher->result()) {
            Q_EMIT mgr->errorOccurred(watcher->result());
        }
        watcher->deleteLater();
    });
    QFuture<ErrorContext> fu = QtConcurrent::run([mgr, sname, autoStart]() {
        auto ec = mgr->setServiceStartupMode(sname, autoStart);
        return ec;
    });
    watcher->setFuture(fu);
}

void SystemServiceTableView::handleTaskError(const ErrorContext &ec) const
{
    MainWindow *mainWindow = MainWindow::instance();
    ErrorDialog::show(mainWindow, ec.getErrorName(), ec.getErrorMessage());
}

void SystemServiceTableView::adjustInfoLabelVisibility()
{
    setUpdatesEnabled(false);
    m_noMatchingResultLabel->setVisible(!m_proxyModel->rowCount() &&
                                        m_spinner &&
                                        !m_spinner->isPlaying());
    if (m_noMatchingResultLabel->isVisible())
        m_noMatchingResultLabel->move(
            rect().center() - m_noMatchingResultLabel->rect().center());
    setUpdatesEnabled(true);
}

void SystemServiceTableView::refreshServiceStatus(const QString sname)
{
    auto *mgr = ServiceManager::instance();
    Q_ASSERT(mgr != nullptr);

    auto sn = mgr->normalizeServiceId(sname);
    auto o = Systemd1UnitInterface::normalizeUnitPath(sn);
    auto entry = mgr->updateServiceEntry(o.path());

    if (!isFinalState(entry.getActiveState().toLocal8Bit())) {
        auto *timer = new CustomTimer(mgr, this);
        timer->start(o.path());
    }
}

void SystemServiceTableView::search(const QString &pattern)
{
    m_proxyModel->setFilterRegExp(QRegExp(pattern, Qt::CaseInsensitive));

    adjustInfoLabelVisibility();
}

void SystemServiceTableView::resizeEvent(QResizeEvent *event)
{
    if (m_spinner) {
        m_spinner->move(rect().center() - m_spinner->rect().center());
    }
    adjustInfoLabelVisibility();

    BaseTableView::resizeEvent(event);
}

void SystemServiceTableView::selectionChanged(const QItemSelection &selected,
                                              const QItemSelection &deselected)
{
    if (selected.size() <= 0) {
        return;
    }

    m_selectedSName = selected.indexes()
                      .value(SystemServiceTableModel::kSystemServiceNameColumn)
                      .data();

    BaseTableView::selectionChanged(selected, deselected);
}

void SystemServiceTableView::initUI(bool settingsLoaded)
{
    // >>> "not found" display label
    m_noMatchingResultLabel =
        new DLabel(DApplication::translate("Common.Search", "No search results"), this);
    DFontSizeManager::instance()->bind(m_noMatchingResultLabel, DFontSizeManager::T4);
    auto palette = DApplicationHelper::instance()->palette(m_noMatchingResultLabel);
    QColor labelColor = palette.color(DPalette::PlaceholderText);
    palette.setColor(DPalette::Text, labelColor);
    m_noMatchingResultLabel->setPalette(palette);
    m_noMatchingResultLabel->setVisible(false);

    auto *hdr = header();
    hdr->setSectionsMovable(true);
    hdr->setSectionsClickable(true);
    hdr->setSectionResizeMode(DHeaderView::Interactive);
    hdr->setStretchLastSection(true);
    hdr->setSortIndicatorShown(true);
    hdr->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    hdr->setContextMenuPolicy(Qt::CustomContextMenu);

    // table options
    setSortingEnabled(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setContextMenuPolicy(Qt::CustomContextMenu);

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
        setColumnWidth(SystemServiceTableModel::kSystemServiceStartupModeColumn, 80);
        setColumnHidden(SystemServiceTableModel::kSystemServiceStartupModeColumn, true);
        setColumnWidth(SystemServiceTableModel::kSystemServiceDescriptionColumn, 320);
        setColumnHidden(SystemServiceTableModel::kSystemServiceDescriptionColumn, false);
        setColumnWidth(SystemServiceTableModel::kSystemServicePIDColumn, 100);
        setColumnHidden(SystemServiceTableModel::kSystemServicePIDColumn, true);
        sortByColumn(SystemServiceTableModel::kSystemServiceNameColumn, Qt::AscendingOrder);
    }

    // >>> service tableview context menu
    m_contextMenu = new DMenu(this);
    // start service
    m_startServiceAction =
        m_contextMenu->addAction(DApplication::translate("Service.Table.Context.Menu", "Start"));
    m_startServiceAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_S));
    // stop service
    m_stopServiceAction =
        m_contextMenu->addAction(DApplication::translate("Service.Table.Context.Menu", "Stop"));
    m_stopServiceAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_T));
    // restart service
    m_restartServiceAction =
        m_contextMenu->addAction(DApplication::translate("Service.Table.Context.Menu", "Restart"));
    m_restartServiceAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_R));
    // service startup mode
    m_setServiceStartupModeMenu =
        m_contextMenu->addMenu(DApplication::translate("Service.Table.Context.Menu", "Startup type"));
    m_setAutoStartAction = m_setServiceStartupModeMenu->addAction(DApplication::translate("Service.Table.Context.Menu", "Auto"));
    m_setManualStartAction = m_setServiceStartupModeMenu->addAction(DApplication::translate("Service.Table.Context.Menu", "Manual"));
    // refresh context menu item
    m_refreshAction =
        m_contextMenu->addAction(DApplication::translate("Service.Table.Context.Menu", "Refresh"));
    m_refreshAction->setShortcut(QKeySequence(QKeySequence::Refresh));

    // >>> header context menu
    m_headerContextMenu = new DMenu(this);
    // load state column
    m_loadStateHeaderAction = m_headerContextMenu->addAction(
                                  DApplication::translate("Service.Table.Header", kSystemServiceLoadState));
    m_loadStateHeaderAction->setCheckable(true);
    // active state column
    m_activeStateHeaderAction = m_headerContextMenu->addAction(
                                    DApplication::translate("Service.Table.Header", kSystemServiceActiveState));
    m_activeStateHeaderAction->setCheckable(true);
    // sub state column
    m_subStateHeaderAction = m_headerContextMenu->addAction(
                                 DApplication::translate("Service.Table.Header", kSystemServiceSubState));
    m_subStateHeaderAction->setCheckable(true);
    // state column
    m_stateHeaderAction = m_headerContextMenu->addAction(
                              DApplication::translate("Service.Table.Header", kSystemServiceState));
    m_stateHeaderAction->setCheckable(true);
    // description column
    m_descriptionHeaderAction = m_headerContextMenu->addAction(
                                    DApplication::translate("Service.Table.Header", kSystemServiceDescription));
    m_descriptionHeaderAction->setCheckable(true);
    // pid column
    m_pidHeaderAction = m_headerContextMenu->addAction(
                            DApplication::translate("Service.Table.Header", kSystemServicePID));
    m_pidHeaderAction->setCheckable(true);
    // startup mode column
    m_startupModeHeaderAction = m_headerContextMenu->addAction(
                                    DApplication::translate("Service.Table.Header", kSystemServiceStartupMode));
    m_startupModeHeaderAction->setCheckable(true);

    if (!settingsLoaded) {
        m_loadStateHeaderAction->setChecked(false);
        m_activeStateHeaderAction->setChecked(true);
        m_subStateHeaderAction->setChecked(true);
        m_stateHeaderAction->setChecked(true);
        m_descriptionHeaderAction->setChecked(true);
        m_pidHeaderAction->setChecked(false);
    }

    m_refreshKP = new QShortcut(QKeySequence(QKeySequence::Refresh), this);
    m_startKP = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_S), this);
    m_stopKP = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_T), this);
    m_restartKP = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_R), this);

    m_spinner = new DSpinner(this);
    auto pa = DApplicationHelper::instance()->applicationPalette();
    QBrush hlBrush = pa.color(DPalette::Active, DPalette::Highlight);
    pa.setColor(DPalette::Highlight, hlBrush.color());
    m_spinner->setPalette(pa);
    m_spinner->move(rect().center() - m_spinner->rect().center());
}

void SystemServiceTableView::initConnections()
{
    auto *dAppHelper = DApplicationHelper::instance();
    Q_ASSERT(dAppHelper != nullptr);
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, [ = ]() {
        if (m_noMatchingResultLabel) {
            auto palette = DApplicationHelper::instance()->applicationPalette();
            QColor labelColor = palette.color(DPalette::PlaceholderText);
            palette.setColor(DPalette::Text, labelColor);
            m_noMatchingResultLabel->setPalette(palette);
        }
    });

    // table events
    connect(this, &SystemServiceTableView::customContextMenuRequested, this,
            &SystemServiceTableView::displayTableContextMenu);

    // table header events
    auto *hdr = header();
    connect(hdr, &QHeaderView::sectionResized, this, [ = ]() { saveSettings(); });
    connect(hdr, &QHeaderView::sectionMoved, this, [ = ]() { saveSettings(); });
    connect(hdr, &QHeaderView::sortIndicatorChanged, this, [ = ]() { saveSettings(); });
    connect(hdr, &QHeaderView::customContextMenuRequested, this,
            &SystemServiceTableView::displayHeaderContextMenu);

    auto *mwnd = MainWindow::instance();
    Q_ASSERT(mwnd != nullptr);
    connect(mwnd->toolbar(), &Toolbar::search, this, &SystemServiceTableView::search);

    connect(m_startServiceAction, &QAction::triggered, this, &SystemServiceTableView::startService);
    connect(m_stopServiceAction, &QAction::triggered, this, &SystemServiceTableView::stopService);
    connect(m_restartServiceAction, &QAction::triggered, this, &SystemServiceTableView::restartService);
    connect(m_setAutoStartAction, &QAction::triggered, this, [ = ]() { setServiceStartupMode(true); });
    connect(m_setManualStartAction, &QAction::triggered, this, [ = ]() { setServiceStartupMode(false); });
    connect(m_refreshAction, &QAction::triggered, this, &SystemServiceTableView::refresh);

    connect(m_contextMenu, &QMenu::aboutToShow, this, [ = ]() {
        if (selectionModel()->selectedRows().size() > 0) {
            // proxy index
            auto index = selectionModel()->selectedRows()[0];
            if (index.isValid()) {
                auto state = m_model->getUnitFileState(m_proxyModel->mapToSource(index));
                auto sname = m_model->getUnitFileName(m_proxyModel->mapToSource(index));
                if (sname.endsWith("@") || isUnitNoOp(state)) {
                    m_setServiceStartupModeMenu->setEnabled(false);
                } else {
                    m_setServiceStartupModeMenu->setEnabled(true);
                }

                if (isServiceAutoStartup(sname, state)) {
                    m_setAutoStartAction->setEnabled(false);
                    m_setManualStartAction->setEnabled(true);
                } else {
                    m_setAutoStartAction->setEnabled(true);
                    m_setManualStartAction->setEnabled(false);
                }
            }
        }
    });

    connect(m_loadStateHeaderAction, &QAction::triggered, this, [ = ](bool b) {
        hdr->setSectionHidden(SystemServiceTableModel::kSystemServiceLoadStateColumn, !b);
        saveSettings();
    });
    connect(m_activeStateHeaderAction, &QAction::triggered, this, [ = ](bool b) {
        hdr->setSectionHidden(SystemServiceTableModel::kSystemServiceActiveStateColumn, !b);
        saveSettings();
    });
    connect(m_subStateHeaderAction, &QAction::triggered, this, [ = ](bool b) {
        hdr->setSectionHidden(SystemServiceTableModel::kSystemServiceSubStateColumn, !b);
        saveSettings();
    });
    connect(m_stateHeaderAction, &QAction::triggered, this, [ = ](bool b) {
        hdr->setSectionHidden(SystemServiceTableModel::kSystemServiceStateColumn, !b);
        saveSettings();
    });
    connect(m_descriptionHeaderAction, &QAction::triggered, this, [ = ](bool b) {
        hdr->setSectionHidden(SystemServiceTableModel::kSystemServiceDescriptionColumn, !b);
        saveSettings();
    });
    connect(m_pidHeaderAction, &QAction::triggered, this, [ = ](bool b) {
        hdr->setSectionHidden(SystemServiceTableModel::kSystemServicePIDColumn, !b);
        saveSettings();
    });
    connect(m_startupModeHeaderAction, &QAction::triggered, this, [ = ](bool b) {
        hdr->setSectionHidden(SystemServiceTableModel::kSystemServiceStartupModeColumn, !b);
        saveSettings();
    });

    connect(m_headerContextMenu, &QMenu::aboutToShow, this, [ = ]() {
        bool b;
        b = hdr->isSectionHidden(SystemServiceTableModel::kSystemServiceLoadStateColumn);
        m_loadStateHeaderAction->setChecked(!b);
        b = hdr->isSectionHidden(SystemServiceTableModel::kSystemServiceActiveStateColumn);
        m_activeStateHeaderAction->setChecked(!b);
        b = hdr->isSectionHidden(SystemServiceTableModel::kSystemServiceSubStateColumn);
        m_subStateHeaderAction->setChecked(!b);
        b = hdr->isSectionHidden(SystemServiceTableModel::kSystemServiceStateColumn);
        m_stateHeaderAction->setChecked(!b);
        b = hdr->isSectionHidden(SystemServiceTableModel::kSystemServiceStartupModeColumn);
        m_startupModeHeaderAction->setChecked(!b);
        b = hdr->isSectionHidden(SystemServiceTableModel::kSystemServiceDescriptionColumn);
        m_descriptionHeaderAction->setChecked(!b);
        b = hdr->isSectionHidden(SystemServiceTableModel::kSystemServicePIDColumn);
        m_pidHeaderAction->setChecked(!b);
    });

    connect(m_refreshKP, &QShortcut::activated, this, &SystemServiceTableView::refresh);
    connect(m_startKP, &QShortcut::activated, this, &SystemServiceTableView::startService);
    connect(m_stopKP, &QShortcut::activated, this, &SystemServiceTableView::stopService);
    connect(m_restartKP, &QShortcut::activated, this, &SystemServiceTableView::restartService);

    // initialize service list
    auto *tbar = mwnd->toolbar();
    connect(tbar, &Toolbar::serviceTabButtonClicked, this, [ = ]() {
        if (!defer_initialized) {
            auto *mgr = ServiceManager::instance();
            mgr->updateServiceList();
            defer_initialized = true;
        }
    });

    auto *mgr = ServiceManager::instance();
    Q_ASSERT(mgr != nullptr);
    connect(mgr, &ServiceManager::errorOccurred, this, [ = ](const ErrorContext & ec) {
        if (ec) {
            if (ec.getErrorMessage() != "Permission denied")
                ErrorDialog::show(this, ec.getErrorName(), ec.getErrorMessage());
        }
    });
    connect(mgr, &ServiceManager::beginUpdateList, this, [ = ]() {
        m_loading = true;
        auto *mwnd = MainWindow::instance();
        Q_EMIT mwnd->loadingStatusChanged(m_loading);

        setEnabled(false);

        m_noMatchingResultLabel->hide();
        m_spinner->start();
        m_spinner->show();
    });
    connect(m_model, &SystemServiceTableModel::modelReset, this, [ = ]() {
        setEnabled(true);

        m_noMatchingResultLabel->hide();
        m_spinner->hide();
        m_spinner->stop();

        m_loading = false;
        auto *mwnd = MainWindow::instance();
        Q_EMIT mwnd->loadingStatusChanged(m_loading);
    });

    connect(m_model, &SystemServiceTableModel::currentRowChanged, this,
    [ = ](int row) {
        m_proxyModel->fetchMore({});
        auto *selmo = selectionModel();
        selmo->select(m_proxyModel->mapFromSource(m_model->index(row, 0)),
                      QItemSelectionModel::SelectCurrent |
                      QItemSelectionModel::Rows |
                      QItemSelectionModel::Clear);
    });

    connect(m_proxyModel, &SystemServiceTableModel::rowsInserted, this,
    [ = ]() {
        adjustInfoLabelVisibility();
    });
    connect(m_proxyModel, &SystemServiceTableModel::rowsRemoved, this,
    [ = ]() {
        adjustInfoLabelVisibility();
    });
}

int SystemServiceTableView::sizeHintForColumn(int column) const
{
    QStyleOptionHeader option;
    option.initFrom(this);
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    return std::max(header()->sizeHintForColumn(column) + margin * 2,
                    BaseTableView::sizeHintForColumn(column) + margin * 2);
}

void SystemServiceTableView::refresh()
{
    if (m_loading)
        return;

    m_model->reset();

    auto *mgr = ServiceManager::instance();
    Q_ASSERT(mgr != nullptr);
    mgr->updateServiceList();
}

bool SystemServiceTableView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this) {
        if (event->type() == QEvent::KeyPress) {
            auto *kev = dynamic_cast<QKeyEvent *>(event);
            if (kev->modifiers() & Qt::CTRL && kev->key() == Qt::Key_M) {
                if (this->hasFocus()) {
                    if (selectedIndexes().size() > 0) {
                        auto index = selectedIndexes()[0];
                        auto rect = visualRect(index);
                        displayTableContextMenu({rect.x() + rect.width() / 2,
                                                 rect.y() + rect.height() / 2});
                        return true;
                    }
                } else if (header()->hasFocus()) {
                    displayHeaderContextMenu({
                        header()->sectionSize(header()->logicalIndexAt(0)) / 2,
                        header()->height() / 2});
                    return true;
                }
            }
        }
    }
    return BaseTableView::eventFilter(obj, event);
}
