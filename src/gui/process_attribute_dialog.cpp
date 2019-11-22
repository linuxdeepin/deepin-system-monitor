#include <proc/readproc.h>
#include <proc/sysinfo.h>

#include <DApplication>
#include <DApplicationHelper>
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

    resize(width, height);
    setMinimumSize(600, 480);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::WindowModal);
    setWindowFlags(Qt::Dialog);
    titlebar()->setQuitMenuDisabled(true);
    titlebar()->setMenuVisible(false);
    setTitlebarShadowEnabled(false);
    titlebar()->setTitle({});

    auto *frame = new DWidget(this);
    frame->setAutoFillBackground(true);

    auto *vlayout = new QVBoxLayout(frame);
    vlayout->setSpacing(0);

    auto *cw = new DWidget(this);
    cw->setContentsMargins(0, 0, 0, 0);
    auto *grid = new QGridLayout(cw);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setVerticalSpacing(0);
    grid->setHorizontalSpacing(margin);
    cw->setLayout(grid);

    DLabel *appIcon = new DLabel(this);

    vlayout->addStretch(1);
    vlayout->addWidget(appIcon, 0, Qt::AlignVCenter | Qt::AlignHCenter);
    vlayout->addSpacing(16);
    auto *appNameLabel = new DLabel(this);
    QFont fn = appNameLabel->font();
    fn.setPointSize(fn.pointSize() + 2);
    appNameLabel->setFont(fn);
    vlayout->addWidget(appNameLabel, 0, Qt::AlignHCenter | Qt::AlignVCenter);
    vlayout->addSpacing(24);
    vlayout->addWidget(cw, 0, Qt::AlignVCenter | Qt::AlignHCenter);
    vlayout->addStretch(1);

    QString buf;
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Process name"));
    auto *procNameLabel = new DLabel(buf, this);
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Command line"));
    auto *procCmdLabel = new DLabel(buf, this);
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Start time"));
    auto *procStartLabel = new DLabel(buf, this);
    auto *procNameText = new DLabel(this);
    auto *procCmdText = new DLabel(this);
    procCmdText->setWordWrap(true);
    procCmdText->setScaledContents(true);
    auto *procStartText = new DLabel(this);
    grid->addWidget(procNameLabel, 0, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(procCmdLabel, 1, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(procStartLabel, 2, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(procNameText, 0, 1, Qt::AlignTop | Qt::AlignLeft);
    grid->addWidget(procCmdText, 1, 1, Qt::AlignTop | Qt::AlignLeft);
    grid->addWidget(procStartText, 2, 1, Qt::AlignTop | Qt::AlignLeft);

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

            procNameText->setText(name);
            procNameText->adjustSize();
            procCmdText->setText(cmdline);
            procCmdText->adjustSize();
            procStartText->setText(QFileInfo(QString("/proc/%1").arg(processId))
                                       .created()
                                       .toString("yyyy-MM-dd hh:mm:ss"));
            procStartText->adjustSize();
            break;
        }
    }

    frame->setLayout(vlayout);
    setCentralWidget(frame);
}

void ProcessAttributeDialog::resizeEvent(QResizeEvent *event)
{
    if (m_settings) {
        m_settings->setOption(kSettingKeyProcessAttributeDialogWidth, width());
        m_settings->setOption(kSettingKeyProcessAttributeDialogHeight, height());
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
