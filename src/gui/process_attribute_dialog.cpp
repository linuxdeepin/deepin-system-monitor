#include <proc/readproc.h>
#include <proc/sysinfo.h>

#include <DApplication>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DFrame>
#include <DLabel>
#include <DTitlebar>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QVBoxLayout>

#include "process_attribute_dialog.h"
#include "settings.h"
#include "utils.h"

using namespace Utils;

ProcessAttributeDialog::ProcessAttributeDialog(pid_t pid, QWidget *parent)
    : DMainWindow(parent)
    , m_pid(pid)
{
    m_settings = Settings::instance();

    initUI();

    installEventFilter(this);
}

void ProcessAttributeDialog::initUI()
{
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    int width = 600, height = 480;
    if (m_settings) {
        QVariant v;
        v = m_settings->getOption(kSettingKeyProcessAttributeDialogWidth);
        if (v.isValid()) {
            width = v.toInt();
        }
        v = m_settings->getOption(kSettingKeyProcessAttributeDialogHeight);
        if (v.isValid()) {
            height = v.toInt();
        }
    }

    m_frame = new DWidget(this);
    m_frame->setAutoFillBackground(true);

    resize(width, height);
    setMinimumSize(600, 480);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::WindowModal);
    setWindowFlags(Qt::Dialog);
    titlebar()->setQuitMenuDisabled(true);
    titlebar()->setMenuVisible(false);
    setTitlebarShadowEnabled(false);
    titlebar()->setTitle({});

    m_tbShadow = new DShadowLine(m_frame);
    m_tbShadow->setFixedWidth(m_frame->width());
    m_tbShadow->setFixedHeight(10);
    m_tbShadow->move(0, 0);
    m_tbShadow->raise();
    m_tbShadow->show();

    auto *grid = new QGridLayout(m_frame);
    grid->setContentsMargins(margin, 0, margin, 0);
    grid->setVerticalSpacing(0);
    grid->setHorizontalSpacing(margin);

    // spacing
    grid->addItem(new QSpacerItem(10, 10), 0, 0, 1, 2);
    grid->setRowStretch(0, 1);

    DLabel *appIcon = new DLabel(this);
    grid->addWidget(appIcon, 1, 0, 1, 2, Qt::AlignVCenter | Qt::AlignHCenter);

    // spacing
    grid->addItem(new QSpacerItem(10, 20), 2, 0, 1, 2);
    grid->setRowStretch(2, 0);

    auto *appNameLabel = new DLabel(this);
    DFontSizeManager::instance()->bind(appNameLabel, DFontSizeManager::T5, QFont::Bold);
    grid->addWidget(appNameLabel, 3, 0, 1, 2, Qt::AlignHCenter | Qt::AlignVCenter);

    // spacing
    grid->addItem(new QSpacerItem(10, 40), 4, 0, 1, 2);
    grid->setRowStretch(4, 0);

    QString buf;
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Process name"));
    auto *procNameLabel = new DLabel(this);
    procNameLabel->setText(buf);
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Command line"));
    auto *procCmdLabel = new DLabel(this);
    procCmdLabel->setText(buf);
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Start time"));
    auto *procStartLabel = new DLabel(this);
    procStartLabel->setText(buf);

    m_procNameText = new DLabel(this);
    m_procNameText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_procCmdText = new DLabel(this);
    m_procCmdText->setWordWrap(true);
    m_procCmdText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_procStartText = new DLabel(this);
    m_procStartText->setTextInteractionFlags(Qt::TextSelectableByMouse);

    grid->addWidget(procNameLabel, 5, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(m_procNameText, 5, 1, Qt::AlignTop | Qt::AlignLeft);

    grid->addWidget(procCmdLabel, 6, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(m_procCmdText, 6, 1, Qt::AlignTop | Qt::AlignLeft);

    grid->addWidget(procStartLabel, 7, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(m_procStartText, 7, 1, Qt::AlignTop | Qt::AlignLeft);

    // spacing
    grid->addItem(new QSpacerItem(10, 10), 8, 0, 1, 2);
    grid->setRowStretch(8, 1);

    // Read the list of open processes information.
    PROCTAB *proc =
        openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLUSR | PROC_FILLCOM);
    static proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_t));

    std::map<int, proc_t> processes;
    while (readproc(proc, &proc_info) != nullptr) {
        processes[proc_info.tid] = proc_info;
    }
    closeproc(proc);

    QScopedPointer<FindWindowTitle> findWindowTitle(new FindWindowTitle());
    findWindowTitle->updateWindowInfos();

    // Read tray icon process.
    QList<int> trayProcessXids = Utils::getTrayWindows();
    QMap<int, int> trayProcessMap;

    for (int xid : trayProcessXids) {
        trayProcessMap[findWindowTitle->getWindowPid(static_cast<xcb_window_t>(xid))] = xid;
    }

    for (auto &i : processes) {
        int processId = (&i.second)->tid;

        if (m_pid == processId) {
            QString cmdline = Utils::getProcessCmdline(processId);
            QString name = getProcessName(&i.second, cmdline);
            std::string desktopFile = getProcessDesktopFile(m_pid, name, cmdline, trayProcessMap);
            QPixmap icon = getProcessIcon(m_pid, desktopFile, findWindowTitle, 80);
            QString displayName = getDisplayNameFromName(name, desktopFile, false);

            appIcon->setPixmap(icon);
            appNameLabel->setText(displayName);

            m_procNameText->setText(name);
            m_procCmdText->setText(cmdline);
            m_procStartText->setText(QFileInfo(QString("/proc/%1").arg(processId))
                                         .created()
                                         .toString("yyyy-MM-dd hh:mm:ss"));
            break;
        }
    }

    m_frame->setLayout(grid);
    setCentralWidget(m_frame);
}

void ProcessAttributeDialog::resizeEvent(QResizeEvent *event)
{
    if (m_settings) {
        m_settings->setOption(kSettingKeyProcessAttributeDialogWidth, width());
        m_settings->setOption(kSettingKeyProcessAttributeDialogHeight, height());
    }

    if (m_frame) {
        m_tbShadow->setFixedWidth(m_frame->size().width());
        m_tbShadow->raise();
        m_tbShadow->show();

        // compare qfontmetrics vs maxwidth
        m_procCmdText->setFixedWidth(m_frame->width() - 120);
    }

    DMainWindow::resizeEvent(event);
}

void ProcessAttributeDialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    if (m_settings) {
        m_settings->setOption(kSettingKeyProcessAttributeDialogWidth, width());
        m_settings->setOption(kSettingKeyProcessAttributeDialogHeight, height());
    }
}

bool ProcessAttributeDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *kev = dynamic_cast<QKeyEvent *>(event);
        if (kev->matches(QKeySequence::Cancel)) {
            close();
            return true;
        } else {
            return false;
        }
    } else {
        return DMainWindow::eventFilter(obj, event);
    }
}
