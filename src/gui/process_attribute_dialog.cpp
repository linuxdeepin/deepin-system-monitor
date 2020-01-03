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
#include <QtMath>

#include "process_attribute_dialog.h"
#include "settings.h"
#include "utils.h"

using namespace Utils;

static const int kPreferedLabelWidth = 80;
static const int kPreferedTextWidth = 200;

ProcessAttributeDialog::ProcessAttributeDialog(pid_t pid, QWidget *parent)
    : DMainWindow(parent)
    , m_pid(pid)
{
    m_settings = Settings::instance();

    initUI();

    installEventFilter(this);
    if (m_procNameLabel) {
        m_procNameLabel->installEventFilter(this);
    }
    if (m_procNameText) {
        m_procNameText->installEventFilter(this);
    }
    if (m_procCmdLabel) {
        m_procCmdLabel->installEventFilter(this);
    }
    if (m_procCmdText) {
        m_procCmdText->installEventFilter(this);
    }
    if (m_procStartLabel) {
        m_procStartLabel->installEventFilter(this);
    }
    if (m_procStartText) {
        m_procStartText->installEventFilter(this);
    }
}

void ProcessAttributeDialog::initUI()
{
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QStyleOption option;
    option.initFrom(this);
    m_margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

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

    auto *vlayout = new QVBoxLayout(m_frame);
    vlayout->setContentsMargins(m_margin, 0, m_margin, 0);
    vlayout->setSpacing(m_margin);

    vlayout->addStretch(1);

    DLabel *appIcon = new DLabel(this);
    vlayout->addWidget(appIcon, 0, Qt::AlignVCenter | Qt::AlignHCenter);

    // spacing
    vlayout->addSpacing(10);

    auto *appNameLabel = new DLabel(this);
    DFontSizeManager::instance()->bind(appNameLabel, DFontSizeManager::T5, QFont::Bold);
    vlayout->addWidget(appNameLabel, 0, Qt::AlignHCenter | Qt::AlignVCenter);

    // spacing
    vlayout->addSpacing(16);

    auto *wnd = new QWidget(m_frame);
    wnd->setAutoFillBackground(false);
    auto *grid = new QGridLayout(wnd);
    grid->setHorizontalSpacing(m_margin);

    QString buf;
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Process name"));
    m_procNameLabel = new DLabel(this);
    m_procNameLabel->setText(buf);
    m_procNameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Command line"));
    m_procCmdLabel = new DLabel(this);
    m_procCmdLabel->setText(buf);
    m_procCmdLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Start time"));
    m_procStartLabel = new DLabel(this);
    m_procStartLabel->setText(buf);
    m_procStartLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_procNameText = new DLabel(this);
    m_procNameText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_procCmdText = new DLabel(this);
    m_procCmdText->setWordWrap(true);
    m_procCmdText->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_procCmdText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_procStartText = new DLabel(this);
    m_procStartText->setTextInteractionFlags(Qt::TextSelectableByMouse);

    grid->addWidget(m_procNameLabel, 0, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(m_procNameText, 0, 1, Qt::AlignTop | Qt::AlignLeft);

    grid->addWidget(m_procCmdLabel, 1, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(m_procCmdText, 1, 1, Qt::AlignTop | Qt::AlignLeft);

    grid->addWidget(m_procStartLabel, 2, 0, Qt::AlignTop | Qt::AlignRight);
    grid->addWidget(m_procStartText, 2, 1, Qt::AlignTop | Qt::AlignLeft);

    wnd->setLayout(grid);
    vlayout->addWidget(wnd, 0, Qt::AlignCenter);

    // spacing
    vlayout->addStretch(1);

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
            m_procCmdText->setText(QUrl::fromPercentEncoding(cmdline.toUtf8()));
            m_procStartText->setText(QFileInfo(QString("/proc/%1").arg(processId))
                                         .created()
                                         .toString("yyyy-MM-dd hh:mm:ss"));
            break;
        }
    }

    m_frame->setLayout(vlayout);
    setCentralWidget(m_frame);
}

void ProcessAttributeDialog::resizeEvent(QResizeEvent *event)
{
    DMainWindow::resizeEvent(event);

    if (m_settings) {
        m_settings->setOption(kSettingKeyProcessAttributeDialogWidth, width());
        m_settings->setOption(kSettingKeyProcessAttributeDialogHeight, height());
    }

    if (m_frame) {
        m_tbShadow->setFixedWidth(m_frame->size().width());
        m_tbShadow->raise();
        m_tbShadow->show();
    }
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
    if (event->type() == QEvent::Show) {
        auto *ctl = dynamic_cast<DLabel *>(obj);
        if (obj == m_procNameLabel || obj == m_procCmdLabel || obj == m_procStartLabel) {
            int max = 0;
            if (ctl->width() < kPreferedLabelWidth) {
                max = kPreferedLabelWidth;
            } else {
                max = ctl->width();
            }
            ctl->setFixedWidth(max);
            ctl->setFixedHeight(ctl->height());
        } else if (obj == m_procNameText || obj == m_procCmdText || obj == m_procStartText) {
            QFontMetrics fm(ctl->font());
            int w = fm.size(Qt::TextSingleLine, ctl->text()).width();
            int max = 0;
            if (w > kPreferedTextWidth) {
                QRect rect {0, 0, kPreferedTextWidth, 0};
                rect = fm.boundingRect(rect, Qt::AlignTop | Qt::AlignLeft | Qt::TextWrapAnywhere,
                                       ctl->text());
                // round up to next integer line height
                //                max = (qFloor(rect.height() * 1. / fm.height()) + 1) *
                //                fm.height();
                max = rect.height();
            } else {
                max = ctl->height();
            }
            ctl->setFixedWidth(kPreferedTextWidth);
            ctl->setFixedHeight(max);
        }
    } else if (event->type() == QEvent::Paint) {
        if (obj == m_procCmdText) {
            auto *ctl = dynamic_cast<DLabel *>(obj);
            auto *pev = dynamic_cast<QPaintEvent *>(event);
            QPainter painter(ctl);
            painter.drawText(pev->rect(), Qt::AlignLeft | Qt::AlignTop | Qt::TextWrapAnywhere,
                             ctl->text());
            return true;
        }
    } else if (event->type() == QEvent::KeyPress) {
        if (obj == this) {
            auto *kev = dynamic_cast<QKeyEvent *>(event);
            if (kev->matches(QKeySequence::Cancel)) {
                close();
                return true;
            } else {
                return false;
            }
        }
    }

    return DMainWindow::eventFilter(obj, event);
}
