#include "errno.h"

#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QKeyEvent>

#include <DApplication>
#include <DApplicationHelper>
#include <DDesktopServices>
#include <DDialog>
#include <DErrorMessage>
#include <DFontSizeManager>
#include <DFrame>
#include <DLabel>
#include <DMenu>
#include <DPalette>
#include <DSlider>
#include <DTipLabel>
#include <DTitlebar>
#include <DToolTip>
#include <DWidget>

#include "kill_process_confirm_dialog.h"
#include "main_window.h"
#include "model/process_sort_filter_proxy_model.h"
#include "model/process_table_model.h"
#include "priority_slider.h"
#include "process/process_entry.h"
#include "process/system_monitor.h"
#include "process_attribute_dialog.h"
#include "process_table_view.h"
#include "settings.h"
#include "toolbar.h"
#include "ui_common.h"
#include "dialog/error_dialog.h"

DWIDGET_USE_NAMESPACE

static const char *kSettingsOption_ProcessTableHeaderState = "process_table_header_state";

ProcessTableView::ProcessTableView(DWidget *parent)
    : BaseTableView(parent)
{
    installEventFilter(this);

    m_model = new ProcessTableModel(this);
    m_proxyModel = new ProcessSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    // setModel must be called before calling loadSettings();
    setModel(m_proxyModel);

    bool settingsLoaded = loadSettings();

    initUI(settingsLoaded);
    initConnections(settingsLoaded);

    auto *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, [ = ]() {
        if (m_notFoundLabel) {
            auto palette = DApplicationHelper::instance()->applicationPalette();
            QColor labelColor = palette.color(DPalette::PlaceholderText);
            palette.setColor(DPalette::Text, labelColor);
            m_notFoundLabel->setPalette(palette);
        }
    });
}

ProcessTableView::~ProcessTableView()
{
    saveSettings();
}

bool ProcessTableView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this) {
        if (event->type() == QEvent::KeyPress) {
            auto *kev = dynamic_cast<QKeyEvent *>(event);
            if (kev->modifiers() & Qt::CTRL && kev->key() == Qt::Key_M) {
                if (this->hasFocus()) {
                    if (selectedIndexes().size() > 0) {
                        auto index = selectedIndexes()[0];
                        auto rect = visualRect(index);
                        displayProcessTableContextMenu({rect.x() + rect.width() / 2, rect.y() + rect.height() / 2});
                        return true;
                    }
                } else if (header()->hasFocus()) {
                    displayProcessTableHeaderContextMenu({header()->sectionSize(header()->logicalIndexAt(0)) / 2, header()->height() / 2});
                    return true;
                }
            }
        }
    }
    return BaseTableView::eventFilter(obj, event);
}

void ProcessTableView::endProcess()
{
    if (m_selectedPID.isNull()) {
        return;
    }

    // dialog
    QString title = DApplication::translate("Kill.Process.Dialog", "End process");
    QString description = DApplication::translate("Kill.Process.Dialog",
                                                  "Ending this process may cause data "
                                                  "loss.\nAre you sure you want to continue?");

    KillProcessConfirmDialog dialog(this);
//    dialog.setTitle(title);
    dialog.setMessage(description);
    dialog.addButton(DApplication::translate("Kill.Process.Dialog", "Cancel"), false);
    dialog.addButton(DApplication::translate("Kill.Process.Dialog", "End"), true,
                     DDialog::ButtonWarning);
    dialog.exec();
    if (dialog.result() == QMessageBox::Ok) {
        auto *sysmon = SystemMonitor::instance();
        if (sysmon) {
            sysmon->endProcess(qvariant_cast<pid_t>(m_selectedPID));
        }
    } else {
        return;
    }
}

void ProcessTableView::pauseProcess()
{
    if (m_selectedPID.isNull()) {
        return;
    }

    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        sysmon->pauseProcess(qvariant_cast<pid_t>(m_selectedPID));
    }
}

void ProcessTableView::resumeProcess()
{
    if (m_selectedPID.isNull()) {
        return;
    }

    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        sysmon->resumeProcess(qvariant_cast<pid_t>(m_selectedPID));
    }
}

void ProcessTableView::openExecDirWithFM()
{
    if (m_selectedPID.isValid()) {
        pid_t pid = qvariant_cast<pid_t>(m_selectedPID);
        QString cmdline = Utils::getProcessCmdline(qvariant_cast<pid_t>(m_selectedPID));

        if (cmdline.size() > 0) {
            // Found wine program location if cmdline starts with c://.
            if (cmdline.startsWith("c:\\")) {
                QString winePrefix = Utils::getProcessEnvironmentVariable(pid, "WINEPREFIX");
                cmdline = cmdline.replace("\\", "/").replace("c:/", "/drive_c/");

                DDesktopServices::showFileItem(winePrefix + cmdline);
            } else {
                QString flatpakAppidEnv =
                    Utils::getProcessEnvironmentVariable(pid, "FLATPAK_APPID");

                // Else find program location through 'which' command.
                if (flatpakAppidEnv == "") {
                    QProcess whichProcess;
                    QString exec = "which";
                    QStringList params;
                    params << cmdline.split(" ");
                    whichProcess.start(exec, params);
                    whichProcess.waitForFinished();
                    QString output(whichProcess.readAllStandardOutput());

                    QString processPath = output.split("\n")[0];
                    DDesktopServices::showFileItem(processPath);
                }
                // Find flatpak application location.
                else {
                    QDir flatpakRootDir = Utils::getFlatpakAppPath(flatpakAppidEnv);
                    flatpakRootDir.cd("files");
                    flatpakRootDir.cd("bin");

                    // Need split full path to get last filename.
                    DDesktopServices::showFileItem(
                        flatpakRootDir.absoluteFilePath(cmdline.split("/").last()));
                }
            }
        }
    }
}

void ProcessTableView::showProperties()
{
    if (m_selectedPID.isValid()) {
        pid_t pid = qvariant_cast<pid_t>(m_selectedPID);
        auto entry = m_model->getProcessEntry(pid);
        auto *attr = new ProcessAttributeDialog(pid,
                                                entry.getName(),
                                                entry.getDisplayName(),
                                                entry.getCmdline(),
                                                entry.getIcon(),
                                                entry.getStartTime(),
                                                this);
        attr->show();
    }
}

void ProcessTableView::killProcess()
{
    if (m_selectedPID.isNull()) {
        return;
    }

    // dialog
    QString title = DApplication::translate("Kill.Process.Dialog", "End process");
    QString description = DApplication::translate("Kill.Process.Dialog",
                                                  "Force ending this process may cause data "
                                                  "loss.\nAre you sure you want to continue?");

    KillProcessConfirmDialog dialog(this);
//    dialog.setTitle(title);
    dialog.setMessage(description);
    dialog.addButton(DApplication::translate("Kill.Process.Dialog", "Cancel"), false);
    dialog.addButton(DApplication::translate("Kill.Process.Dialog", "Force End"), true,
                     DDialog::ButtonWarning);
    dialog.exec();
    if (dialog.result() == QMessageBox::Ok) {
        auto *sysmon = SystemMonitor::instance();
        if (sysmon) {
            sysmon->killProcess(qvariant_cast<pid_t>(m_selectedPID));
        }
    } else {
        return;
    }
}

void ProcessTableView::search(const QString &text)
{
    m_proxyModel->setSortFilterString(text);
    adjustInfoLabelVisibility();
}

void ProcessTableView::switchDisplayMode(SystemMonitor::FilterType type)
{
    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        sysmon->setFilterType(type);
    }
}

void ProcessTableView::changeProcessPriority(int priority)
{
    if (m_selectedPID.isValid()) {
        pid_t pid = qvariant_cast<pid_t>(m_selectedPID);

        auto *sysmon = SystemMonitor::instance();
        if (sysmon) {
            SystemMonitor::ProcessPriority prio = m_model->getProcessPriorityStub(pid);
            if (prio == priority)
                return;

            ErrorContext ec {};
            ec = sysmon->setProcessPriority(pid, priority);
            if (ec) {
                // show error dialog
                ErrorDialog::show(this, ec.getErrorName(), ec.getErrorMessage());
            }
        }
    }
}

bool ProcessTableView::loadSettings()
{
    Settings *s = Settings::instance();
    if (s) {
        QVariant opt = s->getOption(kSettingsOption_ProcessTableHeaderState);
        if (opt.isValid()) {
            QByteArray buf = QByteArray::fromBase64(opt.toByteArray());
            header()->restoreState(buf);
            return true;
        }
    }
    return false;
}

void ProcessTableView::saveSettings()
{
    Settings *s = Settings::instance();
    if (s) {
        QByteArray buf = header()->saveState();
        s->setOption(kSettingsOption_ProcessTableHeaderState, buf.toBase64());
        s->flush();
    }
}

void ProcessTableView::initUI(bool settingsLoaded)
{
    // not found display label
    m_notFoundLabel = new DLabel(DApplication::translate("Common.Search", "No search results"), this);
    DFontSizeManager::instance()->bind(m_notFoundLabel, DFontSizeManager::T4);
    auto palette = DApplicationHelper::instance()->palette(m_notFoundLabel);
    QColor labelColor = palette.color(DPalette::PlaceholderText);
    palette.setColor(DPalette::Text, labelColor);
    m_notFoundLabel->setPalette(palette);
    m_notFoundLabel->setVisible(false);

    // header options
    header()->setSectionsMovable(true);
    header()->setSectionsClickable(true);
    header()->setSectionResizeMode(DHeaderView::Interactive);
    header()->setStretchLastSection(true);
    header()->setSortIndicatorShown(true);
    header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);

    // table options
    setSortingEnabled(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setContextMenuPolicy(Qt::CustomContextMenu);

    m_contextMenu = new DMenu(this);
    m_headerContextMenu = new DMenu(this);

    if (!settingsLoaded) {
        // proc name
        setColumnWidth(ProcessTableModel::kProcessNameColumn, 300);
        setColumnHidden(ProcessTableModel::kProcessNameColumn, false);

        // cpu
        setColumnWidth(ProcessTableModel::kProcessCPUColumn, 100);
        setColumnHidden(ProcessTableModel::kProcessCPUColumn, false);

        // account
        setColumnWidth(ProcessTableModel::kProcessUserColumn, 80);
        setColumnHidden(ProcessTableModel::kProcessUserColumn, true);

        // memory
        setColumnWidth(ProcessTableModel::kProcessMemoryColumn, 70);
        setColumnHidden(ProcessTableModel::kProcessMemoryColumn, false);

        // download
        setColumnWidth(ProcessTableModel::kProcessDownloadColumn, 70);
        setColumnHidden(ProcessTableModel::kProcessDownloadColumn, false);

        // upload
        setColumnWidth(ProcessTableModel::kProcessUploadColumn, 70);
        setColumnHidden(ProcessTableModel::kProcessUploadColumn, false);

        // disk read
        setColumnWidth(ProcessTableModel::kProcessDiskReadColumn, 80);
        setColumnHidden(ProcessTableModel::kProcessDiskReadColumn, true);

        // disk write
        setColumnWidth(ProcessTableModel::kProcessDiskWriteColumn, 80);
        setColumnHidden(ProcessTableModel::kProcessDiskWriteColumn, true);

        // pid
        setColumnWidth(ProcessTableModel::kProcessPIDColumn, 70);
        setColumnHidden(ProcessTableModel::kProcessPIDColumn, false);

        // nice
        setColumnWidth(ProcessTableModel::kProcessNiceColumn, 100);
        setColumnHidden(ProcessTableModel::kProcessNiceColumn, true);

        // priority
        setColumnWidth(ProcessTableModel::kProcessPriorityColumn, 100);
        setColumnHidden(ProcessTableModel::kProcessPriorityColumn, true);

        //sort
        sortByColumn(ProcessTableModel::kProcessCPUColumn, Qt::DescendingOrder);
    }
}

void ProcessTableView::initConnections(bool settingsLoaded)
{
    MainWindow *mainWindow = MainWindow::instance();
    connect(mainWindow->toolbar(), &Toolbar::search, this, &ProcessTableView::search);

    // table context menu
    connect(this, &ProcessTableView::customContextMenuRequested, this,
            &ProcessTableView::displayProcessTableContextMenu);
    // end process
    auto *endProcAction = m_contextMenu->addAction(
                              DApplication::translate("Process.Table.Context.Menu", "End process"));
    endProcAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_E));
    connect(endProcAction, &QAction::triggered, this, &ProcessTableView::endProcess);
    // pause process
    auto *pauseProcAction = m_contextMenu->addAction(
                                DApplication::translate("Process.Table.Context.Menu", "Suspend process"));
    pauseProcAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_P));
    connect(pauseProcAction, &QAction::triggered, this, &ProcessTableView::pauseProcess);
    // resume process
    auto *resumeProcAction = m_contextMenu->addAction(
                                 DApplication::translate("Process.Table.Context.Menu", "Resume process"));
    resumeProcAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_C));
    connect(resumeProcAction, &QAction::triggered, this, &ProcessTableView::resumeProcess);

    // change priority dialog
    auto *chgProcPrioMenu = m_contextMenu->addMenu(
                                DApplication::translate("Process.Table.Context.Menu", "Change priority"));
    QActionGroup *prioGroup = new QActionGroup(chgProcPrioMenu);
    prioGroup->setExclusive(true);

    auto *setVeryHighPrioAction =
        chgProcPrioMenu->addAction(DApplication::translate("Process.Priority", "Very high"));
    setVeryHighPrioAction->setCheckable(true);
    setVeryHighPrioAction->setActionGroup(prioGroup);
    connect(setVeryHighPrioAction, &QAction::triggered,
    [ = ]() { changeProcessPriority(SystemMonitor::kVeryHighPriority); });

    auto *setHighPrioAction =
        chgProcPrioMenu->addAction(DApplication::translate("Process.Priority", "High"));
    setHighPrioAction->setCheckable(true);
    setHighPrioAction->setActionGroup(prioGroup);
    connect(setHighPrioAction, &QAction::triggered,
    [ = ]() { changeProcessPriority(SystemMonitor::kHighPriority); });

    auto *setNormalPrioAction =
        chgProcPrioMenu->addAction(DApplication::translate("Process.Priority", "Normal"));
    setNormalPrioAction->setCheckable(true);
    setNormalPrioAction->setActionGroup(prioGroup);
    connect(setNormalPrioAction, &QAction::triggered,
    [ = ]() { changeProcessPriority(SystemMonitor::kNormalPriority); });

    auto *setLowPrioAction =
        chgProcPrioMenu->addAction(DApplication::translate("Process.Priority", "Low"));
    setLowPrioAction->setCheckable(true);
    setLowPrioAction->setActionGroup(prioGroup);
    connect(setLowPrioAction, &QAction::triggered,
    [ = ]() { changeProcessPriority(SystemMonitor::kLowPriority); });

    auto *setVeryLowPrioAction =
        chgProcPrioMenu->addAction(DApplication::translate("Process.Priority", "Very low"));
    setVeryLowPrioAction->setCheckable(true);
    setVeryLowPrioAction->setActionGroup(prioGroup);
    connect(setVeryLowPrioAction, &QAction::triggered,
    [ = ]() { changeProcessPriority(SystemMonitor::kVeryLowPriority); });

    auto *setCustomPrioAction =
        chgProcPrioMenu->addAction(DApplication::translate("Process.Priority", "Custom"));
    setCustomPrioAction->setCheckable(true);
    setCustomPrioAction->setActionGroup(prioGroup);
    connect(setCustomPrioAction, &QAction::triggered, [ = ]() { customizeProcessPriority(); });

    // show exec location
    auto *openExecDirAction = m_contextMenu->addAction(
                                  DApplication::translate("Process.Table.Context.Menu", "View command location"));
    connect(openExecDirAction, &QAction::triggered, this, &ProcessTableView::openExecDirWithFM);
    // show property
    auto *showAttrAction = m_contextMenu->addAction(
                               DApplication::translate("Process.Table.Context.Menu", "Properties"));
    showAttrAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Enter));
    connect(showAttrAction, &QAction::triggered, this, &ProcessTableView::showProperties);
    m_contextMenu->addSeparator();
    // kill process
    auto *killProcAction = m_contextMenu->addAction(
                               DApplication::translate("Process.Table.Context.Menu", "Kill process"));
    killProcAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_K));
    connect(killProcAction, &QAction::triggered, this, &ProcessTableView::killProcess);

    connect(m_contextMenu, &DMenu::aboutToShow, this, [ = ]() {
        bool b = true;
        auto *sysmon = SystemMonitor::instance();
        if (m_selectedPID.isValid()) {
            pid_t pid = qvariant_cast<pid_t>(m_selectedPID);
            char state = m_model->getProcessState(pid);
            if (state == 'T') {
                b = false;
            } else if (state == 'R') {
                b = true;
            }
            bool b2 = b;
            if (sysmon) {
                b2 = b2 & !sysmon->isSelfProcess(pid);
            }
            pauseProcAction->setEnabled(b2);
            resumeProcAction->setEnabled(!b);

            // initialize priority menu item checkable state
            SystemMonitor::ProcessPriority prio = m_model->getProcessPriorityStub(pid);
            switch (prio) {
            case SystemMonitor::kVeryHighPriority: {
                setVeryHighPrioAction->setChecked(true);
            } break;
            case SystemMonitor::kHighPriority: {
                setHighPrioAction->setChecked(true);
            } break;
            case SystemMonitor::kNormalPriority: {
                setNormalPrioAction->setChecked(true);
            } break;
            case SystemMonitor::kLowPriority: {
                setLowPrioAction->setChecked(true);
            } break;
            case SystemMonitor::kVeryLowPriority: {
                setVeryLowPrioAction->setChecked(true);
            } break;
            case SystemMonitor::kCustomPriority: {
                setCustomPrioAction->setChecked(true);
            } break;
            default: {
                // unexpected, do nothing
            }
            }
        }
    });

    // header
    auto *h = header();
    connect(h, &QHeaderView::sectionResized, this, [ = ]() { saveSettings(); });
    connect(h, &QHeaderView::sectionMoved, this, [ = ]() { saveSettings(); });
    connect(h, &QHeaderView::sortIndicatorChanged, this, [ = ]() { saveSettings(); });
    connect(h, &QHeaderView::customContextMenuRequested, this,
            &ProcessTableView::displayProcessTableHeaderContextMenu);

    // header context menu
    // cpu
    auto *cpuHeaderAction = m_headerContextMenu->addAction(
                                DApplication::translate("Process.Table.Header", kProcessCPU));
    cpuHeaderAction->setCheckable(true);
    connect(cpuHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(ProcessTableModel::kProcessCPUColumn, !b);
        saveSettings();
    });
    // user
    auto *userHeaderAction = m_headerContextMenu->addAction(
                                 DApplication::translate("Process.Table.Header", kProcessUser));
    userHeaderAction->setCheckable(true);
    connect(userHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(ProcessTableModel::kProcessUserColumn, !b);
        saveSettings();
    });
    // memory
    auto *memHeaderAction = m_headerContextMenu->addAction(
                                DApplication::translate("Process.Table.Header", kProcessMemory));
    memHeaderAction->setCheckable(true);
    connect(memHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(ProcessTableModel::kProcessMemoryColumn, !b);
        saveSettings();
    });
    // upload
    auto *uploadHeaderAction = m_headerContextMenu->addAction(
                                   DApplication::translate("Process.Table.Header", kProcessUpload));
    uploadHeaderAction->setCheckable(true);
    connect(uploadHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(ProcessTableModel::kProcessUploadColumn, !b);
        saveSettings();
    });
    // download
    auto *downloadHeaderAction = m_headerContextMenu->addAction(
                                     DApplication::translate("Process.Table.Header", kProcessDownload));
    downloadHeaderAction->setCheckable(true);
    connect(downloadHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(ProcessTableModel::kProcessDownloadColumn, !b);
        saveSettings();
    });
    // disk read
    auto *dreadHeaderAction = m_headerContextMenu->addAction(
                                  DApplication::translate("Process.Table.Header", kProcessDiskRead));
    dreadHeaderAction->setCheckable(true);
    connect(dreadHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(ProcessTableModel::kProcessDiskReadColumn, !b);
        saveSettings();
    });
    // disk write
    auto *dwriteHeaderAction = m_headerContextMenu->addAction(
                                   DApplication::translate("Process.Table.Header", kProcessDiskWrite));
    dwriteHeaderAction->setCheckable(true);
    connect(dwriteHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(ProcessTableModel::kProcessDiskWriteColumn, !b);
        saveSettings();
    });
    // pid
    auto *pidHeaderAction = m_headerContextMenu->addAction(
                                DApplication::translate("Process.Table.Header", kProcessPID));
    pidHeaderAction->setCheckable(true);
    connect(pidHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(ProcessTableModel::kProcessPIDColumn, !b);
        saveSettings();
    });
    // nice
    auto *niceHeaderAction = m_headerContextMenu->addAction(
                                 DApplication::translate("Process.Table.Header", kProcessNice));
    niceHeaderAction->setCheckable(true);
    connect(niceHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(ProcessTableModel::kProcessNiceColumn, !b);
        saveSettings();
    });
    // priority
    auto *priorityHeaderAction = m_headerContextMenu->addAction(
                                     DApplication::translate("Process.Table.Header", kProcessPriority));
    priorityHeaderAction->setCheckable(true);
    connect(priorityHeaderAction, &QAction::triggered, this, [this](bool b) {
        header()->setSectionHidden(ProcessTableModel::kProcessPriorityColumn, !b);
        saveSettings();
    });

    if (!settingsLoaded) {
        cpuHeaderAction->setChecked(true);
        memHeaderAction->setChecked(true);
        uploadHeaderAction->setChecked(true);
        downloadHeaderAction->setChecked(true);
        dreadHeaderAction->setChecked(false);
        dwriteHeaderAction->setChecked(false);
        pidHeaderAction->setChecked(true);
        niceHeaderAction->setChecked(true);
        priorityHeaderAction->setChecked(true);
    }
    connect(m_headerContextMenu, &QMenu::aboutToShow, this, [ = ]() {
        bool b;
        b = header()->isSectionHidden(ProcessTableModel::kProcessCPUColumn);
        cpuHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(ProcessTableModel::kProcessMemoryColumn);
        memHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(ProcessTableModel::kProcessUploadColumn);
        uploadHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(ProcessTableModel::kProcessDownloadColumn);
        downloadHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(ProcessTableModel::kProcessDiskReadColumn);
        dreadHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(ProcessTableModel::kProcessDiskWriteColumn);
        dwriteHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(ProcessTableModel::kProcessPIDColumn);
        pidHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(ProcessTableModel::kProcessNiceColumn);
        niceHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(ProcessTableModel::kProcessPriorityColumn);
        priorityHeaderAction->setChecked(!b);
        b = header()->isSectionHidden(ProcessTableModel::kProcessUserColumn);
        userHeaderAction->setChecked(!b);
    });

    connect(m_model, &ProcessTableModel::modelUpdated, this, [&]() {
        loadSettings();
        adjustInfoLabelVisibility();
        if (m_selectedPID.isValid()) {
            for (int i = 0; i < m_proxyModel->rowCount(); i++) {
                if (m_proxyModel->data(m_proxyModel->index(i, ProcessTableModel::kProcessPIDColumn),
                                       Qt::UserRole) == m_selectedPID)
                    this->setCurrentIndex(m_proxyModel->index(i, 0));
            }
        }
    });

    m_endProcKP = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_E), this);
    connect(m_endProcKP, &QShortcut::activated, this, &ProcessTableView::endProcess);
    m_pauseProcKP = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_P), this);
    connect(m_pauseProcKP, &QShortcut::activated, this, &ProcessTableView::pauseProcess);
    m_resumeProcKP = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_C), this);
    connect(m_resumeProcKP, &QShortcut::activated, this, &ProcessTableView::resumeProcess);
    m_viewPropKP = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Return), this);
    connect(m_viewPropKP, &QShortcut::activated, this, &ProcessTableView::showProperties);
    m_killProcKP = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_K), this);
    connect(m_killProcKP, &QShortcut::activated, this, &ProcessTableView::killProcess);

    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        connect(sysmon, &SystemMonitor::priorityPromoteResultReady, this,
        [ = ](const ErrorContext & ec) {
            if (ec) {
                ErrorDialog::show(this, ec.getErrorName(), ec.getErrorMessage());
            }
        });
        connect(sysmon, &SystemMonitor::processControlResultReady, this,
        [ = ](const ErrorContext & ec) {
            if (ec) {
                ErrorDialog::show(this, ec.getErrorName(), ec.getErrorMessage());
            }
        });
    }
}

void ProcessTableView::displayProcessTableContextMenu(const QPoint &p)
{
    if (selectedIndexes().size() == 0)
        return;

    QPoint point = mapToGlobal(p);
    point.setY(point.y() + header()->sizeHint().height());
    m_contextMenu->popup(point);
}

void ProcessTableView::displayProcessTableHeaderContextMenu(const QPoint &p)
{
    m_headerContextMenu->popup(mapToGlobal(p));
}

void ProcessTableView::resizeEvent(QResizeEvent *event)
{
    adjustInfoLabelVisibility();

    DTreeView::resizeEvent(event);
}

void ProcessTableView::selectionChanged(const QItemSelection &selected,
                                        const QItemSelection &deselected)
{
    if (selected.size() <= 0) {
        return;
    }

    m_selectedPID = selected.indexes().value(ProcessTableModel::kProcessPIDColumn).data();

    DTreeView::selectionChanged(selected, deselected);
}

int ProcessTableView::sizeHintForColumn(int column) const
{
    QStyleOptionHeader option;
    option.initFrom(this);
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    return std::max(header()->sizeHintForColumn(column) + margin * 2,
                    DTreeView::sizeHintForColumn(column) + margin * 2);
}

void ProcessTableView::adjustInfoLabelVisibility()
{
    setUpdatesEnabled(false);
    m_notFoundLabel->setVisible(m_proxyModel->rowCount() == 0
                                && MainWindow::instance()->toolbar()->isSearchContentEmpty());
    if (m_notFoundLabel->isVisible())
        m_notFoundLabel->move(rect().center() - m_notFoundLabel->rect().center());
    setUpdatesEnabled(true);
}

void ProcessTableView::customizeProcessPriority()
{
    DDialog *prioDialog = new DDialog(this);
    prioDialog->setIcon(QIcon::fromTheme("dialog-warning"));
    prioDialog->setAttribute(Qt::WA_DeleteOnClose);
    prioDialog->setTitle(
        DApplication::translate("Process.Table.Custom.Priority.Dialog", "Custom Priority"));
    prioDialog->addSpacing(20);
    PrioritySlider *slider = new PrioritySlider(Qt::Horizontal, prioDialog);
    slider->slider()->setInvertedAppearance(true);
    slider->setMinimum(SystemMonitor::kVeryHighPriorityMax);
    slider->setMaximum(SystemMonitor::kVeryLowPriorityMin);
    slider->setPageStep(1);
    slider->slider()->setTracking(true);
    slider->setMouseWheelEnabled(true);
    slider->setBelowTicks({QString("%1").arg(SystemMonitor::kVeryLowPriorityMin),
                           QString("%1").arg(SystemMonitor::kVeryHighPriorityMax)});
    connect(slider, &DSlider::valueChanged,
    [ = ](int value) { slider->setTipValue(QString("%1").arg(value)); });
    QString prio {"0"};
    if (m_selectedPID.isValid()) {
        pid_t pid = qvariant_cast<pid_t>(m_selectedPID);
        slider->setValue(m_model->getProcessPriority(pid));
        prio = QString("%1").arg(slider->value());
        slider->setTipValue(prio);
    }
    prioDialog->addContent(slider);
    prioDialog->addSpacing(16);
    prioDialog->addButton(DApplication::translate("Process.Table.Custom.Priority.Dialog", "Cancel"),
                          false, DDialog::ButtonNormal);
    prioDialog->addButton(DApplication::translate("Process.Table.Custom.Priority.Dialog", "Change"),
                          true, DDialog::ButtonRecommend);
    connect(prioDialog, &DDialog::buttonClicked, this, [ = ](int index, QString text) {
        Q_UNUSED(text);
        if (index == 1) {
            changeProcessPriority(slider->value());
            prioDialog->setResult(QMessageBox::Ok);
        } else {
            prioDialog->setResult(QMessageBox::Cancel);
        }
    });
    prioDialog->exec();
}
