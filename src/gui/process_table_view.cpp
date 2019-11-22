#include <DApplication>
#include <DApplicationHelper>
#include <DDesktopServices>
#include <DDialog>
#include <DFrame>
#include <DLabel>
#include <DMenu>
#include <DPalette>
#include <DTitlebar>
#include <DWidget>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>

#include "kill_process_confirm_dialog.h"
#include "main_window.h"
#include "model/process_sort_filter_proxy_model.h"
#include "model/process_table_model.h"
#include "process/process_entry.h"
#include "process/system_monitor.h"
#include "process_attribute_dialog.h"
#include "process_item_delegate.h"
#include "process_table_header_view.h"
#include "process_table_view.h"
#include "settings.h"
#include "toolbar.h"

DWIDGET_USE_NAMESPACE

static const char *kSettingsOption_ProcessTableHeaderState = "process_table_header_state";

ProcessTableView::ProcessTableView(DWidget *parent)
    : DTreeView(parent)
{
    bool settingsLoaded = loadSettings();

    m_model = new ProcessTableModel(this);
    m_proxyModel = new ProcessSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    setModel(m_proxyModel);

    initUI(settingsLoaded);
    initConnections(settingsLoaded);
}

ProcessTableView::~ProcessTableView()
{
    saveSettings();
}

void ProcessTableView::endProcess()
{
    // dialog
    QString title = DApplication::translate("Kill.Process.Dialog", "End process");
    QString description = DApplication::translate("Kill.Process.Dialog",
                                                  "Ending this process may cause data "
                                                  "loss.\nAre you sure you want to continue?");

    KillProcessConfirmDialog dialog(this);
    dialog.setTitle(title);
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
    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        sysmon->pauseProcess(qvariant_cast<pid_t>(m_selectedPID));
    }
}

void ProcessTableView::resumeProcess()
{
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
        auto *attr = new ProcessAttributeDialog(qvariant_cast<pid_t>(m_selectedPID), this);
        attr->show();
    }
}

void ProcessTableView::killProcess()
{
    // dialog
    QString title = DApplication::translate("Kill.Process.Dialog", "End process");
    QString description = DApplication::translate("Kill.Process.Dialog",
                                                  "Force ending this process may cause data "
                                                  "loss.\nAre you sure you want to continue?");

    KillProcessConfirmDialog dialog(this);
    dialog.setTitle(title);
    dialog.setMessage(description);
    dialog.addButton(DApplication::translate("Kill.Process.Dialog", "Cancel"), false);
    dialog.addButton(DApplication::translate("Kill.Process.Dialog", "Force end"), true,
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
    m_notFoundLabel = new DLabel(DApplication::translate("Common.Search", "Not Found"), this);
    QFont font = m_notFoundLabel->font();
    font.setPointSize(20);
    m_notFoundLabel->setFont(font);
    auto palette = DApplicationHelper::instance()->palette(m_notFoundLabel);
    palette.setColor(DPalette::Text, palette.color(DPalette::TextTips));
    m_notFoundLabel->setPalette(palette);
    m_notFoundLabel->setVisible(false);

    m_itemDelegate = new ProcessItemDelegate(this);
    setItemDelegate(m_itemDelegate);

    // header options
    m_headerView = new ProcessTableHeaderView(Qt::Horizontal, this);
    setHeader(m_headerView);
    m_headerView->setSectionsMovable(true);
    m_headerView->setSectionsClickable(true);
    m_headerView->setSectionResizeMode(DHeaderView::Interactive);
    m_headerView->setStretchLastSection(true);
    m_headerView->setSortIndicatorShown(true);
    m_headerView->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_headerView->setContextMenuPolicy(Qt::CustomContextMenu);

    // table options
    setSortingEnabled(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setFrameStyle(QFrame::NoFrame);
    viewport()->setAutoFillBackground(false);
    setAlternatingRowColors(false);
    setContextMenuPolicy(Qt::CustomContextMenu);

    m_contextMenu = new DMenu(this);
    m_headerContextMenu = new DMenu(this);

    if (!settingsLoaded) {
        // proc name
        setColumnWidth(ProcessTableModel::kProcessNameColumn, 300);
        setColumnHidden(ProcessTableModel::kProcessNameColumn, false);

        // cpu
        setColumnWidth(ProcessTableModel::kProcessCPUColumn, 70);
        setColumnHidden(ProcessTableModel::kProcessCPUColumn, false);

        // account
        setColumnWidth(ProcessTableModel::kProcessUserColumn, 80);
        setColumnHidden(ProcessTableModel::kProcessUserColumn, false);

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
    connect(endProcAction, &QAction::triggered, this, &ProcessTableView::endProcess);
    // pause process
    auto *pauseProcAction = m_contextMenu->addAction(
        DApplication::translate("Process.Table.Context.Menu", "Pause process"));
    connect(pauseProcAction, &QAction::triggered, this, &ProcessTableView::pauseProcess);
    // resume process
    auto *resumeProcAction = m_contextMenu->addAction(
        DApplication::translate("Process.Table.Context.Menu", "Resume process"));
    connect(resumeProcAction, &QAction::triggered, this, &ProcessTableView::resumeProcess);
    // show exec location
    auto *openExecDirAction = m_contextMenu->addAction(
        DApplication::translate("Process.Table.Context.Menu", "View command location"));
    connect(openExecDirAction, &QAction::triggered, this, &ProcessTableView::openExecDirWithFM);
    // show property
    auto *showAttrAction = m_contextMenu->addAction(
        DApplication::translate("Process.Table.Context.Menu", "Properties"));
    connect(showAttrAction, &QAction::triggered, this, &ProcessTableView::showProperties);
    m_contextMenu->addSeparator();
    // kill process
    auto *killProcAction = m_contextMenu->addAction(
        DApplication::translate("Process.Table.Context.Menu", "Kill process"));
    connect(killProcAction, &QAction::triggered, this, &ProcessTableView::killProcess);

    connect(m_contextMenu, &DMenu::aboutToShow, this, [=]() {
        bool b = true;
        if (m_selectedPID.isValid()) {
            char state = m_model->getProcessState(qvariant_cast<pid_t>(m_selectedPID));
            if (state == 'T') {
                b = false;
            } else if (state == 'R') {
                b = true;
            }
            pauseProcAction->setEnabled(b);
            resumeProcAction->setEnabled(!b);
        }
    });

    // header
    auto *h = header();
    connect(h, &ProcessTableHeaderView::sectionResized, this, [this]() { saveSettings(); });
    connect(h, &ProcessTableHeaderView::sectionMoved, this, [&]() { saveSettings(); });
    connect(h, &ProcessTableHeaderView::sortIndicatorChanged, this, [&]() { saveSettings(); });
    connect(h, &ProcessTableHeaderView::customContextMenuRequested, this,
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
    // TODO: user
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

    if (!settingsLoaded) {
        cpuHeaderAction->setChecked(true);
        memHeaderAction->setChecked(true);
        uploadHeaderAction->setChecked(true);
        downloadHeaderAction->setChecked(true);
        dreadHeaderAction->setChecked(false);
        dwriteHeaderAction->setChecked(false);
        pidHeaderAction->setChecked(true);
    }
    connect(m_headerContextMenu, &QMenu::aboutToShow, this, [=]() {
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
}

void ProcessTableView::displayProcessTableContextMenu(const QPoint &p)
{
    if (selectedIndexes().size() == 0)
        return;

    QPoint point = mapToGlobal(p);
    point.setY(point.y() +
               model()->headerData(0, Qt::Horizontal, Qt::SizeHintRole).toSize().height());
    // TODO: context menu enable status rule
    m_contextMenu->popup(point);
}

void ProcessTableView::displayProcessTableHeaderContextMenu(const QPoint &p)
{
    m_headerContextMenu->popup(mapToGlobal(p));
}

void ProcessTableView::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    adjustInfoLabelVisibility();

    DTreeView::resizeEvent(event);
}

void ProcessTableView::paintEvent(QPaintEvent *event)
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

void ProcessTableView::selectionChanged(const QItemSelection &selected,
                                        const QItemSelection &deselected)
{
    if (selected.size() <= 0) {
        return;
    }

    m_selectedPID = selected.indexes().value(ProcessTableModel::kProcessPIDColumn).data();

    DTreeView::selectionChanged(selected, deselected);
}

void ProcessTableView::adjustInfoLabelVisibility()
{
    setUpdatesEnabled(false);
    m_notFoundLabel->setVisible(m_proxyModel->rowCount() == 0);
    if (m_notFoundLabel->isVisible())
        m_notFoundLabel->move(rect().center() - m_notFoundLabel->rect().center());
    setUpdatesEnabled(true);
}
