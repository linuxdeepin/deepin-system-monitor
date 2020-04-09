#include <DApplication>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DFrame>
#include <DLabel>
#include <DTextBrowser>
#include <DTitlebar>
#include <QAbstractTextDocumentLayout>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTextBlock>
#include <QVBoxLayout>
#include <QtMath>
#include <QScrollBar>

#include "process_attribute_dialog.h"
#include "settings.h"
#include "utils.h"

static const int kPreferedTextWidth = 200;
static const int kAppIconSize = 80;
static const int kIconToNameSpace = 10;
static const int kNameToGridSpace = 16;

ProcessAttributeDialog::ProcessAttributeDialog(pid_t pid,
                                               const QString procName,
                                               const QString displayName,
                                               const QString cmdline,
                                               const QIcon icon,
                                               qulonglong startTime,
                                               QWidget *parent)
    : DMainWindow(parent)
    , m_procName(procName)
    , m_displayName(displayName)
    , m_cmdline(cmdline)
    , m_startTime(startTime)
    , m_icon(icon)
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
    vlayout->addSpacing(kIconToNameSpace);

    m_appNameLabel = new DLabel(this);
    DFontSizeManager::instance()->bind(m_appNameLabel, DFontSizeManager::T5, QFont::Bold);
    m_appNameLabel->setElideMode(Qt::ElideMiddle);
    m_appNameLabel->setAlignment(Qt::AlignCenter);
    vlayout->addWidget(m_appNameLabel, 0, Qt::AlignHCenter | Qt::AlignVCenter);

    // spacing
    vlayout->addSpacing(kNameToGridSpace);

    auto *wnd = new QWidget(m_frame);
    wnd->setAutoFillBackground(false);
    auto *grid = new QGridLayout(wnd);
    grid->setHorizontalSpacing(m_margin);
    grid->setVerticalSpacing(0);

    QString buf;
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Name"));
    m_procNameLabel = new DLabel(wnd);
    m_procNameLabel->setText(buf);
    m_procNameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Command line"));
    m_procCmdLabel = new DLabel(wnd);
    m_procCmdLabel->setText(buf);
    m_procCmdLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    buf = QString("%1:").arg(DApplication::translate("Process.Attributes.Dialog", "Start time"));
    m_procStartLabel = new DLabel(wnd);
    m_procStartLabel->setText(buf);
    m_procStartLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_procNameText = new DTextBrowser(wnd);
    m_procNameText->setFrameStyle(QFrame::NoFrame);
    m_procNameText->setFixedWidth(kPreferedTextWidth);
    m_procNameText->viewport()->setBackgroundRole(QPalette::Window);
    m_procNameText->setWordWrapMode(QTextOption::WrapAnywhere);
    m_procNameText->document()->setDocumentMargin(0);

    m_procCmdText = new DTextBrowser(wnd);
    m_procCmdText->setFrameStyle(QFrame::NoFrame);
    m_procCmdText->setFixedWidth(kPreferedTextWidth);
    m_procCmdText->viewport()->setBackgroundRole(QPalette::Window);
    m_procCmdText->setWordWrapMode(QTextOption::WrapAnywhere);
    m_procCmdText->document()->setDocumentMargin(0);

    m_procStartText = new DTextBrowser(wnd);
    m_procStartText->setFrameStyle(QFrame::NoFrame);
    m_procStartText->setFixedWidth(kPreferedTextWidth);
    m_procStartText->viewport()->setBackgroundRole(QPalette::Window);
    m_procStartText->setWordWrapMode(QTextOption::WrapAnywhere);
    m_procStartText->document()->setDocumentMargin(0);

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

    appIcon->setPixmap(m_icon.pixmap(kAppIconSize, kAppIconSize));
    m_appNameLabel->setText(m_displayName);
    m_appNameLabel->setToolTip(m_displayName);
    m_procNameText->setText(m_procName);
    m_procCmdText->setText(m_cmdline);
    m_procStartText->setText(QDateTime::fromSecsSinceEpoch(qint64(m_startTime)).toString("yyyy-MM-dd hh:mm:ss"));

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

    if (m_procCmdText) {
        QFontMetrics fm(m_procCmdText->font());
        m_cmdh = m_frame->height()
                 - (kAppIconSize + kIconToNameSpace + m_appNameLabel->height() + kNameToGridSpace + m_margin * 2)
                 - m_procNameText->height() - m_procStartText->height();
        int singleline = fm.size(Qt::TextSingleLine, m_procCmdText->toPlainText()).height();
        m_cmdh = (qFloor(m_cmdh * 1.0 / singleline) - 1) * singleline;

        if (m_cmdh > m_procCmdText->document()->size().height()) {
            m_cmdh = int(m_procCmdText->document()->size().height());
        }
        m_procCmdText->setFixedHeight(m_cmdh);
        m_procCmdText->verticalScrollBar()->setValue(0);
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
        if (obj == m_procNameLabel || obj == m_procCmdLabel || obj == m_procStartLabel) {
            auto *ctl = dynamic_cast<DLabel *>(obj);
            QFontMetrics fm(ctl->font());
            int max = fm.size(Qt::TextSingleLine, ctl->text()).width();
            ctl->setFixedWidth(max);
            ctl->setFixedHeight(ctl->height());
        } else if (obj == m_procNameText || obj == m_procStartText || obj == m_procCmdText) {
            auto *ctl = dynamic_cast<DTextBrowser *>(obj);
            QFontMetrics fm(ctl->font());
            int max = fm.size(Qt::TextSingleLine, ctl->toPlainText()).width();
            if (max > kPreferedTextWidth) {
                max = kPreferedTextWidth;
            }
            ctl->setFixedWidth(max);
            ctl->setMaximumHeight(int(ctl->document()->size().height()));
            // fix text cursor not at the first line issue
            auto textCur = ctl->textCursor();
            textCur.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1);
            ctl->setTextCursor(textCur);
        }
    } else if (event->type() == QEvent::Leave) {
        if (obj == m_procNameText) {
            if (m_procNameText->textCursor().hasSelection()) {
                m_procNameText->setFocus();

                auto cur = m_procCmdText->textCursor();
                cur.clearSelection();
                m_procCmdText->setTextCursor(cur);

                cur = m_procStartText->textCursor();
                cur.clearSelection();
                m_procStartText->setTextCursor(cur);
            }
        } else if (obj == m_procCmdText) {
            if (m_procCmdText->textCursor().hasSelection()) {
                m_procCmdText->setFocus();

                auto cur = m_procNameText->textCursor();
                cur.clearSelection();
                m_procNameText->setTextCursor(cur);

                cur = m_procStartText->textCursor();
                cur.clearSelection();
                m_procStartText->setTextCursor(cur);
            }
        } else if (obj == m_procStartText) {
            if (m_procStartText->textCursor().hasSelection()) {
                m_procStartText->setFocus();

                auto cur = m_procCmdText->textCursor();
                cur.clearSelection();
                m_procCmdText->setTextCursor(cur);

                cur = m_procNameText->textCursor();
                cur.clearSelection();
                m_procNameText->setTextCursor(cur);
            }
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
    } else if (event->type() == QEvent::MouseButtonPress) {
        auto *ev = dynamic_cast<QMouseEvent *>(event);
        if (!(ev->button() & Qt::RightButton)) {
            auto cur = m_procNameText->textCursor();
            cur.clearSelection();
            m_procNameText->setTextCursor(cur);

            cur = m_procCmdText->textCursor();
            cur.clearSelection();
            m_procCmdText->setTextCursor(cur);

            cur = m_procStartText->textCursor();
            cur.clearSelection();
            m_procStartText->setTextCursor(cur);
        }
    }

    return DMainWindow::eventFilter(obj, event);
}
